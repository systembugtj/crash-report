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

#ifndef ERROR_REPORT_SENDER_H_
#define ERROR_REPORT_SENDER_H_
#include <string>
#include "assync_notification.h"
#include "tinyxml.h"
#include "crash_info_reader.h"

// TODO(yesp) : 该类应该进行较大幅度的重构
using std::string;

enum ActionType {
  COLLECT_CRASH_INFO = 0x01,
  COMPRESS_REPORT = 0x02,
  RESTART_APP = 0x04,
  SEND_REPORT = 0x08
};

class CErrorReportSender {
public:

  CErrorReportSender();
  ~CErrorReportSender();

  BOOL DoWork(int action);
  void SetExportFlag(BOOL bExport, CString sExportFile);

  void WaitForCompletion();
  int GetGlobalStatus();
  void GetStatus(int& nProgressPct, std::vector<CString>& msg_log);
  void Cancel();
  void FeedbackReady(int code);

  LONG64 GetUncompressedReportSize(ErrorReportInfo& eri);

  int GetCurReport();
  BOOL SetCurReport(int nCurReport);

  BOOL SetLogFile(LPCTSTR szFileName);

  void DoWorkAssync();static DWORD WINAPI WorkerThread(LPVOID lpParam);

  BOOL CollectCrashFiles();
  int CalcFileMD5Hash(CString sFileName, CString& sMD5Hash);
  int DumpRegKey(CString sRegKey, CString sDestFile, CString& sErrorMsg);
  int DumpRegKey(HKEY hKeyParent, CString sSubKey, TiXmlElement* elem);
  BOOL TakeDesktopScreenshot();
  BOOL CreateMiniDump();
  BOOL CreateCrashDescriptionXML(ErrorReportInfo& eri);
  void AddElemToXML(CString sName, CString sValue, TiXmlNode* root);
  static BOOL CALLBACK MiniDumpCallback(PVOID CallbackParam, PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);
  BOOL OnMinidumpProgress(const PMINIDUMP_CALLBACK_INPUT CallbackInput,
      PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);
  BOOL RestartApp();
  BOOL CompressReportFiles(ErrorReportInfo& eri);
  void UnblockParentProcess();

  BOOL SendReport();
  BOOL SendOverHTTPMutilpart(CString m_sZipName, string appname,string appversion,
                    string crashguid,string description,string md5);
  BOOL SendOverHTTPBase64(CString m_sZipName, string appname,string appversion,
    string crashguid,string description,string md5);

  //int Base64EncodeAttachment(CString sFileName, std::string& sEncodedFileData);

  int m_nGlobalStatus;
  int m_nCurReport;
  HANDLE m_hThread; // Handle to the worker thread
  int m_SendAttempt; // Number of current sending attempt
  AssyncNotification m_Assync; // Used for communication with the main thread
  //CHttpRequestSender m_HttpSender; // Used to send report over HTTP
  CString m_sZipName; // Name of the ZIP archive to send
  int m_Action; // Current action
  BOOL m_bExport;
  CString m_sExportFileName;
};

extern CErrorReportSender g_ErrorReportSender;

#endif //  ERROR_REPORT_SENDER_H_
