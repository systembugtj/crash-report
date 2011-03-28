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
#include "error_report_sender.h"
#include "http_request_sender.h"
#include "crash_report.h"
#include "base/md5.h"
#include "base/utility.h"
#include "zip.h"
#include "crash_info_reader.h"
#include "base/strconv.h"
#include "screen_capure.h"
#include "base/base64.h"
#include <sys/stat.h>
#include "dbghelp.h"

// Globally accessible object
CErrorReportSender g_ErrorReportSender;

CErrorReportSender::CErrorReportSender() {
  m_nGlobalStatus = 0;
  m_hThread = NULL;
  m_SendAttempt = 0;
  m_Action = COLLECT_CRASH_INFO;
  m_bExport = FALSE;
  m_nCurReport = 0;
}

CErrorReportSender::~CErrorReportSender() {
}

int CErrorReportSender::GetGlobalStatus() {
  return m_nGlobalStatus;
}

int CErrorReportSender::GetCurReport() {
  return m_nCurReport;
}

BOOL CErrorReportSender::SetCurReport(int nCurReport) {
  if (nCurReport < 0 || nCurReport >= g_CrashInfo.GetReportCount()) {
    ATLASSERT(0);
    return FALSE;
  }
  m_nCurReport = nCurReport;
  return TRUE;
}

// This method does crash files collection and
// error report sending work
BOOL CErrorReportSender::DoWork(int action) {
  m_Action = action;

  // Create worker thread which will do all work assynchroniously
  m_hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID) this, 0, NULL);
  if (m_hThread == NULL)
    return FALSE;

  return TRUE;
}

// This method is the worker thread procedure that delegates further work 
// back to the CErrorReportSender class
DWORD WINAPI CErrorReportSender::WorkerThread(LPVOID lpParam) {
  CErrorReportSender* pSender = (CErrorReportSender*) lpParam;
  pSender->DoWorkAssync();

  return 0;
}

void CErrorReportSender::UnblockParentProcess() {
  // Notify the parent process that we have finished with minidump,
  // so the parent process is able to unblock and terminate itself.
  CString sEventName;

  sEventName.Format(_T("Local\\CrashRptEvent_%s"),
      g_CrashInfo.GetReport(0).m_sCrashGUID);
  HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, sEventName);
  if (hEvent != NULL)
    SetEvent(hEvent);
}

// This method collects required crash files (minidump, screenshot etc.)
// and then sends the error report over the Internet.
void CErrorReportSender::DoWorkAssync() {
  m_Assync.Reset();

  if (g_CrashInfo.m_bSendRecentReports) {
    CString sMsg;
    sMsg.Format(_T(">>> Performing actions with error report: '%s'"),
                g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName);
    m_Assync.SetProgress(sMsg, 0, false);
  }

  if (m_Action & COLLECT_CRASH_INFO) {
    TakeDesktopScreenshot();
    if (m_Assync.IsCancelled()) {
      UnblockParentProcess();
      Utility::RecycleFile(
          g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName, true);
      return;
    }

    CreateMiniDump();

    if (m_Assync.IsCancelled()) {
      UnblockParentProcess();
      Utility::RecycleFile(g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName, true);
      return;
    }

    // Notify the parent process that we have finished with minidump,
    // so the parent process is able to unblock and terminate itself.
    UnblockParentProcess();

    // Copy user-provided files.
    CollectCrashFiles();
    if (m_Assync.IsCancelled()) {
      Utility::RecycleFile(
          g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName, true);
      return;
    }

    m_Assync.SetProgress(_T("[confirm_send_report]"), 100, false);
  }

  if (m_Action & COMPRESS_REPORT) {
    BOOL bCompress = CompressReportFiles(g_CrashInfo.GetReport(m_nCurReport));
    if (!bCompress) {
      m_Assync.SetProgress(_T("[status_failed]"), 100, false);
      return;
    }
  }

  if (m_Action & RESTART_APP) {
    RestartApp();
  }
  if (m_Action & SEND_REPORT) {
    SendReport();
  }

  return;
}

void CErrorReportSender::SetExportFlag(BOOL bExport, CString sExportFile) {
  m_bExport = bExport;
  m_sExportFileName = sExportFile;
}

// This method blocks until worker thread is exited
void CErrorReportSender::WaitForCompletion() {
  WaitForSingleObject(m_hThread, INFINITE);
}

void CErrorReportSender::GetStatus(int& nProgressPct,
    std::vector<CString>& msg_log) {
  m_Assync.GetProgress(nProgressPct, msg_log);
}

void CErrorReportSender::Cancel() {
  m_Assync.Cancel();
}

void CErrorReportSender::FeedbackReady(int code) {
  m_Assync.FeedbackReady(code);
}

// This takes the desktop screenshot (screenshot of entire virtual screen
// or screenshot of the main window). 
BOOL CErrorReportSender::TakeDesktopScreenshot() {
  CScreenCapture sc;
  ScreenshotInfo ssi;
  std::vector<CString> screenshot_names;

//  m_Assync.SetProgress(_T("[taking_screenshot]"), 0);

  if (!g_CrashInfo.m_bAddScreenshot) {
    return TRUE;
  }

  DWORD dwFlags = g_CrashInfo.m_dwScreenshotFlags;
  SCREENSHOT_IMAGE_FORMAT fmt = SCREENSHOT_FORMAT_PNG;
  if ((dwFlags & CR_AS_USE_JPEG_FORMAT) != 0)
    fmt = SCREENSHOT_FORMAT_JPG;

  BOOL bGrayscale = (dwFlags & CR_AS_GRAYSCALE_IMAGE) != 0;
  std::vector<CRect> wnd_list;

  if ((dwFlags & CR_AS_MAIN_WINDOW) != 0) {
    // Take screenshot of the main window
    std::vector<WindowInfo> aWindows;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
        g_CrashInfo.m_dwProcessId);
    if (hProcess != NULL) {
      sc.FindWindows(hProcess, FALSE, &aWindows);
      CloseHandle(hProcess);
    }
    if (aWindows.size() > 0) {
      wnd_list.push_back(aWindows[0].m_rcWnd);
      ssi.m_aWindows.push_back(aWindows[0]);
    }
  } else if ((dwFlags & CR_AS_PROCESS_WINDOWS) != 0) {
    // Take screenshot of the main window    
    std::vector<WindowInfo> aWindows;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
        g_CrashInfo.m_dwProcessId);
    if (hProcess != NULL) {
      sc.FindWindows(hProcess, TRUE, &aWindows);
      CloseHandle(hProcess);
    }

    int i;
    for (i = 0; i < (int) aWindows.size(); i++)
      wnd_list.push_back(aWindows[i].m_rcWnd);
    ssi.m_aWindows = aWindows;
  } else // (dwFlags&CR_AS_VIRTUAL_SCREEN)!=0
  {
    // Take screenshot of the entire desktop
    CRect rcScreen;
    sc.GetScreenRect(&rcScreen);
    wnd_list.push_back(rcScreen);
  }

  ssi.m_bValid = TRUE;
  sc.GetScreenRect(&ssi.m_rcVirtualScreen);

  BOOL bTakeScreenshot = sc.CaptureScreenRect(wnd_list, g_CrashInfo.GetReport(
      m_nCurReport).m_sErrorReportDirName, 0, fmt, g_CrashInfo.m_nJpegQuality,
      bGrayscale, ssi.m_aMonitors, screenshot_names);
  if (bTakeScreenshot == FALSE) {
    return FALSE;
  }

  g_CrashInfo.GetReport(0).m_ScreenshotInfo = ssi;

  // Prepare the list of screenshot files we will add to the error report
  std::vector<ERIFileItem> FilesToAdd;
  size_t i;
  for (i = 0; i < screenshot_names.size(); i++) {
    CString sDestFile;
    int nSlashPos = screenshot_names[i].ReverseFind('\\');
    sDestFile = screenshot_names[i].Mid(nSlashPos + 1);
    ERIFileItem fi;
    fi.m_sSrcFile = screenshot_names[i];
    fi.m_sDestFile = sDestFile;
    fi.m_sDesc = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
        "DetailDlg"), _T("DescScreenshot"));
    g_CrashInfo.GetReport(0).m_FileItems[fi.m_sDestFile] = fi;
  }

  // Done
  return TRUE;
}

// This callbask function is called by MinidumpWriteDump
BOOL CALLBACK CErrorReportSender::MiniDumpCallback(PVOID CallbackParam,
    PMINIDUMP_CALLBACK_INPUT CallbackInput,
    PMINIDUMP_CALLBACK_OUTPUT CallbackOutput) {
  // Delegate back to the CErrorReportSender
  CErrorReportSender* pErrorReportSender = (CErrorReportSender*) CallbackParam;
  return pErrorReportSender->OnMinidumpProgress(CallbackInput, CallbackOutput);
}

// This method is called when MinidumpWriteDump notifies us about
// currently performed action
BOOL CErrorReportSender::OnMinidumpProgress(
    const PMINIDUMP_CALLBACK_INPUT CallbackInput,
    PMINIDUMP_CALLBACK_OUTPUT CallbackOutput) {
  switch (CallbackInput->CallbackType) {
  case CancelCallback: {
    // This callback allows to cancel minidump generation
    if (m_Assync.IsCancelled()) {
      CallbackOutput->Cancel = TRUE;
    }
  }
    break;

  case ModuleCallback: {
  }
    break;
  case ThreadCallback: {
  }
    break;
  }
  return TRUE;
}

// This method creates minidump of the process
// TODO(yesp) : 应该好好研究这个函数。
BOOL CErrorReportSender::CreateMiniDump() {
  BOOL bStatus = FALSE;
  HMODULE hDbgHelp = NULL;
  HANDLE hFile = NULL;
  MINIDUMP_EXCEPTION_INFORMATION mei;
  MINIDUMP_CALLBACK_INFORMATION mci;
  CString sMinidumpFile =
      g_CrashInfo.GetReport(m_nCurReport). m_sErrorReportDirName + _T(
          "\\crashdump.dmp");
  std::vector<ERIFileItem> files_to_add;
  ERIFileItem fi;
  CString sErrorMsg;

  if (g_CrashInfo.m_bGenerateMinidump == FALSE) {
    return TRUE;
  }

  m_Assync.SetProgress(_T("[creating_dump]"), 0, false);

  // Load dbghelp.dll
  hDbgHelp = LoadLibrary(g_CrashInfo.m_sDbgHelpPath);
  if (hDbgHelp == NULL) {
    //try again ... fallback to dbghelp.dll in path
    const CString sDebugHelpDLL_name = "dbghelp.dll";
    hDbgHelp = LoadLibrary(sDebugHelpDLL_name);
  }

  if (hDbgHelp == NULL) {
    goto cleanup;
  }

  // Create the minidump file
  hFile = CreateFile(sMinidumpFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD dwError = GetLastError();
    CString sMsg;
    sMsg.Format(_T("Couldn't create minidump file: %s"),
                Utility::FormatErrorMsg(dwError));
    sErrorMsg = sMsg;
    return FALSE;
  }

  // Set valid dbghelp API version  
  typedef LPAPI_VERSION (WINAPI* LPIMAGEHLPAPIVERSIONEX)(LPAPI_VERSION AppVersion);
  LPIMAGEHLPAPIVERSIONEX lpImagehlpApiVersionEx =
      (LPIMAGEHLPAPIVERSIONEX) GetProcAddress(hDbgHelp, "ImagehlpApiVersionEx");
  ATLASSERT(lpImagehlpApiVersionEx != NULL);
  if (lpImagehlpApiVersionEx != NULL) {
    API_VERSION CompiledApiVer;
    CompiledApiVer.MajorVersion = 6;
    CompiledApiVer.MinorVersion = 1;
    CompiledApiVer.Revision = 11;
    CompiledApiVer.Reserved = 0;
    LPAPI_VERSION pActualApiVer = lpImagehlpApiVersionEx(&CompiledApiVer);
    pActualApiVer;
    ATLASSERT(CompiledApiVer.MajorVersion == pActualApiVer->MajorVersion);
    ATLASSERT(CompiledApiVer.MinorVersion == pActualApiVer->MinorVersion);
    ATLASSERT(CompiledApiVer.Revision == pActualApiVer->Revision);
  }

  // Write minidump to the file
  mei.ThreadId = g_CrashInfo.m_dwThreadId;
  mei.ExceptionPointers = g_CrashInfo.m_pExInfo;
  mei.ClientPointers = TRUE;

  mci.CallbackRoutine = MiniDumpCallback;
  mci.CallbackParam = this;

  typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(
      HANDLE hProcess,
      DWORD ProcessId,
      HANDLE hFile,
      MINIDUMP_TYPE DumpType,
      CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
      CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam,
      CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump =
      (LPMINIDUMPWRITEDUMP) GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
  if (!pfnMiniDumpWriteDump) {
    sErrorMsg = _T("Bad MiniDumpWriteDump function");
    return FALSE;
  }

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
      g_CrashInfo.m_dwProcessId);

  BOOL bWriteDump = pfnMiniDumpWriteDump(hProcess, g_CrashInfo.m_dwProcessId,
      hFile, g_CrashInfo.m_MinidumpType, &mei, NULL, &mci);

  if (!bWriteDump) {
    CString sMsg = Utility::FormatErrorMsg(GetLastError());
    sErrorMsg = sMsg;
    goto cleanup;
  }

  bStatus = TRUE;
  m_Assync.SetProgress(_T("Finished creating dump."), 100, false);

  cleanup:

  // Close file
  if (hFile)
    CloseHandle(hFile);

  // Unload dbghelp.dll
  if (hDbgHelp)
    FreeLibrary(hDbgHelp);

  fi.m_bMakeCopy = false;
  fi.m_sDesc = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
      "DetailDlg"), _T("DescCrashDump"));
  fi.m_sDestFile = _T("crashdump.dmp");
  fi.m_sSrcFile = sMinidumpFile;
  fi.m_sErrorStatus = sErrorMsg;
  files_to_add.push_back(fi);

  // Add file to the list
  g_CrashInfo.GetReport(0).m_FileItems[fi.m_sDestFile] = fi;

  return bStatus;
}

void CErrorReportSender::AddElemToXML(CString sName, CString sValue,
    TiXmlNode* root) {
  strconv_t strconv;
  TiXmlHandle hElem = new TiXmlElement(strconv.t2utf8(sName));
  root->LinkEndChild(hElem.ToNode());
  TiXmlText* text = new TiXmlText(strconv.t2utf8(sValue));
  hElem.ToElement()->LinkEndChild(text);
}

BOOL CErrorReportSender::CreateCrashDescriptionXML(ErrorReportInfo& eri) {
  BOOL bStatus = FALSE;
  ERIFileItem fi;
  CString sFileName = eri.m_sErrorReportDirName + _T("\\crashrpt.xml");
  CString sErrorMsg;
  strconv_t strconv;
  TiXmlDocument doc;
  FILE* f = NULL;

  fi.m_bMakeCopy = false;
  fi.m_sDesc = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
      "DetailDlg"), _T("DescXML"));
  fi.m_sDestFile = _T("crashrpt.xml");
  fi.m_sSrcFile = sFileName;
  fi.m_sErrorStatus = sErrorMsg;
  // Add this file to the list
  eri.m_FileItems[fi.m_sDestFile] = fi;

  TiXmlNode* root = root = new TiXmlElement("CrashRpt");
  doc.LinkEndChild(root);
  CString sCrashRptVer;
  sCrashRptVer.Format(_T("%d"), CRASHRPT_VER);
  TiXmlHandle(root).ToElement()->SetAttribute("version", strconv.t2utf8(
      sCrashRptVer));

  {
    TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc.InsertBeforeChild(root, *decl);
  }

  AddElemToXML(_T("CrashGUID"), eri.m_sCrashGUID, root);
  AddElemToXML(_T("AppName"), eri.m_sAppName, root);
  AddElemToXML(_T("AppVersion"), eri.m_sAppVersion, root);
  AddElemToXML(_T("ImageName"), eri.m_sImageName, root);
  AddElemToXML(_T("OperatingSystem"), eri.m_sOSName, root);

  CString sOSIs64Bit;
  sOSIs64Bit.Format(_T("%d"), eri.m_bOSIs64Bit);
  AddElemToXML(_T("OSIs64Bit"), sOSIs64Bit, root);

  AddElemToXML(_T("GeoLocation"), eri.m_sGeoLocation, root);
  AddElemToXML(_T("SystemTimeUTC"), eri.m_sSystemTimeUTC, root);

  CString sExceptionType;
  sExceptionType.Format(_T("%d"), g_CrashInfo.m_nExceptionType);
  AddElemToXML(_T("ExceptionType"), sExceptionType, root);
  if (g_CrashInfo.m_nExceptionType == CR_SEH_EXCEPTION) {
    CString sExceptionCode;
    sExceptionCode.Format(_T("%d"), g_CrashInfo.m_dwExceptionCode);
    AddElemToXML(_T("ExceptionCode"), sExceptionCode, root);
  } else if (g_CrashInfo.m_nExceptionType == CR_CPP_SIGFPE) {
    CString sFPESubcode;
    sFPESubcode.Format(_T("%d"), g_CrashInfo.m_uFPESubcode);
    AddElemToXML(_T("FPESubcode"), sFPESubcode, root);
  } else if (g_CrashInfo.m_nExceptionType == CR_CPP_INVALID_PARAMETER) {
    AddElemToXML(_T("InvParamExpression"), g_CrashInfo.m_sInvParamExpr, root);
    AddElemToXML(_T("InvParamFunction"), g_CrashInfo.m_sInvParamFunction, root);
    AddElemToXML(_T("InvParamFile"), g_CrashInfo.m_sInvParamFile, root);

    CString sInvParamLine;
    sInvParamLine.Format(_T("%d"), g_CrashInfo.m_uInvParamLine);
    AddElemToXML(_T("InvParamLine"), sInvParamLine, root);
  }

  CString sGuiResources;
  sGuiResources.Format(_T("%d"), eri.m_dwGuiResources);
  AddElemToXML(_T("GUIResourceCount"), sGuiResources, root);

  CString sProcessHandleCount;
  sProcessHandleCount.Format(_T("%d"), eri.m_dwProcessHandleCount);
  AddElemToXML(_T("OpenHandleCount"), sProcessHandleCount, root);

  AddElemToXML(_T("MemoryUsageKbytes"), eri.m_sMemUsage, root);

  if (eri.m_ScreenshotInfo.m_bValid) {
    TiXmlHandle hScreenshotInfo = new TiXmlElement("ScreenshotInfo");
    root->LinkEndChild(hScreenshotInfo.ToNode());

    TiXmlHandle hVirtualScreen = new TiXmlElement("VirtualScreen");
    CString sNum;

    sNum.Format(_T("%d"), eri.m_ScreenshotInfo.m_rcVirtualScreen.left);
    hVirtualScreen.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.m_ScreenshotInfo.m_rcVirtualScreen.top);
    hVirtualScreen.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.m_ScreenshotInfo.m_rcVirtualScreen.Width());
    hVirtualScreen.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.m_ScreenshotInfo.m_rcVirtualScreen.Height());
    hVirtualScreen.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

    hScreenshotInfo.ToNode()->LinkEndChild(hVirtualScreen.ToNode());

    TiXmlHandle hMonitors = new TiXmlElement("Monitors");
    hScreenshotInfo.ToElement()->LinkEndChild(hMonitors.ToNode());

    size_t i;
    for (i = 0; i < eri.m_ScreenshotInfo.m_aMonitors.size(); i++) {
      MonitorInfo& mi = eri.m_ScreenshotInfo.m_aMonitors[i];
      CString sNum;
      TiXmlHandle hMonitor = new TiXmlElement("Monitor");

      sNum.Format(_T("%d"), mi.m_rcMonitor.left);
      hMonitor.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.top);
      hMonitor.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.Width());
      hMonitor.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.Height());
      hMonitor.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

      hMonitor.ToElement()->SetAttribute("file", strconv.t2utf8(
          Utility::GetFileName(mi.m_sFileName)));

      hMonitors.ToElement()->LinkEndChild(hMonitor.ToNode());
    }

    TiXmlHandle hWindows = new TiXmlElement("Windows");
    hScreenshotInfo.ToElement()->LinkEndChild(hWindows.ToNode());

    for (i = 0; i < eri.m_ScreenshotInfo.m_aWindows.size(); i++) {
      WindowInfo& wi = eri.m_ScreenshotInfo.m_aWindows[i];
      CString sNum;
      TiXmlHandle hWindow = new TiXmlElement("Window");

      sNum.Format(_T("%d"), wi.m_rcWnd.left);
      hWindow.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.top);
      hWindow.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.Width());
      hWindow.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.Height());
      hWindow.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

      hWindow.ToElement()->SetAttribute("title", strconv.t2utf8(wi.m_sTitle));

      hWindows.ToElement()->LinkEndChild(hWindow.ToNode());
    }
  }

  TiXmlHandle hCustomProps = new TiXmlElement("CustomProps");
  root->LinkEndChild(hCustomProps.ToNode());

  std::map<CString, CString>::iterator it2;
  for (it2 = eri.m_Props.begin(); it2 != eri.m_Props.end(); it2++) {
    TiXmlHandle hProp = new TiXmlElement("Prop");

    hProp.ToElement()->SetAttribute("name", strconv.t2utf8(it2->first));
    hProp.ToElement()->SetAttribute("value", strconv.t2utf8(it2->second));

    hCustomProps.ToElement()->LinkEndChild(hProp.ToNode());
  }

  TiXmlHandle hFileItems = new TiXmlElement("FileList");
  root->LinkEndChild(hFileItems.ToNode());

  std::map<CString, ERIFileItem>::iterator it;
  for (it = eri.m_FileItems.begin(); it != eri.m_FileItems.end(); it++) {
    ERIFileItem& fi = it->second;
    TiXmlHandle hFileItem = new TiXmlElement("FileItem");

    hFileItem.ToElement()->SetAttribute("name", strconv.t2utf8(fi.m_sDestFile));
    hFileItem.ToElement()->SetAttribute("description", strconv.t2utf8(
        fi.m_sDesc));
    if (!fi.m_sErrorStatus.IsEmpty())
      hFileItem.ToElement()->SetAttribute("error", strconv.t2utf8(
          fi.m_sErrorStatus));

    hFileItems.ToElement()->LinkEndChild(hFileItem.ToNode());
  }

#if _MSC_VER<1400
  f = _tfopen(sFileName, _T("w"));
#else
  _tfopen_s(&f, sFileName, _T("w"));
#endif

  if (f == NULL) {
    sErrorMsg = _T("Error opening file for writing");
    goto cleanup;
  }

  doc.useMicrosoftBOM = true;
  bool bSave = doc.SaveFile(f);
  if (!bSave) {
    sErrorMsg = doc.ErrorDesc();
    goto cleanup;
  }

  fclose(f);
  f = NULL;

  bStatus = TRUE;

  cleanup:

  if (f)
    fclose(f);

  if (!bStatus) {
    eri.m_FileItems[fi.m_sDestFile].m_sErrorStatus = sErrorMsg;
  }

  return bStatus;
}

// This method collects user-specified files
BOOL CErrorReportSender::CollectCrashFiles() {
  BOOL bStatus = FALSE;
  CString str;
  CString sErrorReportDir =
      g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName;
  CString sSrcFile;
  CString sDestFile;
  HANDLE hSrcFile = INVALID_HANDLE_VALUE;
  HANDLE hDestFile = INVALID_HANDLE_VALUE;
  LARGE_INTEGER lFileSize;
  BOOL bGetSize = FALSE;
  LPBYTE buffer[4096];
  LARGE_INTEGER lTotalWritten;
  DWORD dwBytesRead = 0;
  DWORD dwBytesWritten = 0;
  BOOL bRead = FALSE;
  BOOL bWrite = FALSE;
  std::map<CString, CString>::iterator rit;

  // Copy application-defined files that should be copied on crash
  m_Assync.SetProgress(_T("[copying_files]"), 0, false);

  std::map<CString, ERIFileItem>::iterator it;
  for (it = g_CrashInfo.GetReport(m_nCurReport).m_FileItems.begin(); it
      != g_CrashInfo.GetReport(m_nCurReport).m_FileItems.end(); it++) {
    if (m_Assync.IsCancelled())
      goto cleanup;

    if (it->second.m_bMakeCopy) {

      hSrcFile = CreateFile(it->second.m_sSrcFile, GENERIC_READ,
          FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hSrcFile == INVALID_HANDLE_VALUE) {
        it->second.m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
      }

      bGetSize = GetFileSizeEx(hSrcFile, &lFileSize);
      if (!bGetSize) {
        it->second.m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
        CloseHandle(hSrcFile);
        hSrcFile = INVALID_HANDLE_VALUE;
        continue;
      }

      sDestFile = sErrorReportDir + _T("\\") + it->second.m_sDestFile;

      hDestFile = CreateFile(sDestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
          CREATE_ALWAYS, 0, NULL);
      if (hDestFile == INVALID_HANDLE_VALUE) {
        it->second.m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
        CloseHandle(hSrcFile);
        hSrcFile = INVALID_HANDLE_VALUE;
        continue;
      }

      lTotalWritten.QuadPart = 0;

      for (;;) {
        if (m_Assync.IsCancelled())
          goto cleanup;

        bRead = ReadFile(hSrcFile, buffer, 4096, &dwBytesRead, NULL);
        if (!bRead || dwBytesRead == 0)
          break;

        bWrite = WriteFile(hDestFile, buffer, dwBytesRead, &dwBytesWritten,
            NULL);
        if (!bWrite || dwBytesRead != dwBytesWritten)
          break;

        lTotalWritten.QuadPart += dwBytesWritten;

        int nProgress = (int) (100.0f * lTotalWritten.QuadPart
            / lFileSize.QuadPart);

        m_Assync.SetProgress(nProgress, false);
      }

      //if(lTotalWritten.QuadPart!=lFileSize.QuadPart)
      //  goto cleanup; // Error copying file

      CloseHandle(hSrcFile);
      hSrcFile = INVALID_HANDLE_VALUE;
      CloseHandle(hDestFile);
      hDestFile = INVALID_HANDLE_VALUE;
    }
  }

  // Create dump of registry keys
  ErrorReportInfo& eri = g_CrashInfo.GetReport(m_nCurReport);
  for (rit = eri.m_RegKeys.begin(); rit != eri.m_RegKeys.end(); rit++) {
    if (m_Assync.IsCancelled())
      goto cleanup;

    CString sFileName = eri.m_sErrorReportDirName + _T("\\") + rit->second;
    // Create registry key dump
    CString sErrorMsg;
    DumpRegKey(rit->first, sFileName, sErrorMsg);
    ERIFileItem fi;
    fi.m_sSrcFile = sFileName;
    fi.m_sDestFile = rit->second;
    fi.m_sDesc = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
        "DetailDlg"), _T("DescRegKey"));
    fi.m_bMakeCopy = FALSE;
    fi.m_sErrorStatus = sErrorMsg;
    std::vector<ERIFileItem> file_list;
    file_list.push_back(fi);
    // Add file to the list of file items
    g_CrashInfo.GetReport(0).m_FileItems[fi.m_sDestFile] = fi;
  }

  // Create crash description XML
  CreateCrashDescriptionXML(g_CrashInfo.GetReport(0));

  // Success
  bStatus = TRUE;

  cleanup:

  if (hSrcFile != INVALID_HANDLE_VALUE)
    CloseHandle(hSrcFile);

  if (hDestFile != INVALID_HANDLE_VALUE)
    CloseHandle(hDestFile);

  m_Assync.SetProgress(_T("Finished copying files."), 100, false);

  return 0;
}

int CErrorReportSender::DumpRegKey(CString sRegKey, CString sDestFile,
    CString& sErrorMsg) {
  strconv_t strconv;
  TiXmlDocument document;

  // Load document if file already exists
  // otherwise create new document.

  FILE* f = NULL;
#if _MSC_VER<1400
  f = _tfopen(sDestFile, _T("rb"));
#else
  _tfopen_s(&f, sDestFile, _T("rb"));
#endif
  if (f != NULL) {
    document.LoadFile(f);
    fclose(f);
    f = NULL;
  }

  TiXmlHandle hdoc(&document);

  TiXmlElement* registry = document.RootElement();

  if (registry == NULL) {
    registry = new TiXmlElement("registry");
    document.LinkEndChild(registry);
  }

  TiXmlNode* declaration = hdoc.Child(0).ToNode();
  if (declaration == NULL || declaration->Type()
      != TiXmlNode::TINYXML_DECLARATION) {
    TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    document.InsertBeforeChild(registry, *decl);
  }

  DumpRegKey(NULL, sRegKey, registry);

#if _MSC_VER<1400
  f = _tfopen(sDestFile, _T("wb"));
#else
  _tfopen_s(&f, sDestFile, _T("wb"));
#endif
  if (f == NULL) {
    sErrorMsg = _T("Error opening file for writing.");
    return 1;
  }

  bool bSave = document.SaveFile(f);

  fclose(f);

  if (!bSave) {
    sErrorMsg = _T("Error saving XML document to file: ");
    sErrorMsg += document.ErrorDesc();
  }

  return (bSave == true) ? 0 : 1;
}

int CErrorReportSender::DumpRegKey(HKEY hParentKey, CString sSubKey,
    TiXmlElement* elem) {
  strconv_t strconv;
  HKEY hKey = NULL;

  if (hParentKey == NULL) {
    int nSkip = 0;
    if (sSubKey.Left(19).Compare(_T("HKEY_LOCAL_MACHINE\\")) == 0) {
      hKey = HKEY_LOCAL_MACHINE;
      nSkip = 18;
    } else if (sSubKey.Left(18).Compare(_T("HKEY_CURRENT_USER\\")) == 0) {
      hKey = HKEY_CURRENT_USER;
      nSkip = 17;
    } else {
      return 1; // Invalid key.
    }

    CString sKey = sSubKey.Mid(0, nSkip);
    const char* szKey = strconv.t2utf8(sKey);
    sSubKey = sSubKey.Mid(nSkip + 1);

    TiXmlHandle key_node = elem->FirstChild(szKey);
    if (key_node.ToElement() == NULL) {
      key_node = new TiXmlElement("k");
      elem->LinkEndChild(key_node.ToNode());
      key_node.ToElement()->SetAttribute("name", szKey);
    }

    DumpRegKey(hKey, sSubKey, key_node.ToElement());
  } else {
    int pos = sSubKey.Find('\\');
    CString sKey = sSubKey;
    if (pos > 0)
      sKey = sSubKey.Mid(0, pos);
    const char* szKey = strconv.t2utf8(sKey);

    TiXmlHandle key_node = elem->FirstChild(szKey);
    if (key_node.ToElement() == NULL) {
      key_node = new TiXmlElement("k");
      elem->LinkEndChild(key_node.ToNode());
      key_node.ToElement()->SetAttribute("name", szKey);
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(hParentKey, sKey, 0, GENERIC_READ, &hKey)) {
      if (pos > 0) {
        sSubKey = sSubKey.Mid(pos + 1);
        DumpRegKey(hKey, sSubKey, key_node.ToElement());
      } else {
        DWORD dwSubKeys = 0;
        DWORD dwMaxSubKey = 0;
        DWORD dwValues = 0;
        DWORD dwMaxValueNameLen = 0;
        DWORD dwMaxValueLen = 0;
        LONG lResult = RegQueryInfoKey(hKey, NULL, 0, 0, &dwSubKeys,
            &dwMaxSubKey, 0, &dwValues, &dwMaxValueNameLen, &dwMaxValueLen,
            NULL, NULL);
        if (lResult == ERROR_SUCCESS) {
          // Enumerate and dump subkeys          
          int i;
          for (i = 0; i < (int) dwSubKeys; i++) {
            LPWSTR szName = new WCHAR[dwMaxSubKey];
            DWORD dwLen = dwMaxSubKey;
            lResult = RegEnumKeyEx(hKey, i, szName, &dwLen, 0, NULL, 0, NULL);
            if (lResult == ERROR_SUCCESS) {
              DumpRegKey(hKey, CString(szName), key_node.ToElement());
            }

            delete[] szName;
          }

          // Dump key values 
          for (i = 0; i < (int) dwValues; i++) {
            LPWSTR szName = new WCHAR[dwMaxValueNameLen];
            LPBYTE pData = new BYTE[dwMaxValueLen];
            DWORD dwNameLen = dwMaxValueNameLen;
            DWORD dwValueLen = dwMaxValueLen;
            DWORD dwType = 0;

            lResult = RegEnumValue(hKey, i, szName, &dwNameLen, 0, &dwType,
                pData, &dwValueLen);
            if (lResult == ERROR_SUCCESS) {
              TiXmlHandle val_node = key_node.ToElement()->FirstChild(
                  strconv.w2utf8(szName));
              if (val_node.ToElement() == NULL) {
                val_node = new TiXmlElement("v");
                key_node.ToElement()->LinkEndChild(val_node.ToNode());
              }

              val_node.ToElement()->SetAttribute("name", strconv.w2utf8(szName));

              char str[128];
              LPSTR szType = NULL;
              if (dwType == REG_BINARY)
                szType = "REG_BINARY";
              else if (dwType == REG_DWORD)
                szType = "REG_DWORD";
              else if (dwType == REG_EXPAND_SZ)
                szType = "REG_EXPAND_SZ";
              else if (dwType == REG_MULTI_SZ)
                szType = "REG_MULTI_SZ";
              else if (dwType == REG_QWORD)
                szType = "REG_QWORD";
              else if (dwType == REG_SZ)
                szType = "REG_SZ";
              else {
#if _MSC_VER<1400
                sprintf(str, "Unknown type (0x%08x)", dwType);
#else
                sprintf_s(str, 128, "Unknown type (0x%08x)", dwType);
#endif
                szType = str;
              }

              val_node.ToElement()->SetAttribute("type", szType);

              if (dwType == REG_BINARY) {
                std::string str;
                int i;
                for (i = 0; i < (int) dwValueLen; i++) {
                  char num[10];
#if _MSC_VER<1400
                  sprintf(num, "%02X", pData[i]);
#else
                  sprintf_s(num, 10, "%02X", pData[i]);
#endif
                  str += num;
                  if (i < (int) dwValueLen)
                    str += " ";
                }

                val_node.ToElement()->SetAttribute("value", str.c_str());
              } else if (dwType == REG_DWORD) {
                LPDWORD pdwValue = (LPDWORD) pData;
                char str[64];
#if _MSC_VER<1400
                sprintf(str, "0x%08x (%lu)", *pdwValue, *pdwValue);
#else
                sprintf_s(str, 64, "0x%08x (%lu)", *pdwValue, *pdwValue);
#endif
                val_node.ToElement()->SetAttribute("value", str);
              } else if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
                const char* szValue = strconv.t2utf8((LPCTSTR) pData);
                val_node.ToElement()->SetAttribute("value", szValue);

                /*if(dwType==REG_EXPAND_SZ)
                 {
                 DWORD dwDstBuffSize = ExpandEnvironmentStrings((LPCTSTR)pData, NULL, 0);
                 LPTSTR szExpanded = new TCHAR[dwDstBuffSize];
                 ExpandEnvironmentStrings((LPCTSTR)pData, szExpanded, dwDstBuffSize);
                 val_node.ToElement()->SetAttribute("expanded", strconv.t2utf8(szExpanded));
                 delete [] szExpanded;
                 }*/
              } else if (dwType == REG_MULTI_SZ) {
                LPCTSTR szValues = (LPCTSTR) pData;
                int prev = 0;
                int pos = 0;
                for (;;) {
                  if (szValues[pos] == 0) {
                    CString sValue = CString(szValues + prev, pos - prev);
                    const char* szValue = strconv.t2utf8(sValue);

                    TiXmlHandle str_node = new TiXmlElement("str");
                    val_node.ToElement()->LinkEndChild(str_node.ToNode());
                    str_node.ToElement()->SetAttribute("value", szValue);

                    prev = pos + 1;
                  }

                  if (szValues[pos] == 0 && szValues[pos + 1] == 0)
                    break; // Double-null

                  pos++;
                }
              }
            }

            delete[] szName;
            delete[] pData;
          }
        }
      }

      RegCloseKey(hKey);
    } else {
      CString sErrMsg = Utility::FormatErrorMsg(GetLastError());
      const char* szErrMsg = strconv.t2utf8(sErrMsg);
      key_node.ToElement()->SetAttribute("error", szErrMsg);
    }
  }

  return 0;
}

int CErrorReportSender::CalcFileMD5Hash(CString sFileName, CString& sMD5Hash) {
  FILE* f = NULL;
  BYTE buff[512];
  MD5 md5;
  MD5_CTX md5_ctx;
  unsigned char md5_hash[16];
  int i;

  sMD5Hash.Empty();

#if _MSC_VER<1400
  f = _tfopen(sFileName.GetBuffer(0), _T("rb"));
#else
  _tfopen_s(&f, sFileName.GetBuffer(0), _T("rb"));
#endif

  if (f == NULL)
    return -1;

  md5.MD5Init(&md5_ctx);

  while (!feof(f)) {
    size_t count = fread(buff, 1, 512, f);
    if (count > 0) {
      md5.MD5Update(&md5_ctx, buff, (unsigned int) count);
    }
  }

  fclose(f);
  md5.MD5Final(md5_hash, &md5_ctx);

  for (i = 0; i < 16; i++) {
    CString number;
    number.Format(_T("%02x"), md5_hash[i]);
    sMD5Hash += number;
  }

  return 0;
}

// This method restarts the application
BOOL CErrorReportSender::RestartApp() {
  if (g_CrashInfo.m_bAppRestart == FALSE)
    return FALSE;

  m_Assync.SetProgress(_T("Restarting the application..."), 0, false);

  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));

  CString sCmdLine;
  if (g_CrashInfo.m_sRestartCmdLine.IsEmpty()) {
    // Format this way to avoid first empty parameter
    sCmdLine.Format(_T("\"%s\""),
        g_CrashInfo.GetReport(m_nCurReport).m_sImageName);
  } else {
    sCmdLine.Format(_T("\"%s\" \"%s\""),
        g_CrashInfo.GetReport(m_nCurReport).m_sImageName,
        g_CrashInfo.m_sRestartCmdLine.GetBuffer(0));
  }
  BOOL bCreateProcess = CreateProcess(
      g_CrashInfo.GetReport(m_nCurReport).m_sImageName, sCmdLine.GetBuffer(0),
      NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if (!bCreateProcess) {
    m_Assync.SetProgress(_T("Error restarting the application!"), 0, false);
    return FALSE;
  }

  m_Assync.SetProgress(_T("Application restarted OK."), 0, false);
  return TRUE;
}

LONG64 CErrorReportSender::GetUncompressedReportSize(ErrorReportInfo& eri) {
  LONG64 lTotalSize = 0;
  std::map<CString, ERIFileItem>::iterator it;
  HANDLE hFile = INVALID_HANDLE_VALUE;
//  CString sMsg;
  BOOL bGetSize = FALSE;
  LARGE_INTEGER lFileSize;

  for (it = eri.m_FileItems.begin(); it != eri.m_FileItems.end(); it++) {
    if (m_Assync.IsCancelled())
      return 0;

    CString sFileName = it->second.m_sSrcFile.GetBuffer(0);
    hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      continue;
    }

    bGetSize = GetFileSizeEx(hFile, &lFileSize);
    if (!bGetSize) {
      CloseHandle(hFile);
      continue;
    }

    lTotalSize += lFileSize.QuadPart;
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
  }

  return lTotalSize;
}

// This method compresses the files contained in the report and produces ZIP archive.
BOOL CErrorReportSender::CompressReportFiles(ErrorReportInfo& eri) {
  BOOL bStatus = FALSE;
  strconv_t strconv;
  zipFile hZip = NULL;
  CString sMsg;
  LONG64 lTotalSize = 0;
  LONG64 lTotalCompressed = 0;
  BYTE buff[1024];
  DWORD dwBytesRead = 0;
  HANDLE hFile = INVALID_HANDLE_VALUE;
  std::map<CString, ERIFileItem>::iterator it;
  FILE* f = NULL;
  CString sMD5Hash;

  if (m_bExport)
    m_Assync.SetProgress(_T("[exporting_report]"), 0, false);
  else
    m_Assync.SetProgress(_T("[compressing_files]"), 0, false);

  lTotalSize = GetUncompressedReportSize(eri);

  if (m_bExport)
    m_sZipName = m_sExportFileName;
  else
    m_sZipName = eri.m_sErrorReportDirName + _T(".zip");

  hZip = zipOpen((const char*) m_sZipName.GetBuffer(0), APPEND_STATUS_CREATE);
  if (hZip == NULL) {
    goto cleanup;
  }

  for (it = eri.m_FileItems.begin(); it != eri.m_FileItems.end(); it++) {
    if (m_Assync.IsCancelled())
      goto cleanup;

    CString sDstFileName = it->second.m_sDestFile.GetBuffer(0);
    CString sFileName = it->second.m_sSrcFile.GetBuffer(0);
    CString sDesc = it->second.m_sDesc;

    HANDLE hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      continue;
    }

    BY_HANDLE_FILE_INFORMATION fi;
    GetFileInformationByHandle(hFile, &fi);

    SYSTEMTIME st;
    FileTimeToSystemTime(&fi.ftCreationTime, &st);

    zip_fileinfo info;
    info.dosDate = 0;
    info.tmz_date.tm_year = st.wYear;
    info.tmz_date.tm_mon = st.wMonth - 1;
    info.tmz_date.tm_mday = st.wDay;
    info.tmz_date.tm_hour = st.wHour;
    info.tmz_date.tm_min = st.wMinute;
    info.tmz_date.tm_sec = st.wSecond;
    info.external_fa = FILE_ATTRIBUTE_NORMAL;
    info.internal_fa = FILE_ATTRIBUTE_NORMAL;

    int n = zipOpenNewFileInZip(hZip, (const char*) strconv.t2a(
        sDstFileName.GetBuffer(0)), &info, NULL, 0, NULL, 0,
        strconv.t2a(sDesc), Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if (n != 0) {
      continue;
    }

    for (;;) {
      if (m_Assync.IsCancelled())
        goto cleanup;

      BOOL bRead = ReadFile(hFile, buff, 1024, &dwBytesRead, NULL);
      if (!bRead || dwBytesRead == 0)
        break;

      int res = zipWriteInFileInZip(hZip, buff, dwBytesRead);
      if (res != 0) {
        zipCloseFileInZip(hZip);
        break;
      }

      lTotalCompressed += dwBytesRead;

      float fProgress = 100.0f * lTotalCompressed / lTotalSize;
      m_Assync.SetProgress((int) fProgress, false);
    }

    zipCloseFileInZip(hZip);
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
  }

  if (hZip != NULL) {
    zipClose(hZip, NULL);
    hZip = NULL;
  }

  // Save MD5 hash file
  if (!m_bExport) {
    int nCalcMD5 = CalcFileMD5Hash(m_sZipName, sMD5Hash);
    if (nCalcMD5 != 0) {
      goto cleanup;
    }

#if _MSC_VER <1400
    f = _tfopen(m_sZipName + _T(".md5"), _T("wt"));
#else
    _tfopen_s(&f, m_sZipName + _T(".md5"), _T("wt"));
#endif
    if (f == NULL) {
      goto cleanup;
    }

    _ftprintf(f, sMD5Hash);
    fclose(f);
    f = NULL;
  }

  if (lTotalSize == lTotalCompressed)
    bStatus = TRUE;

  cleanup:

  if (hZip != NULL)
    zipClose(hZip, NULL);

  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);

  if (f != NULL)
    fclose(f);
  return bStatus;
}

// This method sends the error report over the Internet
BOOL CErrorReportSender::SendReport() {
  int status = 1;

  m_Assync.SetProgress(_T("[sending_report]"), 0);

  //  使用Map,因为map是有序的，因此这里是按照优先级大小顺序进行发送的。
  // TODO(yesp) : 删除该方法，使用worker依次调用发送发送。
  std::multimap<int, int> order;

  std::pair<int, int> pair1(g_CrashInfo.m_uPriorities[CR_HTTP], CR_HTTP);
  order.insert(pair1);

  std::multimap<int, int>::reverse_iterator rit;

  for (rit = order.rbegin(); rit != order.rend(); rit++) {
    if (rit->first <= 0) {
      continue;
    }
    m_Assync.SetProgress(_T("[sending_attempt]"), 0);
    m_SendAttempt++;

    if (m_Assync.IsCancelled()) {
      break;
    }

    int id = rit->second;
    BOOL bResult = FALSE;

    if (id == CR_HTTP)
      bResult = SendOverHTTP();
    if (bResult == FALSE)
      continue;

    if (0 == m_Assync.WaitForCompletion()) {
      status = 0;
      break;
    }
  }

  // Remove compressed error report file
  Utility::RecycleFile(m_sZipName, true);

  if (status == 0) {
    m_Assync.SetProgress(_T("[status_success]"), 0);
    g_CrashInfo.GetReport(m_nCurReport).m_DeliveryStatus = DELIVERED;
    // Delete report files
    Utility::RecycleFile(
        g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName, true);
  } else {
    //  已经发送失败，这时候应该主动关闭窗口，而不应该让用户点击close进行关闭。
    g_CrashInfo.GetReport(m_nCurReport).m_DeliveryStatus = FAILED;
    m_Assync.SetProgress(_T("[status_failed]"), 0);

    // Check if we should store files for later delivery or we should remove them
    if (!g_CrashInfo.m_bQueueEnabled) {
      // Delete report files
      Utility::RecycleFile(
          g_CrashInfo.GetReport(m_nCurReport).m_sErrorReportDirName, true);
    }
  }

  m_nGlobalStatus = status;
  m_Assync.SetCompleted(status);
  return 0;
}

// This method sends the report over HTTP request
BOOL CErrorReportSender::SendOverHTTP() {
  strconv_t strconv;

  if (g_CrashInfo.m_uPriorities[CR_HTTP] == CR_NEGATIVE_PRIORITY) {
    return FALSE;
  }

  if (g_CrashInfo.m_sUrl.IsEmpty()) {
    return FALSE;
  }

  CHttpRequest request;
  request.m_sUrl = g_CrashInfo.m_sUrl;

  request.m_aTextFields[_T("appname")] = strconv.t2a(
                        g_CrashInfo.GetReport(m_nCurReport).m_sAppName);
  request.m_aTextFields[_T("appversion")] = strconv.t2a(
                        g_CrashInfo.GetReport(m_nCurReport).m_sAppVersion);
  request.m_aTextFields[_T("crashguid")] = strconv.t2a(
                        g_CrashInfo.GetReport(m_nCurReport).m_sCrashGUID);
  request.m_aTextFields[_T("emailfrom")] = strconv.t2a(
                        g_CrashInfo.GetReport(m_nCurReport).m_sEmailFrom);
  request.m_aTextFields[_T("emailsubject")] = strconv.t2a(
                        g_CrashInfo.m_sEmailSubject);
  request.m_aTextFields[_T("description")] = strconv.t2a(
                        g_CrashInfo.GetReport(m_nCurReport).m_sDescription);

  CString sMD5Hash;
  CalcFileMD5Hash(m_sZipName, sMD5Hash);
  request.m_aTextFields[_T("md5")] = strconv.t2a(sMD5Hash);

  if (g_CrashInfo.m_bHttpBinaryEncoding) {
    CHttpRequestFile f;
    f.m_sSrcFileName = m_sZipName;
    f.m_sContentType = _T("application/zip");
    request.m_aIncludedFiles[_T("crashrpt")] = f;
  } else {
    m_Assync.SetProgress(_T("Base-64 encoding file \
                         attachment, please wait..."), 1);
    std::string sEncodedData;
    int nRet = Base64EncodeAttachment(m_sZipName, sEncodedData);
    if (nRet != 0) {
      return FALSE;
    }
    request.m_aTextFields[_T("crashrpt")] = sEncodedData;
  }

  BOOL bSend = m_HttpSender.SendAssync(request, &m_Assync);
  return bSend;
}

int CErrorReportSender::Base64EncodeAttachment(CString sFileName,
    std::string& sEncodedFileData) {
  strconv_t strconv;

  int uFileSize = 0;
  BYTE* uchFileData = NULL;
  struct _stat st;

  int nResult = _tstat(sFileName, &st);
  if (nResult != 0)
    return 1; // File not found.

  // Allocate buffer of file size
  uFileSize = st.st_size;
  uchFileData = new BYTE[uFileSize];

  // Read file data to buffer.
  FILE* f = NULL;
#if _MSC_VER<1400
  f = _tfopen(sFileName, _T("rb"));
#else
  /*errno_t err = */_tfopen_s(&f, sFileName, _T("rb"));
#endif 

  if (!f || fread(uchFileData, uFileSize, 1, f) != 1) {
    delete[] uchFileData;
    uchFileData = NULL;
    return 2; // Coudln't read file data.
  }

  fclose(f);

  sEncodedFileData = base64_encode(uchFileData, uFileSize);

  delete[] uchFileData;

  // OK.
  return 0;
}

