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

// CrashRptTest.cpp : main source file for CrashRptTest.exe
//
// TODO(yesp) : 不要使用优先级，而是使用一个AddSendMethod的回调函数，以将
//  压缩后的文件发送出去，默认的发送函数有http,smtp,smapi三种，实现一个sender manager,
//  每个Sender有相同的抽象函数，该函数返回bool,以表示该方法使用发送成功。
//  这里不应该使用异步方式进行发送，而只是需要将发送窗口最小化到托盘即可。发送完毕后（成功或者失败），
//  退出发送程序。
#include "stdafx.h"
#include "resource.h"
#include "main_dlg.h"
#include "crash_thread.h"
#include <shellapi.h>

CAppModule _Module;
HANDLE g_hWorkingThread = NULL;
CrashThreadInfo g_CrashThreadInfo;

// Helper function that returns path to application directory
CString GetAppDir() {
	CString string;
	LPTSTR buf = string.GetBuffer(_MAX_PATH);
	GetModuleFileName(NULL, buf, _MAX_PATH);
	*(_tcsrchr(buf, '\\')) = 0; // remove executable name
	string.ReleaseBuffer();
	return string;
}

// Helper function that returns path to module
CString GetModulePath(HMODULE hModule) {
	CString string;
	LPTSTR buf = string.GetBuffer(_MAX_PATH);
	GetModuleFileName(hModule, buf, _MAX_PATH);
	TCHAR* ptr = _tcsrchr(buf, '\\');
	if (ptr != NULL)
		*(ptr) = 0; // remove executable name
	string.ReleaseBuffer();
	return string;
}

int Run(LPTSTR /*lpstrCmdLine*/= NULL, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	// Get command line params
	const wchar_t* szCommandLine = GetCommandLineW();
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);

	CMainDlg dlgMain;
	if (argc == 2 && wcscmp(argv[1], L"/restart") == 0)
		dlgMain.m_bRestarted = TRUE;
	else
		dlgMain.m_bRestarted = FALSE;

	if (dlgMain.Create(NULL) == NULL) {
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);
	int nRet = theLoop.Run();
	_Module.RemoveMessageLoop();
	return nRet;
}

BOOL WINAPI CrashCallback(LPVOID lpvState) {
	UNREFERENCED_PARAMETER(lpvState);
	// Crash happened!
	return TRUE;
}

bool InstallCrashReport(crash_report::CrAutoInstallHelper& helper) {
  // Install crash handlers.
  helper.set_send_method(CR_HTTP_MutilPart);
  helper.set_application_name("CrashRpt Tests");
  helper.set_application_version("1.2.7");
  helper.set_crash_server_url("http://localhost:8080/crashrpt.php");
  helper.set_crash_callback(CrashCallback);
  helper.set_flags(CR_INST_ALL_EXCEPTION_HANDLERS |
    CR_INST_APP_RESTART|CR_INST_SEND_QUEUED_REPORTS );
  helper.set_minidump_type(MiniDumpNormal);
  helper.set_privacy_policy_url("http://code.google.com/p/crashrpt/wiki/PrivacyPolicyTemplate");
  helper.set_restart_cmd("/restart");
  if(helper.Install()!=0) {
    MessageBoxA(NULL, "fail to install crash_report!\n", "error", 0);
    char reason[1000];
    crGetLastErrorMsgA(reason, 1000);
    MessageBoxA(NULL, reason, "reason", 0);
    return false;
  }
  CString sLogFile = GetAppDir() + _T("\\dummy.log");
  CString sIniFile = GetAppDir() + _T("\\dummy.ini");
  helper.AddFile(sLogFile, NULL, _T("Dummy Log File"), CR_AF_MAKE_FILE_COPY);
  helper.AddFile(sIniFile, _T("Dummy INI File"));
  helper.AddScreenshot(CR_AS_PROCESS_WINDOWS|CR_AS_USE_JPEG_FORMAT, 10);
  helper.AddScreenshot(CR_AS_MAIN_WINDOW);
  helper.AddProperty(_T("VideoCard"),_T("nVidia GeForce 9800"));
  helper.AddRegKey(_T("HKEY_CURRENT_USER\\Software\\Microsoft\\"
    "Windows\\CurrentVersion\\Explorer"), _T("regkey.xml"), 0);
  helper.AddRegKey(_T("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\"
    "Windows\\CurrentVersion\\Internet Settings"), _T("regkey.xml"), 0);
  return true;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));
  
  //  this object must in main/ WinMain
  crash_report::CrAutoInstallHelper helper;
  if (!InstallCrashReport(helper)) {
    return -1;
  }

	/* Create another thread */
  g_CrashThreadInfo.m_bStop = false;
  g_CrashThreadInfo.m_hWakeUpEvent = CreateEvent(NULL, FALSE, FALSE, _T("WakeUpEvent"));
  ATLASSERT(g_CrashThreadInfo.m_hWakeUpEvent!=NULL);

  DWORD dwThreadId = 0;
  g_hWorkingThread = CreateThread(NULL, 0, CrashThread, (LPVOID)&g_CrashThreadInfo, 0, &dwThreadId);
  ATLASSERT(g_hWorkingThread!=NULL);

  // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
  ::DefWindowProc(NULL, 0, 0, 0L);
  AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls
  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  int nRet = Run(lpstrCmdLine, nCmdShow);
  _Module.Term();

  // Close another thread
  g_CrashThreadInfo.m_bStop = true;
  SetEvent(g_CrashThreadInfo.m_hWakeUpEvent);
  // Wait until thread terminates
  WaitForSingleObject(g_hWorkingThread, INFINITE);
  ::CoUninitialize();

  return nRet;
}
