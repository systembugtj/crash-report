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

#include "stdafx.h"
#include "resource.h"
#include "error_report_dlg.h"
#include "resend_dlg.h"
#include "crash_info_reader.h"
#include "base/strconv.h"
#include "base/utility.h"

CAppModule _Module;
CErrorReportDlg dlgErrorReport;
CResendDlg dlgResend;

int Run(LPTSTR /*lpstrCmdLine*/= NULL, int /*nCmdShow*/= SW_SHOWDEFAULT) {
  const wchar_t* szCommandLine = GetCommandLineW();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);

  // Read the crash info passed by crash_report.dll to crash_sender.exe 
  if (argc != 2)
    return 1; // No arguments passed

  // Read crash info
  CString sFileName = CString(argv[1]);
  int nInit = g_CrashInfo.Init(sFileName);
  if (nInit != 0) {
    MessageBox(NULL, _T("Couldn't initialize!"), _T("crash_sender.exe"),
        MB_ICONERROR);
    return 1;
  }

  if (!g_CrashInfo.m_bSendRecentReports) {
    // Do the crash info collection work assynchronously
    g_ErrorReportSender.DoWork(COLLECT_CRASH_INFO);
  }

  // Check window mirroring settings 
  CString sRTL = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
      "Settings"), _T("RTLReading"));
  if (sRTL.CompareNoCase(_T("1")) == 0) {
    SetProcessDefaultLayout( LAYOUT_RTL);
  }

  CMessageLoop theLoop;
  _Module.AddMessageLoop(&theLoop);

  if (!g_CrashInfo.m_bSendRecentReports) {
    if (dlgErrorReport.Create(NULL) == NULL) {
      ATLTRACE(_T("Main dialog creation failed!\n"));
      return 0;
    }
  } else {
    // check if another instance of crash_sender.exe is running
    ::CreateMutex(NULL, FALSE,
        _T("Local\\43773530-129a-4298-88f2-20eea3e4a59b"));
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
      // Another crash_sender.exe already tries to resend recent reports; exit.
      return 0;
    }

    if (g_CrashInfo.GetReportCount() == 0)
      return 0; // There are no reports for us to send

    // Check if it is ok to remind user now
    if (!g_CrashInfo.IsRemindNowOK())
      return 0;

    if (dlgResend.Create(NULL) == NULL) {
      ATLTRACE(_T("Resend dialog creation failed!\n"));
      return 0;
    }
  }

  int nRet = theLoop.Run();

  // Wait until the worker thread is exited  
  g_ErrorReportSender.WaitForCompletion();
  nRet = g_ErrorReportSender.GetGlobalStatus();

  _Module.RemoveMessageLoop();

  return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
  HRESULT hRes = ::CoInitialize(NULL);
  // If you are running on NT 4.0 or higher you can use the following call instead to
  // make the EXE free threaded. This means that calls come in on a random RPC thread.
  //	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  ATLASSERT(SUCCEEDED(hRes));

  // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
  ::DefWindowProc(NULL, 0, 0, 0L);

  AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES); // add flags to support other controls

  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  int nRet = Run(lpstrCmdLine, nCmdShow);

  _Module.Term();
  ::CoUninitialize();

  return nRet;
}
