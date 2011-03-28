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

// File: CrashHandler.h
// Description: Exception handling functionality.
// Authors: mikecarruth, zexspectrum
// Date: 

#ifndef _CRASHHANDLER_H_
#define _CRASHHANDLER_H_

#include "stdafx.h"
#include <signal.h>
#include <exception>
#include "crash_report.h"      
#include "base/utility.h"
#include "critical_section.h"
#include "shared_memory.h"

/* This structure contains pointer to the exception handlers for a thread.*/
struct ThreadExceptionHandlers
{
  ThreadExceptionHandlers()
  {
    m_prevTerm = NULL;
    m_prevUnexp = NULL;
    m_prevSigFPE = NULL;
    m_prevSigILL = NULL;
    m_prevSigSEGV = NULL;
  }

  terminate_handler m_prevTerm;        // Previous terminate handler   
  unexpected_handler m_prevUnexp;      // Previous unexpected handler
  void (__cdecl *m_prevSigFPE)(int);   // Previous FPE handler
  void (__cdecl *m_prevSigILL)(int);   // Previous SIGILL handler
  void (__cdecl *m_prevSigSEGV)(int);  // Previous illegal storage access handler
};

// Sets the last error message (for the caller thread).
int crSetErrorMsg(PTSTR pszErrorMsg);

struct FileItem
{
  CString m_sSrcFilePath; // Path to the original file. 
  CString m_sDstFileName; // Destination file name.
  CString m_sDescription; // Description.
  BOOL m_bMakeCopy;       // Should we make a copy of this file on crash?
};

class CCrashHandler  
{
public:
	
  // Default constructor.
  CCrashHandler();

  virtual ~CCrashHandler();

  int Init(
      LPCTSTR lpcszAppName = NULL,
      LPCTSTR lpcszAppVersion = NULL,
      LPCTSTR lpcszCrashSenderPath = NULL,
      LPGETLOGFILE lpfnCallback = NULL,           
      LPCTSTR lpcszUrl = NULL,
      UINT (*puPriorities)[5] = NULL,
      DWORD dwFlags = 0,
      LPCTSTR lpcszPrivacyPolicyURL = NULL,
      LPCTSTR lpcszDebugHelpDLLPath = NULL,
      MINIDUMP_TYPE MiniDumpType = MiniDumpNormal,
      LPCTSTR lpcszErrorReportSaveDir = NULL,
      LPCTSTR lpcszRestartCmdLine = NULL,
      LPCTSTR lpcszLangFilePath = NULL,
      LPCTSTR lpcszCustomSenderIcon = NULL);

  BOOL IsInitialized();
  int Destroy();
  int AddFile(LPCTSTR lpFile, LPCTSTR lpDestFile, LPCTSTR lpDesc, DWORD dwFlags);
  int AddProperty(CString sPropName, CString sPropValue);
  int AddScreenshot(DWORD dwFlags, int nJpegQuality);
  int AddRegKey(LPCTSTR szRegKey, LPCTSTR szDstFileName, DWORD dwFlags);
  int GenerateErrorReport(PCR_EXCEPTION_INFO pExceptionInfo = NULL);
     
  int SetProcessExceptionHandlers(DWORD dwFlags);
  int UnSetProcessExceptionHandlers();
  int SetThreadExceptionHandlers(DWORD dwFlags);   
  int UnSetThreadExceptionHandlers();
  static CCrashHandler* GetCurrentProcessCrashHandler();
  static void ReleaseCurrentProcessCrashHandler();

  /* Exception handler functions. */

  static LONG WINAPI SehHandler(PEXCEPTION_POINTERS pExceptionPtrs);
  static void __cdecl TerminateHandler();
  static void __cdecl UnexpectedHandler();

#if _MSC_VER>=1300
  static void __cdecl PureCallHandler();
#endif 

#if _MSC_VER>=1300 && _MSC_VER<1400
  static void __cdecl SecurityHandler(int code, void *x);
#endif

#if _MSC_VER>=1400
  static void __cdecl InvalidParameterHandler(const wchar_t* expression, 
    const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);
#endif

#if _MSC_VER>=1300
  static int __cdecl NewHandler(size_t);
#endif

  static void SigabrtHandler(int);
  static void SigfpeHandler(int /*code*/, int subcode);
  static void SigintHandler(int);
  static void SigillHandler(int);
  static void SigsegvHandler(int);
  static void SigtermHandler(int);

  /* Crash report generation methods */

  // Collects current process state.
  void GetExceptionPointers(
        DWORD dwExceptionCode, 
        EXCEPTION_POINTERS** pExceptionPointers);
    
  // Packs crash description into shared memory.
  CRASH_DESCRIPTION* PackCrashInfoIntoSharedMem(CSharedMem* pSharedMem, BOOL bTempMem);
  DWORD PackString(CString str);
  DWORD PackFileItem(FileItem& fi);
  DWORD PackProperty(CString sName, CString sValue);
  DWORD PackRegKey(CString sKeyName, CString sDstFileName);
    
  // Launches the crash_sender.exe process.
  int LaunchCrashSender(
        CString sCmdLineParams, 
        BOOL bWait, 
        HANDLE* phProcess);  
  
  // Sets internal pointers to exception handlers to NULL.
  void InitPrevExceptionHandlerPointers();

  // Acqure exclusive access to this crash handler.
  void CrashLock(BOOL bLock);

  /* Private member variables. */

  static CCrashHandler* m_pProcessCrashHandler; // Singleton of the CCrashHandler class.
  
  LPTOP_LEVEL_EXCEPTION_FILTER  m_oldSehHandler;  // previous SEH exception filter.
      
#if _MSC_VER>=1300
  _purecall_handler m_prevPurec;   // Previous pure virtual call exception filter.
  _PNH m_prevNewHandler; // Previous new operator exception filter.
#endif

#if _MSC_VER>=1400
  _invalid_parameter_handler m_prevInvpar; // Previous invalid parameter exception filter.
#endif

#if _MSC_VER>=1300 && _MSC_VER<1400
  _secerr_handler_func m_prevSec; // Previous security exception filter.
#endif

  void (__cdecl *m_prevSigABRT)(int); // Previous SIGABRT handler.  
  void (__cdecl *m_prevSigINT)(int);  // Previous SIGINT handler.
  void (__cdecl *m_prevSigTERM)(int); // Previous SIGTERM handler.

  // List of exception handlers installed for worker threads of current process.
  std::map<DWORD, ThreadExceptionHandlers> m_ThreadExceptionHandlers;
  CCritSec m_csThreadExceptionHandlers; // Synchronization lock for m_ThreadExceptionHandlers.

  BOOL m_bInitialized;           // Flag telling if this object was initialized.  
  CString m_sAppName;            // Application name.
  CString m_sAppVersion;         // Application version.  
  CString m_sCrashGUID;          // Crash GUID.
  CString m_sImageName;          // Process image name.
  DWORD m_dwFlags;               // Flags.
  MINIDUMP_TYPE m_MinidumpType;  // Minidump type.
  //BOOL m_bAppRestart;            // This is packed into dwFlags
  CString m_sRestartCmdLine;     // App restart command line.
  CString m_sUrl;                // Url to use when sending error report over HTTP.  
  UINT m_uPriorities[3];         // Delivery priorities.
  CString m_sPrivacyPolicyURL;   // Privacy policy URL.
  CString m_sPathToCrashSender;  // Path to crash_sender.exe
  CString m_sLangFileName;       // Language file.
  CString m_sPathToDebugHelpDll; // Path to dbghelp.dll.
  CString m_sUnsentCrashReportsFolder; // Path to the folder where to save error reports.
  LPGETLOGFILE m_lpfnCallback;   // Client crash callback.    
  BOOL m_bAddScreenshot;         // Should we add screenshot?
  DWORD m_dwScreenshotFlags;     // Screenshot flags.
  int m_nJpegQuality;
  CString m_sCustomSenderIcon;   // Resource name that can be used as custom Error Report dialog icon.
  std::map<CString, FileItem> m_files; // File items to include.
  std::map<CString, CString> m_props;  // User-defined properties to include.
  std::map<CString, CString> m_RegKeys; // Registry keys to dump.  
  CCritSec m_csCrashLock;        // Critical section used to synchronize thread access to this object. 
  HANDLE m_hEvent;               // Event used to synchronize crash_report.dll with crash_sender.exe.
  CSharedMem m_SharedMem;        // Shared memory.  
  CRASH_DESCRIPTION* m_pCrashDesc; // Pointer to crash description shared mem view.
  CSharedMem* m_pTmpSharedMem;        // Used temporary
  CRASH_DESCRIPTION* m_pTmpCrashDesc; // Used temporary
};



#endif	// !_CRASHHANDLER_H_

