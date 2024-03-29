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
#ifndef ABOUT_DLG_H_
#define ABOUT_DLG_H_
#include "stdafx.h"
#include <shellapi.h>
#include <atlctrlx.h>
#include "crash_report_helper.h"

class CAboutDlg: public CDialogImpl<CAboutDlg> {
public:
  enum {
    IDD = IDD_ABOUTBOX
  };

  CStatic m_statVersion;
  CHyperLink m_lnkVisit;

  BEGIN_MSG_MAP( CAboutDlg)MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM , LPARAM , BOOL&) {
    m_statVersion = GetDlgItem(IDC_VERSION);
    CString sVersion;
    sVersion.Format(_T("Version: %d.%d.%d"),
        CRASHRPT_VER/1000,
        (CRASHRPT_VER%1000)/100,
        (CRASHRPT_VER%1000)%100);
    m_statVersion.SetWindowText(sVersion);
    m_lnkVisit.SubclassWindow(GetDlgItem(IDC_LINK));
    m_lnkVisit.SetHyperLink(_T("http://code.google.com/p/crash-report/"));
    return 0;
  }

  LRESULT OnOK(WORD, WORD, HWND, BOOL&) {
    CloseDialog(0);
    return 0;
  }

  LRESULT OnCancel(WORD, WORD, HWND, BOOL&) {
    CloseDialog(1);
    return 0;
  }

  void CloseDialog(int) {
    ShowWindow( SW_HIDE);
  }

};
#endif // ABOUT_DLG_H_
