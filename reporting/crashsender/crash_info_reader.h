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

// File: CrashInfoReader.h
// Description: Retrieves crash information passed from crash_report.dll in form of XML files.
// Authors: zexspectrum
// Date: 2010

#ifndef CRASH_INFO_READER_H_
#define CRASH_INFO_READER_H_
#include "stdafx.h"
#include "tinyxml.h"
#include "shared_memory.h"
#include "screen_capure.h"

// The structure describing file item.
struct ERIFileItem {
  ERIFileItem() {
    m_bMakeCopy = FALSE;
  }

  CString m_sDestFile; // Destination file name (not including directory name).
  CString m_sSrcFile; // Absolute source file path.
  CString m_sDesc; // File description.
  BOOL m_bMakeCopy; // Should we copy source file to error report folder?
  CString m_sErrorStatus; // Empty if OK, non-empty if error occurred.
};

// Error report delivery status
enum DELIVERY_STATUS {
  PENDING = 0, // Status pending
  DELIVERED = 1, // Error report was delivered ok
  FAILED = 2
// Error report delivery failed
};

// Error report info
struct ErrorReportInfo {
  ErrorReportInfo() {
    m_bSelected = TRUE;
    m_DeliveryStatus = PENDING;
    m_dwGuiResources = 0;
    m_dwProcessHandleCount = 0;
    m_uTotalSize = 0;
  }

  CString m_sErrorReportDirName; // Name of the directory where error report files are located.
  CString m_sCrashGUID; // Crash GUID.
  CString m_sAppName; // Application name.
  CString m_sAppVersion; // Application version.
  CString m_sImageName; // Path to the application executable file.
  CString m_sEmailFrom; // E-mail sender address.
  CString m_sDescription; // User-provided problem description.
  CString m_sSystemTimeUTC; // The time when crash occurred (UTC).
  DWORD m_dwGuiResources; // GUI resource count.
  DWORD m_dwProcessHandleCount; // Process handle count.
  CString m_sMemUsage; // Memory usage.
  CString m_sOSName; // Operating system friendly name.
  BOOL m_bOSIs64Bit; // Is operating system 64-bit?
  CString m_sGeoLocation; // Geographic location.
  ScreenshotInfo m_ScreenshotInfo; // Screenshot info
  ULONG64 m_uTotalSize; // Summary size of this (uncompressed) report.
  BOOL m_bSelected; // Is this report selected for delivery.
  DELIVERY_STATUS m_DeliveryStatus; // Delivery status.

  // The list of files that are included into this report.
  std::map<CString, ERIFileItem> m_FileItems;
  std::map<CString, CString> m_RegKeys;
  std::map<CString, CString> m_Props;
};

// Remind policy.
enum REMIND_POLICY {
  REMIND_LATER, // Remind later.
  NEVER_REMIND
// Never remind.
};

// Class responsible for reading the crash info.
class CCrashInfoReader {
public:
  int m_nCrashRptVersion;
  CString m_sUnsentCrashReportsFolder;
  CString m_sLangFileName;
  CString m_sDbgHelpPath;
  CString m_sAppName;
  CString m_sCustomSenderIcon;
  CString m_sUrl;
  BOOL m_bSilentMode;
  BOOL m_bSendErrorReport;
  BOOL m_bStoreZIPArchives;
  BOOL m_bSendRecentReports;
  BOOL m_bAppRestart;
  CString m_sRestartCmdLine;
  // TODO(yesp) : remove Priorities for all protocol, but use worker to try all protocols,
  int send_method;
  CString m_sPrivacyPolicyURL;
  BOOL m_bGenerateMinidump;
  MINIDUMP_TYPE m_MinidumpType;
  BOOL m_bAddScreenshot;
  DWORD m_dwScreenshotFlags;
  int m_nJpegQuality;
  CPoint m_ptCursorPos; // Mouse cursor position on crash.
  CRect m_rcAppWnd; // Rectangle of the application's main window.
  BOOL m_bQueueEnabled;
  DWORD m_dwProcessId;
  DWORD m_dwThreadId;
  PEXCEPTION_POINTERS m_pExInfo;
  int m_nExceptionType;
  DWORD m_dwExceptionCode;
  UINT m_uFPESubcode;
  CString m_sInvParamExpr;
  CString m_sInvParamFunction;
  CString m_sInvParamFile;
  UINT m_uInvParamLine;
  /* Member functions */
  // Gets crash info from shared memory
  int Init(CString sFileMappingName);
  // Loads custom icon (if defined)
  HICON GetCustomIcon();
  // Retrieves some crash info from crash description XML
  int ParseCrashDescription(CString sFileName, BOOL bParseFileItems,
      ErrorReportInfo& eri);
  BOOL AddUserInfoToCrashDescriptionXML(CString sEmail, CString sDesc);
  BOOL AddFilesToCrashDescriptionXML(std::vector<ERIFileItem>);
  ErrorReportInfo& GetReport(int nIndex) {
    return m_Reports[nIndex];
  }
  int GetReportCount() {
    return (int) m_Reports.size();
  }
  BOOL GetLastRemindDate(SYSTEMTIME& LastDate);
  BOOL SetLastRemindDateToday();
  BOOL IsRemindNowOK();
  REMIND_POLICY GetRemindPolicy();
  BOOL SetRemindPolicy(REMIND_POLICY Policy);

private:
  int UnpackCrashDescription(ErrorReportInfo& eri);
  int UnpackString(DWORD dwOffset, CString& str);
  void CollectMiscCrashInfo(ErrorReportInfo& eri);
  // Gets the list of file items 
  int ParseFileList(TiXmlHandle& hRoot, ErrorReportInfo& eri);
  int ParseRegKeyList(TiXmlHandle& hRoot, ErrorReportInfo& eri);
  // Calculates size of an uncompressed error report.
  LONG64 GetUncompressedReportSize(ErrorReportInfo& eri);

  // Array of error reports
  std::vector<ErrorReportInfo> m_Reports;
  // Path to ~CrashRpt.ini file.
  CString m_sINIFile;
  CSharedMem m_SharedMem;
  CRASH_DESCRIPTION* m_pCrashDesc;
};

// Declare globally available object.
extern CCrashInfoReader g_CrashInfo;
#endif //  CRASH_INFO_READER_H_

