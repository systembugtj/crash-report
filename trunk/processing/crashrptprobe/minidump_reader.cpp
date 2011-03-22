/************************************************************************************* 
 This file is a part of CrashRpt library.

 Copyright (c) 2003, Michael Carruth
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of the author nor the names of its contributors
 may be used to endorse or promote products derived from this software without
 specific prior written permission.
 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************************/
//  rewrited by Shunping Ye(yeshunping@gmail.com)

#include "stdafx.h"
#include "minidump_reader.h"
#include <assert.h>
#include "utility.h"
#include "strconv.h"
#include "md5.h"

MiniDumpReader* g_pMiniDumpReader = NULL;

// Callback function prototypes
BOOL CALLBACK ReadProcessMemoryProc64(HANDLE hProcess, DWORD64 lpBaseAddress,
                                      PVOID lpBuffer, DWORD nSize,
                                      LPDWORD lpNumberOfBytesRead);
PVOID CALLBACK FunctionTableAccessProc64(HANDLE hProcess, DWORD64 AddrBase);
DWORD64 CALLBACK GetModuleBaseProc64(HANDLE hProcess, DWORD64 Address);
BOOL CALLBACK SymRegisterCallbackProc64(HANDLE hProcess,
                                        ULONG ActionCode,
                                        ULONG64 CallbackData,
                                        ULONG64 UserContext);

MiniDumpReader::MiniDumpReader() {
  loaded_ = FALSE;
  read_sys_info_stream_ = FALSE;
  read_exception_stream_ = FALSE;
  read_module_list_stream_ = FALSE;
  read_memory_list_stream_ = FALSE;
  read_thread_list_stream_ = FALSE;
  file_handle_ = INVALID_HANDLE_VALUE;
  handle_file_mapping_ = NULL;
  map_address_ = NULL;
}

MiniDumpReader::~MiniDumpReader() {
  Close();
}

int MiniDumpReader::Open(CString sFileName, CString sSymSearchPath) {
  static DWORD dwProcessID = 0;

  if (loaded_) {
    return 1;
  }

  file_name_ = sFileName;
  symbol_search_path_ = sSymSearchPath;
  file_handle_ = CreateFile(sFileName, FILE_ALL_ACCESS, 0, NULL,
                            OPEN_EXISTING, NULL, NULL);
  if (file_handle_ == INVALID_HANDLE_VALUE) {
    Close();
    return 1;
  }
  handle_file_mapping_ = CreateFileMapping(file_handle_, NULL,
                                           PAGE_READONLY, 0, 0, 0);
  if (handle_file_mapping_ == NULL) {
    Close();
    return 2;
  }
  map_address_ = MapViewOfFile(handle_file_mapping_, FILE_MAP_READ, 0, 0, 0);
  if (map_address_ == NULL) {
    Close();
    return 3;
  }

  dump_data_.hProcess = (HANDLE)(++dwProcessID);
  DWORD dwOptions = 0;
  // Symbols are not loaded until a reference is made requiring the symbols be loaded.
  //dwOptions |= SYMOPT_DEFERRED_LOADS;
  // Do not load an unmatched .pdb file.
  dwOptions |= SYMOPT_EXACT_SYMBOLS;
  // Do not display system dialog boxes when there is a media failure such
  //  as no media in a drive.
  dwOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
  // All symbols are presented in undecorated form.
  dwOptions |= SYMOPT_UNDNAME;
  SymSetOptions(dwOptions);

  strconv_t strconv;
  BOOL bSymInit = SymInitializeW(dump_data_.hProcess,
                                 strconv.t2w(sSymSearchPath), FALSE);
  if (!bSymInit) {
    dump_data_.hProcess = NULL;
    Close();
    return 5;
  }

  read_sys_info_stream_ = !ReadSysInfoStream();
  read_module_list_stream_ = !ReadModuleListStream();
  read_thread_list_stream_ = !ReadThreadListStream();
  read_memory_list_stream_ = !ReadMemoryListStream();
  read_exception_stream_ = !ReadExceptionStream();
  loaded_ = TRUE;
  return 0;
}

void MiniDumpReader::Close() {
  UnmapViewOfFile(map_address_);
  if (handle_file_mapping_ != NULL) {
    CloseHandle(handle_file_mapping_);
  }
  if (file_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle_);
  }
  map_address_ = NULL;
  if (dump_data_.hProcess != NULL) {
    SymCleanup(dump_data_.hProcess);
  }
}

BOOL MiniDumpReader::CheckDbgHelpApiVersion() {
  // Set valid dbghelp API version
  API_VERSION CompiledApiVer;
  CompiledApiVer.MajorVersion = 6;
  CompiledApiVer.MinorVersion = 1;
  CompiledApiVer.Revision = 11;
  CompiledApiVer.Reserved = 0;
  LPAPI_VERSION pActualApiVer = ImagehlpApiVersionEx(&CompiledApiVer);
  if (CompiledApiVer.MajorVersion != pActualApiVer->MajorVersion
      || CompiledApiVer.MinorVersion != pActualApiVer->MinorVersion
      || CompiledApiVer.Revision != pActualApiVer->Revision) {
    return FALSE; // Not exact version of dbghelp.dll! Expected v6.11.
  }

  return TRUE;
}

// Extracts a UNICODE string stored in minidump file by its relative address
CString MiniDumpReader::GetMinidumpString(LPVOID start_addr, RVA rva) {
  MINIDUMP_STRING* pms = (MINIDUMP_STRING*) ((LPBYTE) start_addr + rva);
  CString sModule = pms->Buffer;
  return sModule;
}

int MiniDumpReader::ReadSysInfoStream() {
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(map_address_, SystemInfoStream, &pmd,
                                 &pStreamStart, &uStreamSize);

  if (bRead) {
    MINIDUMP_SYSTEM_INFO* pSysInfo = (MINIDUMP_SYSTEM_INFO*) pStreamStart;
    dump_data_.processor_architecture = pSysInfo->ProcessorArchitecture;
    dump_data_.processors_number = pSysInfo->NumberOfProcessors;
    dump_data_.product_type = pSysInfo->ProductType;
    dump_data_.major_version = pSysInfo->MajorVersion;
    dump_data_.minor_version = pSysInfo->MinorVersion;
    dump_data_.build_number = pSysInfo->BuildNumber;
    dump_data_.lastest_service_pack = GetMinidumpString(map_address_,
                                             pSysInfo->CSDVersionRva);

    // Clean up
    pStreamStart = NULL;
    uStreamSize = 0;
    pmd = NULL;
  } else {
    return 1;
  }
  return 0;
}

int MiniDumpReader::ReadExceptionStream() {
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(map_address_, ExceptionStream, &pmd,
      &pStreamStart, &uStreamSize);

  if (bRead) {
    MINIDUMP_EXCEPTION_STREAM* pExceptionStream =
        (MINIDUMP_EXCEPTION_STREAM*) pStreamStart;
    if (pExceptionStream != NULL && uStreamSize
        >= sizeof(MINIDUMP_EXCEPTION_STREAM)) {
      dump_data_.exception_thread_id = pExceptionStream->ThreadId;
      dump_data_.exception_code
          = pExceptionStream->ExceptionRecord.ExceptionCode;
      dump_data_.exception_address
          = pExceptionStream->ExceptionRecord.ExceptionAddress;
      dump_data_.exception_thread_context
          = (CONTEXT*) (((LPBYTE) map_address_)
              + pExceptionStream->ThreadContext.Rva);

      CString sMsg;
      int nExcModuleRowID = GetModuleRowIdByAddress(
          dump_data_.exception_address);
      if (nExcModuleRowID >= 0) {
        sMsg.Format(_T("Unhandled exception at 0x%I64x in %s: 0x%x : %s"),
            dump_data_.exception_address,
            dump_data_.modules_list[nExcModuleRowID].module_name,
            dump_data_.exception_code, _T("Exception description."));
      } else {

      }
      dump_data_.load_log.push_back(sMsg);
    }
  } else {
    CString sMsg;
    sMsg = _T("No exception information found in minidump.");
    dump_data_.load_log.push_back(sMsg);
    return 1;
  }

  return 0;
}

int MiniDumpReader::ReadModuleListStream() {
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;
  strconv_t strconv;

  bRead = MiniDumpReadDumpStream(map_address_, ModuleListStream, &pmd,
      &pStreamStart, &uStreamSize);

  if (bRead) {
    MINIDUMP_MODULE_LIST* pModuleStream = (MINIDUMP_MODULE_LIST*) pStreamStart;
    if (pModuleStream != NULL) {
      ULONG32 uNumberOfModules = pModuleStream->NumberOfModules;
      ULONG32 i;
      for (i = 0; i < uNumberOfModules; i++) {
        MINIDUMP_MODULE* pModule =
            (MINIDUMP_MODULE*) ((LPBYTE) pModuleStream->Modules + i
                * sizeof(MINIDUMP_MODULE));

        CString sModuleName = GetMinidumpString(map_address_,
            pModule->ModuleNameRva);
        const wchar_t* szModuleName = strconv.t2w(sModuleName);
        DWORD64 dwBaseAddr = pModule->BaseOfImage;
        DWORD64 dwImageSize = pModule->SizeOfImage;

        CString sShortModuleName = sModuleName;
        int pos = -1;
        pos = sModuleName.ReverseFind('\\');
        if (pos >= 0)
          sShortModuleName = sShortModuleName.Mid(pos + 1);

        /*DWORD64 dwLoadResult = */
        SymLoadModuleExW(dump_data_.hProcess, NULL, (PWSTR) szModuleName,
            NULL, dwBaseAddr, (DWORD) dwImageSize, NULL, 0);

        IMAGEHLP_MODULE64 modinfo;
        memset(&modinfo, 0, sizeof(IMAGEHLP_MODULE64));
        modinfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        BOOL bModuleInfo = SymGetModuleInfo64(dump_data_.hProcess,
            dwBaseAddr, &modinfo);
        MinidumpModule m;
        if (!bModuleInfo) {
          m.image_unmatched = TRUE;
          m.no_symbol_info = TRUE;
          m.pdb_unmatched = TRUE;
          m.version_info = NULL;
          m.image_name = sModuleName;
          m.module_name = sShortModuleName;
          m.base_address = dwBaseAddr;
          m.image_size = dwImageSize;
        } else {
          m.base_address = modinfo.BaseOfImage;
          m.image_size = modinfo.ImageSize;
          m.module_name = sShortModuleName;
          m.image_name = modinfo.ImageName;
          m.loaded_image_name = modinfo.LoadedImageName;
          m.loaded_pdb_name = modinfo.LoadedPdbName;
          m.version_info = &pModule->VersionInfo;
          m.pdb_unmatched = modinfo.PdbUnmatched;
          BOOL bTimeStampMatched = pModule->TimeDateStamp
              == modinfo.TimeDateStamp;
          m.image_unmatched = !bTimeStampMatched;
          m.no_symbol_info = !modinfo.GlobalSymbols;
        }

        dump_data_.modules_list.push_back(m);
        dump_data_.module_index[m.base_address] = dump_data_.modules_list.size()
            - 1;

        CString sMsg;
        if (m.image_unmatched)
          sMsg.Format(_T("Loaded '*%s'"), sModuleName);
        else
          sMsg.Format(_T("Loaded '%s'"), m.loaded_image_name);

        if (m.image_unmatched)
          sMsg += _T(", No matching binary found.");
        else if (m.pdb_unmatched)
          sMsg += _T(", No matching PDB file found.");
        else {
          if (m.no_symbol_info)
            sMsg += _T(", No symbols loaded.");
          else
            sMsg += _T(", Symbols loaded.");
        }
        dump_data_.load_log.push_back(sMsg);
      }
    }
  } else {
    return 1;
  }

  return 0;
}

int MiniDumpReader::GetModuleRowIdByBaseAddr(DWORD64 dwBaseAddr) {
  std::map<DWORD64, size_t>::iterator it = dump_data_.module_index.find(
      dwBaseAddr);
  if (it != dump_data_.module_index.end())
    return (int) it->second;
  return -1;
}

int MiniDumpReader::GetModuleRowIdByAddress(DWORD64 dwAddress) {
  UINT i;
  for (i = 0; i < dump_data_.modules_list.size(); i++) {
    if (dump_data_.modules_list[i].base_address <= dwAddress && dwAddress
        < dump_data_.modules_list[i].base_address
            + dump_data_.modules_list[i].image_size)
      return i;
  }
  return -1;
}

int MiniDumpReader::GetThreadRowIdByThreadId(DWORD dwThreadId) {
  std::map<DWORD, size_t>::iterator it = dump_data_.thread_index.find(
      dwThreadId);
  if (it != dump_data_.thread_index.end())
    return (int) it->second;
  return -1;
}

int MiniDumpReader::ReadMemoryListStream() {
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(map_address_, MemoryListStream, &pmd,
      &pStreamStart, &uStreamSize);

  if (bRead) {
    MINIDUMP_MEMORY_LIST* pMemStream = (MINIDUMP_MEMORY_LIST*) pStreamStart;
    if (pMemStream != NULL) {
      ULONG32 uNumberOfMemRanges = pMemStream->NumberOfMemoryRanges;
      ULONG i;
      for (i = 0; i < uNumberOfMemRanges; i++) {
        MINIDUMP_MEMORY_DESCRIPTOR* pMemDesc =
            (MINIDUMP_MEMORY_DESCRIPTOR*) (&pMemStream->MemoryRanges[i]);
        MinidumpMemRange mr;
        mr.start_address = pMemDesc->StartOfMemoryRange;
        mr.data_size = pMemDesc->Memory.DataSize;
        mr.start_pointer = (LPBYTE) map_address_ + pMemDesc->Memory.Rva;

        dump_data_.memory_ranges.push_back(mr);
      }
    }
  } else {
    return 1;
  }

  return 0;
}

int MiniDumpReader::ReadThreadListStream() {
  LPVOID pStreamStart = NULL;
  ULONG uStreamSize = 0;
  MINIDUMP_DIRECTORY* pmd = NULL;
  BOOL bRead = FALSE;

  bRead = MiniDumpReadDumpStream(map_address_, ThreadListStream, &pmd,
      &pStreamStart, &uStreamSize);

  if (bRead) {
    MINIDUMP_THREAD_LIST* pThreadList = (MINIDUMP_THREAD_LIST*) pStreamStart;
    if (pThreadList != NULL && uStreamSize >= sizeof(MINIDUMP_THREAD_LIST)) {
      ULONG32 uThreadCount = pThreadList->NumberOfThreads;

      ULONG32 i;
      for (i = 0; i < uThreadCount; i++) {
        MINIDUMP_THREAD* pThread =
            (MINIDUMP_THREAD*) (&pThreadList->Threads[i]);

        MinidumpThread mt;
        mt.thread_id = pThread->ThreadId;
        mt.thread_context = (CONTEXT*) (((LPBYTE) map_address_)
            + pThread->ThreadContext.Rva);

        dump_data_.threads_list.push_back(mt);
        dump_data_.thread_index[mt.thread_id] = dump_data_.threads_list.size()
            - 1;
      }
    }
  } else {
    return 1;
  }
  return 0;
}

int MiniDumpReader::StackWalk(DWORD dwThreadId) {
  int nThreadIndex = GetThreadRowIdByThreadId(dwThreadId);
  if (dump_data_.threads_list[nThreadIndex].has_stack_walk == TRUE)
    return 0; // Already done

  CONTEXT* pThreadContext = NULL;

  if (dump_data_.threads_list[nThreadIndex].thread_id
      == dump_data_.exception_thread_id)
    pThreadContext = dump_data_.exception_thread_context;
  else
    pThreadContext = dump_data_.threads_list[nThreadIndex].thread_context;

  if (pThreadContext == NULL)
    return 1;

  // Make modifiable context
  CONTEXT Context;
  memcpy(&Context, pThreadContext, sizeof(CONTEXT));

  g_pMiniDumpReader = this;

  // Init stack frame with correct initial values
  // See this:
  // http://www.codeproject.com/KB/threads/StackWalker.aspx
  //
  // Given a current dbghelp, your code should:
  //  1. Always use StackWalk64
  //  2. Always set AddrPC to the current instruction pointer (Eip on x86, Rip on x64 and StIIP on IA64)
  //  3. Always set AddrStack to the current stack pointer (Esp on x86, Rsp on x64 and IntSp on IA64)
  //  4. Set AddrFrame to the current frame pointer when meaningful. On x86 this is Ebp, on x64 you 
  //     can use Rbp (but is not used by VC2005B2; instead it uses Rdi!) and on IA64 you can use RsBSP. 
  //     StackWalk64 will ignore the value when it isn't needed for unwinding.
  //  5. Set AddrBStore to RsBSP for IA64. 

  STACKFRAME64 sf;
  memset(&sf, 0, sizeof(STACKFRAME64));

  sf.AddrPC.Mode = AddrModeFlat;
  sf.AddrFrame.Mode = AddrModeFlat;
  sf.AddrStack.Mode = AddrModeFlat;
  sf.AddrBStore.Mode = AddrModeFlat;

  DWORD dwMachineType = 0;
  switch (dump_data_.processor_architecture) {
#ifdef _X86_
  case PROCESSOR_ARCHITECTURE_INTEL:
  dwMachineType = IMAGE_FILE_MACHINE_I386;
  sf.AddrPC.Offset = pThreadContext->Eip;
  sf.AddrStack.Offset = pThreadContext->Esp;
  sf.AddrFrame.Offset = pThreadContext->Ebp;
  break;
#endif
#ifdef _AMD64_
  case PROCESSOR_ARCHITECTURE_AMD64:
  dwMachineType = IMAGE_FILE_MACHINE_AMD64;
  sf.AddrPC.Offset = pThreadContext->Rip;
  sf.AddrStack.Offset = pThreadContext->Rsp;
  sf.AddrFrame.Offset = pThreadContext->Rbp;
  break;
#endif
#ifdef _IA64_
  case PROCESSOR_ARCHITECTURE_AMD64:
  dwMachineType = IMAGE_FILE_MACHINE_IA64;
  sf.AddrPC.Offset = pThreadContext->StIIP;
  sf.AddrStack.Offset = pThreadContext->IntSp;
  sf.AddrFrame.Offset = pThreadContext->RsBSP;
  sf.AddrBStore.Offset = pThreadContext->RsBSP;
  break;
#endif 
  default: {
    assert(0);
    return 1; // Unsupported architecture
  }
  }

  for (;;) {
    BOOL bWalk = ::StackWalk64(dwMachineType, // machine type
        dump_data_.hProcess, // our process handle
        (HANDLE) dwThreadId, // thread ID
        &sf, // stack frame
        dwMachineType == IMAGE_FILE_MACHINE_I386 ? NULL : (&Context), // used for non-I386 machines
        ReadProcessMemoryProc64, // our routine
        FunctionTableAccessProc64, // our routine
        GetModuleBaseProc64, // our routine
        NULL // safe to be NULL
        );

    if (!bWalk)
      break;

    MinidumpStackFrame stack_frame;
    stack_frame.address_PC_offset = sf.AddrPC.Offset;

    // Get module info
    IMAGEHLP_MODULE64 mi;
    memset(&mi, 0, sizeof(IMAGEHLP_MODULE64));
    mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    BOOL bGetModuleInfo = SymGetModuleInfo64(dump_data_.hProcess,
        sf.AddrPC.Offset, &mi);
    if (bGetModuleInfo) {
      stack_frame.module_row_id = GetModuleRowIdByBaseAddr(mi.BaseOfImage);
    }

    // Get symbol info
    DWORD64 dwDisp64;
    BYTE buffer[4096];
    SYMBOL_INFO* sym_info = (SYMBOL_INFO*) buffer;
    sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info->MaxNameLen = 4096 - sizeof(SYMBOL_INFO) - 1;
    BOOL bGetSym = SymFromAddr(dump_data_.hProcess, sf.AddrPC.Offset,
        &dwDisp64, sym_info);

    if (bGetSym) {
      stack_frame.symbol_name = CString(sym_info->Name, sym_info->NameLen);
      stack_frame.symbol_offset = dwDisp64;
    }

    // Get source filename and line
    DWORD dwDisplacement;
    IMAGEHLP_LINE64 line;
    BOOL bGetLine = SymGetLineFromAddr64(dump_data_.hProcess,
        sf.AddrPC.Offset, &dwDisplacement, &line);

    if (bGetLine) {
      stack_frame.source_file_name = line.FileName;
      stack_frame.source_line_number = line.LineNumber;
    }

    dump_data_.threads_list[nThreadIndex].stack_trace.push_back(stack_frame);
  }

  CString sStackTrace;
  UINT i;
  for (i = 0; i < dump_data_.threads_list[nThreadIndex].stack_trace.size(); i++) {
    MinidumpStackFrame& frame = dump_data_.threads_list[nThreadIndex].stack_trace[i];

    if (frame.symbol_name.IsEmpty())
      continue;

    CString sModuleName;
    CString sAddrPCOffset;
    CString sSymbolName;
    CString sOffsInSymbol;
    CString sSourceFile;
    CString sSourceLine;

    if (frame.module_row_id >= 0) {
      sModuleName = dump_data_.modules_list[frame.module_row_id].module_name;
    }

    sSymbolName = frame.symbol_name;
    sAddrPCOffset.Format(_T("0x%I64x"), frame.address_PC_offset);
    sSourceFile = frame.source_file_name;
    sSourceLine.Format(_T("%d"), frame.source_line_number);
    sOffsInSymbol.Format(_T("0x%I64x"), frame.symbol_offset);

    CString str;
    str = sModuleName;
    if (!str.IsEmpty())
      str += _T("!");

    if (sSymbolName.IsEmpty())
      str += sAddrPCOffset;
    else {
      str += sSymbolName;
      str += _T("+");
      str += sOffsInSymbol;
    }

    if (!sSourceFile.IsEmpty()) {
      size_t pos = sSourceFile.ReverseFind('\\');
      if (pos >= 0)
        sSourceFile = sSourceFile.Mid((int) pos + 1);
      str += _T(" [ ");
      str += sSourceFile;
      str += _T(": ");
      str += sSourceLine;
      str += _T(" ] ");
    }

    sStackTrace += str;
    sStackTrace += _T("\n");
  }

  if (!sStackTrace.IsEmpty()) {
    strconv_t strconv;
    const char* szStackTrace = strconv.t2utf8(sStackTrace);
    MD5 md5;
    MD5_CTX md5_ctx;
    unsigned char md5_hash[16];
    md5.MD5Init(&md5_ctx);
    md5.MD5Update(&md5_ctx, (unsigned char*) szStackTrace,
        (unsigned int) strlen(szStackTrace));
    md5.MD5Final(md5_hash, &md5_ctx);

    for (i = 0; i < 16; i++) {
      CString number;
      number.Format(_T("%02x"), md5_hash[i]);
      dump_data_.threads_list[nThreadIndex].stack_trace_md5 += number;
    }
  }

  dump_data_.threads_list[nThreadIndex].has_stack_walk = TRUE;

  return 0;
}

// This callback function is used by StackWalk64. It provides access to 
// ranges of memory stored in minidump file
BOOL CALLBACK ReadProcessMemoryProc64(HANDLE hProcess, DWORD64 lpBaseAddress,
    PVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead) {
  *lpNumberOfBytesRead = 0;

  // Validate input parameters
  if (hProcess != g_pMiniDumpReader->dump_data().hProcess || lpBaseAddress
      == NULL || lpBuffer == NULL || nSize == 0) {
    // Invalid parameter
    return FALSE;
  }

  ULONG i;
  for (i = 0; i < g_pMiniDumpReader->dump_data().memory_ranges.size(); i++) {
    const MinidumpMemRange& mr = g_pMiniDumpReader->dump_data().memory_ranges[i];
    if (lpBaseAddress >= mr.start_address && lpBaseAddress
        < mr.start_address + mr.data_size) {
      DWORD64 dwOffs = lpBaseAddress - mr.start_address;
      LONG64 lBytesRead = 0;
      if (mr.data_size - dwOffs > nSize) {
        lBytesRead = nSize;
      } else {
        lBytesRead = mr.data_size - dwOffs;
      }
      if (lBytesRead <= 0 || nSize < lBytesRead) {
        return FALSE;
      }
      *lpNumberOfBytesRead = (DWORD) lBytesRead;
      memcpy(lpBuffer, (LPBYTE) mr.start_pointer + dwOffs, (size_t) lBytesRead);
      return TRUE;
    }
  }
  return FALSE;
}

// This callback function is used by StackWalk64. It provides access to 
// function table stored in minidump file
PVOID CALLBACK FunctionTableAccessProc64(HANDLE hProcess, DWORD64 AddrBase) {
  return SymFunctionTableAccess64(hProcess, AddrBase);
}

// This callback function is used by StackWalk64. It provides access to 
// module list stored in minidump file
DWORD64 CALLBACK GetModuleBaseProc64(HANDLE hProcess, DWORD64 Address) {
  return SymGetModuleBase64(hProcess, Address);
}
