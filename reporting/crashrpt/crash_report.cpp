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

// File: CrashRpt.cpp
// Description: CrashRpt API implementation.
// Authors: mikecarruth, zexspectrum
// Date: 

#include "stdafx.h"
#include "crash_report.h"
#include "crash_handler.h"
#include "utility.h"
#include "strconv.h"

HANDLE g_hModuleCrashRpt = NULL; // Handle to CrashRpt.dll module.
CComAutoCriticalSection g_cs;    // Critical section for thread-safe accessing error messages.
std::map<DWORD, CString> g_sErrorMsg; // Last error messages for each calling thread.

CRASHRPTAPI(LPVOID) InstallW(LPGETLOGFILE pfnCallback)
{
  CR_INSTALL_INFOW info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.size = sizeof(CR_INSTALL_INFO);
  info.crash_callback = pfnCallback;
  crInstallW(&info);

  return NULL;
}

CRASHRPTAPI(LPVOID) InstallA(LPGETLOGFILE pfnCallback)
{
  strconv_t strconv;
  return InstallW(pfnCallback);
}

CRASHRPTAPI(void) Uninstall(LPVOID lpState)
{
  lpState;
  crUninstall();  
}

CRASHRPTAPI(void) AddFileW(LPVOID lpState, const wchar_t* lpFile, const wchar_t* lpDesc)
{ 
  lpState;
  crAddFileW(lpFile, lpDesc);
}

CRASHRPTAPI(void) AddFileA(LPVOID lpState, const char* lpFile, const char* lpDesc)
{
  strconv_t strconv;
  const wchar_t* lpwszFile = strconv.a2w(lpFile);
  const wchar_t* lpwszDesc = strconv.a2w(lpDesc);
  AddFileW(lpState, lpwszFile, lpwszDesc);
}

CRASHRPTAPI(void) GenerateErrorReport(LPVOID lpState, PEXCEPTION_POINTERS pExInfo)
{
  lpState;

  CR_EXCEPTION_INFO ei;
  memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
  ei.cb = sizeof(CR_EXCEPTION_INFO);
  ei.exctype = CR_SEH_EXCEPTION;
  ei.pexcptrs = pExInfo;
  
  crGenerateErrorReport(&ei);
}

CRASHRPTAPI(int) crInstallW(CR_INSTALL_INFOW* pInfo)
{
  int nStatus = -1;
  crSetErrorMsg(_T("Success."));
  strconv_t strconv;
  CCrashHandler *pCrashHandler = NULL;

  // Validate input parameters.
  if(pInfo==NULL || 
     pInfo->size!=sizeof(CR_INSTALL_INFOW))     
  {     
    crSetErrorMsg(_T("pInfo is NULL or pInfo->size member is not valid."));
    nStatus = 1;
    goto cleanup;
  }

  // Check if crInstall() already was called for current process.
  pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler!=NULL &&
     pCrashHandler->IsInitialized())
  {    
    crSetErrorMsg(_T("Can't install crash handler to the same process twice."));
    nStatus = 2; 
    goto cleanup;
  }
  
  if(pCrashHandler==NULL)
  {
    pCrashHandler = new CCrashHandler();
    if(pCrashHandler==NULL)
    {    
      crSetErrorMsg(_T("Error allocating memory for crash handler."));
      nStatus = 3; 
      goto cleanup;
    }
  }
  
  LPCTSTR ptszAppName = strconv.w2t((LPWSTR)pInfo->application_name);
  LPCTSTR ptszAppVersion = strconv.w2t((LPWSTR)pInfo->application_version);
  LPCTSTR ptszCrashSenderPath = strconv.w2t((LPWSTR)pInfo->sender_path);
  LPCTSTR ptszUrl = strconv.w2t((LPWSTR)pInfo->crash_server_url);
  LPCTSTR ptszPrivacyPolicyURL = strconv.w2t((LPWSTR)pInfo->privacy_policy_url);
  LPCTSTR ptszDebugHelpDLL_file = strconv.w2t((LPWSTR)pInfo->debug_help_dll);
  MINIDUMP_TYPE miniDumpType = pInfo->minidump_type;
  LPCTSTR ptszErrorReportSaveDir = strconv.w2t((LPWSTR)pInfo->save_dir);
  LPCTSTR ptszRestartCmdLine = strconv.w2t((LPWSTR)pInfo->restart_cmd);
  LPCTSTR ptszLangFilePath = strconv.w2t((LPWSTR)pInfo->langpack_path);
  LPCTSTR ptszCustomSenderIcon = strconv.w2t((LPWSTR)pInfo->custom_sender_icon);

  int nInitResult = pCrashHandler->Init(
    ptszAppName, 
    ptszAppVersion, 
    ptszCrashSenderPath,
    pInfo->crash_callback,
    ptszUrl,
    &pInfo->priorities,
    pInfo->flags,
    ptszPrivacyPolicyURL,
    ptszDebugHelpDLL_file,
    miniDumpType,
    ptszErrorReportSaveDir,
    ptszRestartCmdLine,
    ptszLangFilePath,
    ptszCustomSenderIcon
    );
  
  if(nInitResult!=0)
  {    
    nStatus = 4;
    goto cleanup;
  }

  // OK.
  nStatus = 0;

cleanup:
  
  if(nStatus!=0) // If failed
  {
    if(pCrashHandler!=NULL && 
       !pCrashHandler->IsInitialized())
    {
      // Release crash handler object
      CCrashHandler::ReleaseCurrentProcessCrashHandler();
    }
  }

  return nStatus;
}

CRASHRPTAPI(int) crInstallA(CR_INSTALL_INFOA* pInfo)
{
  if(pInfo==NULL)
    return crInstallW((CR_INSTALL_INFOW*)NULL);

  // Convert pInfo members to wide char

  strconv_t strconv;
  
  CR_INSTALL_INFOW ii;
  memset(&ii, 0, sizeof(CR_INSTALL_INFOW));
  ii.size = sizeof(CR_INSTALL_INFOW);
  ii.crash_callback = pInfo->crash_callback;
  ii.application_name = strconv.a2w(pInfo->application_name);
  ii.application_version = strconv.a2w(pInfo->application_version);
  ii.sender_path = strconv.a2w(pInfo->sender_path);
  ii.crash_server_url = strconv.a2w(pInfo->crash_server_url);
  memcpy(&ii.priorities, pInfo->priorities, 3*sizeof(UINT));
  ii.flags = pInfo->flags;
  ii.privacy_policy_url = strconv.a2w(pInfo->privacy_policy_url);
  ii.debug_help_dll = strconv.a2w(pInfo->debug_help_dll);
  ii.minidump_type = pInfo->minidump_type;
  ii.save_dir = strconv.a2w(pInfo->save_dir);
  ii.restart_cmd = strconv.a2w(pInfo->restart_cmd);
  ii.langpack_path = strconv.a2w(pInfo->langpack_path);
  return crInstallW(&ii);
}

CRASHRPTAPI(int) crUninstall()
{
  crSetErrorMsg(_T("Success."));

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();
  
  if(pCrashHandler==NULL ||
     !pCrashHandler->IsInitialized())
  {     
    crSetErrorMsg(_T("Crash handler wasn't preiviously installed for this process."));
    return 1; 
  }

  // Uninstall main thread's C++ exception handlers
  int nUnset = pCrashHandler->UnSetThreadExceptionHandlers();
  if(nUnset!=0)
    return 2;

  int nDestroy = pCrashHandler->Destroy();
  if(nDestroy!=0)
    return 3;

  delete pCrashHandler;
    
  return 0;
}

// Sets C++ exception handlers for the calling thread
CRASHRPTAPI(int) 
crInstallToCurrentThread2(DWORD dwFlags)
{
  crSetErrorMsg(_T("Success."));

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();
  
  if(pCrashHandler==NULL)
  {    
    crSetErrorMsg(_T("Crash handler was already installed for current thread."));
    return 1; 
  }

  int nResult = pCrashHandler->SetThreadExceptionHandlers(dwFlags);
  if(nResult!=0)
    return 2; // Error?

  // Ok.
  return 0;
}

// Unsets C++ exception handlers from the calling thread
CRASHRPTAPI(int) 
crUninstallFromCurrentThread()
{
  crSetErrorMsg(_T("Success."));

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {
    ATLASSERT(pCrashHandler!=NULL);
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current thread."));
    return 1; // Invalid parameter?
  }

  int nResult = pCrashHandler->UnSetThreadExceptionHandlers();
  if(nResult!=0)
    return 2; // Error?

  // OK.
  return 0;
}

CRASHRPTAPI(int) 
crInstallToCurrentThread()
{
  return crInstallToCurrentThread2(0);
}

CRASHRPTAPI(int) 
crAddFileW(PCWSTR pszFile, PCWSTR pszDesc)
{
  return crAddFile2W(pszFile, NULL, pszDesc, 0);
}

CRASHRPTAPI(int) 
crAddFileA(PCSTR pszFile, PCSTR pszDesc)
{
  return crAddFile2A(pszFile, NULL, pszDesc, 0);
}

CRASHRPTAPI(int) 
crAddFile2W(PCWSTR pszFile, PCWSTR pszDestFile, PCWSTR pszDesc, DWORD dwFlags)
{
  crSetErrorMsg(_T("Success."));

  strconv_t strconv;

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current process."));
    return 1; // No handler installed for current process?
  }
  
  LPCTSTR lptszFile = strconv.w2t((LPWSTR)pszFile);
  LPCTSTR lptszDestFile = strconv.w2t((LPWSTR)pszDestFile);
  LPCTSTR lptszDesc = strconv.w2t((LPWSTR)pszDesc);

  int nAddResult = pCrashHandler->AddFile(lptszFile, lptszDestFile, lptszDesc, dwFlags);
  if(nAddResult!=0)
  {    
    // Couldn't add file
    return 2; 
  }

  // OK.
  return 0;
}

CRASHRPTAPI(int) 
crAddFile2A(PCSTR pszFile, PCSTR pszDestFile, PCSTR pszDesc, DWORD dwFlags)
{
  // Convert parameters to wide char

  strconv_t strconv;

  const wchar_t* pwszFile = strconv.a2w(pszFile);
  const wchar_t* pwszDestFile = strconv.a2w(pszDestFile);
  const wchar_t* pwszDesc = strconv.a2w(pszDesc);    
  
  return crAddFile2W(pwszFile, pwszDestFile, pwszDesc, dwFlags);
}

CRASHRPTAPI(int) 
crAddScreenshot(
   DWORD dwFlags
   )
{
  return crAddScreenshot2(dwFlags, 95);
}

CRASHRPTAPI(int)
crAddScreenshot2(
   DWORD dwFlags,
   int nJpegQuality
   )
{
  crSetErrorMsg(_T("Unspecified error."));
  
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current thread."));
    return 1; // Invalid parameter?
  }

  return pCrashHandler->AddScreenshot(dwFlags, nJpegQuality);
}

CRASHRPTAPI(int)
crAddPropertyW(
   const wchar_t* pszPropName,
   const wchar_t* pszPropValue
   )
{
  crSetErrorMsg(_T("Unspecified error."));

  strconv_t strconv;
  LPCTSTR pszPropNameT = strconv.w2t(pszPropName);
  LPCTSTR pszPropValueT = strconv.w2t(pszPropValue);

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {   
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current process."));
    return 1; // No handler installed for current process?
  }

  int nResult = pCrashHandler->AddProperty(CString(pszPropNameT), CString(pszPropValueT));
  if(nResult!=0)
  {    
    crSetErrorMsg(_T("Invalid property name specified."));
    return 2; // Failed to add the property
  }
  
  crSetErrorMsg(_T("Success."));
  return 0;
}

CRASHRPTAPI(int)
crAddPropertyA(
   const char* pszPropName,
   const char* pszPropValue
   )
{
  // This is just a wrapper for wide-char function version
  strconv_t strconv;
  return crAddPropertyW(strconv.a2w(pszPropName), strconv.a2w(pszPropValue));
}

CRASHRPTAPI(int)
crAddRegKeyW(   
   const wchar_t* pszRegKey,
   const wchar_t* pszDstFileName,
   DWORD dwFlags
   )
{
  crSetErrorMsg(_T("Unspecified error."));

  strconv_t strconv;
  LPCTSTR pszRegKeyT = strconv.w2t(pszRegKey);
  LPCTSTR pszDstFileNameT = strconv.w2t(pszDstFileName);  

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current process."));
    return 1; // No handler installed for current process?
  }

  int nResult = pCrashHandler->AddRegKey(pszRegKeyT, pszDstFileNameT, dwFlags);
  if(nResult!=0)
  {    
    crSetErrorMsg(_T("Invalid parameter or key doesn't exist."));
    return 2; // Failed to add the property
  }
  
  crSetErrorMsg(_T("Success."));
  return 0;
}

CRASHRPTAPI(int)
crAddRegKeyA(   
   const char* pszRegKey,
   const char* pszDstFileName,
   DWORD dwFlags
   )
{
  // This is just a wrapper for wide-char function version
  strconv_t strconv;
  return crAddRegKeyW(strconv.a2w(pszRegKey), strconv.a2w(pszDstFileName), dwFlags);
}

CRASHRPTAPI(int) 
crGenerateErrorReport(
  CR_EXCEPTION_INFO* pExceptionInfo)
{
  crSetErrorMsg(_T("Unspecified error."));

  if(pExceptionInfo==NULL || 
     pExceptionInfo->cb!=sizeof(CR_EXCEPTION_INFO))
  {
    crSetErrorMsg(_T("Exception info is NULL or invalid."));    
    return 1;
  }

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    // Handler is not installed for current process 
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current process."));
    ATLASSERT(pCrashHandler!=NULL);
    return 2;
  } 

  return pCrashHandler->GenerateErrorReport(pExceptionInfo);  
}

CRASHRPTAPI(int) 
crGetLastErrorMsgW(LPWSTR pszBuffer, UINT uBuffSize)
{
  if(pszBuffer==NULL || uBuffSize==0)
    return -1; // Null pointer to buffer

  strconv_t strconv;

  g_cs.Lock();

  DWORD dwThreadId = GetCurrentThreadId();
  std::map<DWORD, CString>::iterator it = g_sErrorMsg.find(dwThreadId);

  if(it==g_sErrorMsg.end())
  {
    // No error message for current thread.
    CString sErrorMsg = _T("No error.");
	  const wchar_t* pwszErrorMsg = strconv.t2w(sErrorMsg.GetBuffer(0));
    int size =  min(sErrorMsg.GetLength(), (int)uBuffSize-1);
	  WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, size);    
    g_cs.Unlock();
    return size;
  }
  
  const wchar_t* pwszErrorMsg = strconv.t2w(it->second.GetBuffer(0));
  int size = min((int)wcslen(pwszErrorMsg), (int)uBuffSize-1);
  WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, size);
  pszBuffer[uBuffSize-1] = 0; // Zero terminator  
  g_cs.Unlock();
  return size;
}

CRASHRPTAPI(int) 
crGetLastErrorMsgA(LPSTR pszBuffer, UINT uBuffSize)
{  
  if(pszBuffer==NULL || uBuffSize==0)
    return -1;

  strconv_t strconv;

  WCHAR* pwszBuffer = new WCHAR[uBuffSize];
    
  int res = crGetLastErrorMsgW(pwszBuffer, uBuffSize);
  
  const char* paszBuffer = strconv.w2a(pwszBuffer);  

  STRCPY_S(pszBuffer, uBuffSize, paszBuffer);

  delete [] pwszBuffer;

  return res;
}

int crSetErrorMsg(PTSTR pszErrorMsg)
{  
  g_cs.Lock();
  DWORD dwThreadId = GetCurrentThreadId();
  g_sErrorMsg[dwThreadId] = pszErrorMsg;
  g_cs.Unlock();
  return 0;
}


CRASHRPTAPI(int) 
crExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
  crSetErrorMsg(_T("Unspecified error."));

  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current process."));
    return EXCEPTION_CONTINUE_SEARCH; 
  }

  CR_EXCEPTION_INFO ei;
  memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
  ei.cb = sizeof(CR_EXCEPTION_INFO);  
  ei.exctype = CR_SEH_EXCEPTION;
  ei.pexcptrs = ep;
  ei.code = code;

  int res = pCrashHandler->GenerateErrorReport(&ei);
  if(res!=0)
  {
    // If goes here than GenerateErrorReport() failed  
    return EXCEPTION_CONTINUE_SEARCH;  
  }  
  
  crSetErrorMsg(_T("Success."));
  return EXCEPTION_EXECUTE_HANDLER;  
}

//-----------------------------------------------------------------------------------------------
// Below crEmulateCrash() related stuff goes 


class CDerived;
class CBase
{
public:
   CBase(CDerived *derived): m_pDerived(derived) {};
   ~CBase();
   virtual void function(void) = 0;

   CDerived * m_pDerived;
};

class CDerived : public CBase
{
public:
   CDerived() : CBase(this) {};   // C4355
   virtual void function(void) {};
};

CBase::~CBase()
{
   m_pDerived -> function();
}

#include <float.h>
void sigfpe_test()
{ 
  // Code taken from http://www.devx.com/cplus/Article/34993/1954

  //Set the x86 floating-point control word according to what
  //exceptions you want to trap. 
  _clearfp(); //Always call _clearfp before setting the control
              //word
  //Because the second parameter in the following call is 0, it
  //only returns the floating-point control word
  unsigned int cw; 
#if _MSC_VER<1400
  cw = _controlfp(0, 0); //Get the default control
#else
  _controlfp_s(&cw, 0, 0); //Get the default control
#endif 
                                      //word
  //Set the exception masks off for exceptions that you want to
  //trap.  When a mask bit is set, the corresponding floating-point
  //exception is //blocked from being generating.
  cw &=~(EM_OVERFLOW|EM_UNDERFLOW|EM_ZERODIVIDE|
         EM_DENORMAL|EM_INVALID);
  //For any bit in the second parameter (mask) that is 1, the 
  //corresponding bit in the first parameter is used to update
  //the control word.  
  unsigned int cwOriginal;
#if _MSC_VER<1400
  cwOriginal = _controlfp(cw, MCW_EM); //Set it.
#else
  _controlfp_s(&cwOriginal, cw, MCW_EM); //Set it.
#endif
                              //MCW_EM is defined in float.h.
                              //Restore the original value when done:
                              //_controlfp(cwOriginal, MCW_EM);

  // Divide by zero

  float a = 1;
  float b = 0;
  float c = a/b;
  c; 
}

#define BIG_NUMBER 0x1fffffff
#pragma warning(disable: 4717) // avoid C4717 warning
int RecurseAlloc() 
{
   int *pi = new int[BIG_NUMBER];
   pi;
   RecurseAlloc();
   return 0;
}

// Vulnerable function
#pragma warning(disable : 4996)   // for strcpy use
void test_buffer_overrun(const char *str) 
{
   char* buffer = (char*)_alloca(10);
   strcpy(buffer, str); // overrun buffer !!!

   // use a secure CRT function to help prevent buffer overruns
   // truncate string to fit a 10 byte buffer
   // strncpy_s(buffer, _countof(buffer), str, _TRUNCATE);
}
#pragma warning(default : 4996)  


CRASHRPTAPI(int) 
crEmulateCrash(unsigned ExceptionType) throw (...)
{
  crSetErrorMsg(_T("Unspecified error."));

  switch(ExceptionType)
  {
  case CR_SEH_EXCEPTION:
    {
      // Access violation
      int *p = 0;
      *p = 0;
    }
    break;
  case CR_CPP_TERMINATE_CALL:
    {
      // Call terminate
      terminate();
    }
    break;
  case CR_CPP_UNEXPECTED_CALL:
    {
      // Call unexpected
      unexpected();
    }
    break;
  case CR_CPP_PURE_CALL:
    {
      // pure virtual method call
      CDerived derived;
    }
    break;
  case CR_CPP_SECURITY_ERROR:
    {
      // Cause buffer overrun (/GS compiler option)

      // declare buffer that is bigger than expected
      char large_buffer[] = "This string is longer than 10 characters!!";
      test_buffer_overrun(large_buffer);
    }
    break;
  case CR_CPP_INVALID_PARAMETER:
    {      
      char* formatString;
      // Call printf_s with invalid parameters.
      formatString = NULL;
      printf(formatString);
    }
    break;
  case CR_CPP_NEW_OPERATOR_ERROR:
    {
      // Cause memory allocation error
      RecurseAlloc();
    }
    break;
  case CR_CPP_SIGABRT: 
    {
      // Call abort
      abort();
    }
    break;
  case CR_CPP_SIGFPE:
    {
      // floating point exception ( /fp:except compiler option)
      sigfpe_test();
      return 1;
    }    
  case CR_CPP_SIGILL: 
    {
      int result = raise(SIGILL);  
      ATLASSERT(result==0);
      crSetErrorMsg(_T("Error raising SIGILL."));
      return result;
    }    
  case CR_CPP_SIGINT: 
    {
      int result = raise(SIGINT);  
      ATLASSERT(result==0);
      crSetErrorMsg(_T("Error raising SIGINT."));
      return result;
    }    
  case CR_CPP_SIGSEGV: 
    {
      int result = raise(SIGSEGV);  
      ATLASSERT(result==0);
      crSetErrorMsg(_T("Error raising SIGSEGV."));
      return result;
    }    
  case CR_CPP_SIGTERM: 
    {
     int result = raise(SIGTERM);  
     crSetErrorMsg(_T("Error raising SIGTERM."));
	   ATLASSERT(result==0);     
     return result;
    }
  case CR_NONCONTINUABLE_EXCEPTION: 
    {
      // Raise noncontinuable software exception
      RaiseException(123, EXCEPTION_NONCONTINUABLE, 0, NULL);        
    }
    break;
  case CR_THROW: 
    {
      // Throw typed C++ exception.
      throw 13;
    }
    break;
  default:
    {
      crSetErrorMsg(_T("Unknown exception type specified."));          
    }
    break;
  }
 
  return 1;
}

#ifndef CRASHRPT_LIB
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if(dwReason==DLL_PROCESS_ATTACH)
  {
    // Save handle to the CrashRpt.dll module.
    g_hModuleCrashRpt = hModule;
  }
  return TRUE;
}
#endif

