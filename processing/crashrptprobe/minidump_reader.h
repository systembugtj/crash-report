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


#ifndef MINIDUMP_READER_H_
#define MINIDUMP_READER_H_

#include "stdafx.h"
#include "dbghelp.h"
#include <map>
#include <vector>

// Describes a loaded module
struct MinidumpModule {
  ULONG64 base_address;
  ULONG64 image_size;
  CString module_name; // Module name  
  CString image_name; // The image name. The name may or may not contain a full path.
  CString loaded_image_name; // The full path and file name of the file from which symbols were loaded. 
  CString loaded_pdb_name; // The full path and file name of the .pdb file.
  BOOL image_unmatched; // If TRUE than there wasn't matching binary found.
  BOOL pdb_unmatched; // If TRUE than there wasn't matching PDB file found.
  BOOL no_symbol_info; // If TRUE than no symbols were generated for this module.
  VS_FIXEDFILEINFO* version_info; // Version info for module.
};

// Describes a stack frame
struct MinidumpStackFrame {
  MinidumpStackFrame() {
    module_row_id = -1;
    symbol_offset = 0;
    source_line_number = -1;
  }

  DWORD64 address_PC_offset;
  int module_row_id; // ROWID of the record in CPR_MDMP_MODULES table.
  CString symbol_name; // Name of symbol
  DWORD64 symbol_offset; // Offset in symbol
  CString source_file_name; // Name of source file
  int source_line_number; // Line number in the source file
};

// Describes a thread
struct MinidumpThread {
  MinidumpThread() {
    thread_id = 0;
    thread_context = NULL;
    has_stack_walk = FALSE;
  }

  DWORD thread_id; // Thread ID.
  CONTEXT* thread_context; // Thread context
  BOOL has_stack_walk; // Was stack trace retrieved for this thread?
  CString stack_trace_md5;
  std::vector<MinidumpStackFrame> stack_trace; // Stack trace for this thread.
};

// Describes a memory range
struct MinidumpMemRange {
  ULONG64 start_address; // Starting address
  ULONG32 data_size; // Size of data
  LPVOID start_pointer; // Pointer to the memrange data stored in minidump
};

// Minidump data
struct MinidumpData {
  MinidumpData() {
    hProcess = INVALID_HANDLE_VALUE;
    processor_architecture = 0;
    product_type = 0;
    major_version = 0;
    minor_version = 0;
    build_number = 0;
    exception_code = 0;
    exception_address = 0;
    exception_thread_id = 0;
    exception_thread_context = NULL;
  }

  HANDLE hProcess;

  USHORT processor_architecture;
  UCHAR processors_number;
   // Type of machine (workstation, server, ...)
  UCHAR product_type;
  ULONG major_version;
  ULONG minor_version;
  ULONG build_number;
  CString lastest_service_pack;

  // info for exception
  ULONG32 exception_code;
  ULONG64 exception_address;
  ULONG32 exception_thread_id;
  CONTEXT* exception_thread_context;

  std::vector<MinidumpThread> threads_list;
   // <thread_id, thread_entry_index> pairs
  std::map<DWORD, size_t> thread_index;
  std::vector<MinidumpModule> modules_list;
   // <base_addr, module_entry_index> pairs
  std::map<DWORD64, size_t> module_index;
  std::vector<MinidumpMemRange> memory_ranges;
  std::vector<CString> load_log;
};

// Class for opening minidumps
class MiniDumpReader {
 public:
  MiniDumpReader();
  ~MiniDumpReader();
  int Open(CString sFileName, CString sSymSearchPath);
  int StackWalk(DWORD dwThreadId);
  void Close();
  BOOL CheckDbgHelpApiVersion();
  int GetModuleRowIdByBaseAddr(DWORD64 dwBaseAddr);
  int GetModuleRowIdByAddress(DWORD64 dwAddress);
  int GetThreadRowIdByThreadId(DWORD dwThreadId);

  BOOL read_sys_info_stream() const { return read_sys_info_stream_; }
  BOOL read_exception_stream() const { return read_exception_stream_; }
  BOOL read_module_list_stream() const { return read_module_list_stream_; }
  BOOL read_memory_list_stream() const { return read_memory_list_stream_; }
  BOOL read_thread_list_stream() const { return read_thread_list_stream_; }
  const MinidumpData& dump_data() {return dump_data_;};

 private:
  // Helper function which extracts a UNICODE string from the minidump
  CString GetMinidumpString(LPVOID pStartAddr, RVA rva);

  int ReadSysInfoStream();
  int ReadExceptionStream();
  int ReadModuleListStream();
  int ReadMemoryListStream();
  int ReadThreadListStream();

 private:
  MinidumpData dump_data_;
   // Is minidump loaded?
  BOOL loaded_;
  //  has each stream been read? 
  BOOL read_sys_info_stream_;
  BOOL read_exception_stream_;
  BOOL read_module_list_stream_;
  BOOL read_memory_list_stream_;
  BOOL read_thread_list_stream_;
  CString file_name_;
   // The list of symbol search dirs passed.
  CString symbol_search_path_;
   // Handle to opened .DMP file
  HANDLE file_handle_;
  HANDLE handle_file_mapping_;
  LPVOID map_address_;

};

#endif
