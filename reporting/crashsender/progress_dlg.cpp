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
#include "progress_dlg.h"
#include "base/utility.h"
#include "crash_info_reader.h"

LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/,
    LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  // Check if current UI language is an RTL language
  CString sRTL = Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
      "Settings"), _T("RTLReading"));
  if (sRTL.CompareNoCase(_T("1")) == 0) {
    // Mirror this window
    Utility::SetLayoutRTL( m_hWnd);
  }

  HICON hIcon = NULL;

  // Try to load custom icon
  hIcon = g_CrashInfo.GetCustomIcon();
  if (hIcon == NULL)
    hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(
        IDR_MAINFRAME));

  SetIcon(hIcon, FALSE);
  SetIcon(hIcon, TRUE);

  m_statText = GetDlgItem(IDC_TEXT);
  m_statText.SetWindowText(Utility::GetINIString(g_CrashInfo.m_sLangFileName,
      _T("ProgressDlg"), _T("CollectingCrashInfo")));

  m_prgProgress = GetDlgItem(IDC_PROGRESS);
  m_prgProgress.SetRange(0, 100);
  m_prgProgress.ModifyStyle(0, PBS_MARQUEE);

  m_listView = GetDlgItem(IDC_LIST);
  m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT,
      LVS_EX_FULLROWSELECT);
  m_listView.InsertColumn(0, _T("Status"), LVCFMT_LEFT, 2048);

  m_btnCancel = GetDlgItem(IDCANCEL);
  m_btnCancel.SetWindowText(Utility::GetINIString(g_CrashInfo.m_sLangFileName,
      _T("ProgressDlg"), _T("Cancel")));

  m_ActionOnCancel = DONT_CLOSE;
  m_ActionOnClose = CLOSE_MYSELF;

  DlgResize_Init();

  return TRUE;
}

LRESULT CProgressDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/,
    LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (m_ActionOnClose == CLOSE_MYSELF_AND_PARENT) {
    AnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);

    HWND hWndParent = ::GetParent(m_hWnd);
    ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
    return 0;
  } else if (m_ActionOnClose == CLOSE_MYSELF) {
    AnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);
    return 0;
  }

  return 0;
}

LRESULT CProgressDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/,
    HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (m_ActionOnCancel == CLOSE_MYSELF_AND_PARENT) {
    AnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);

    HWND hWndParent = ::GetParent(m_hWnd);
    ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
    return 0;
  } else if (m_ActionOnCancel == CLOSE_MYSELF) {
    AnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);
    return 0;
  }

  // Start cancelling the worker thread
  g_ErrorReportSender.Cancel();

  // Disable Cancel button
  m_btnCancel.EnableWindow(0);

  return 0;
}

void CProgressDlg::Start(BOOL bCollectInfo, BOOL bMakeVisible) {
  if (bCollectInfo) {
    CString sCaption;
    sCaption.Format(Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
        "ProgressDlg"), _T("DlgCaption2")), g_CrashInfo.m_sAppName);
    SetWindowText(sCaption);
  } else {
    CString sCaption;
    sCaption.Format(Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
        "ProgressDlg"), _T("DlgCaption")), g_CrashInfo.m_sAppName);
    SetWindowText(sCaption);
  }

  SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  // Center the dialog on the screen
  CenterWindow();

  if (!g_CrashInfo.m_bSilentMode && bMakeVisible) {
    ShowWindow( SW_SHOW);
    SetFocus();
  }

  if (!bCollectInfo) {
    SetTimer(1, 3000); // Hide this dialog in 3 sec.
  }

  SetTimer(0, 200); // Update this dialog each 200 ms.

  m_ActionOnCancel = DONT_CLOSE;
}

void CProgressDlg::Stop() {
  KillTimer(0);
  KillTimer(1);
}
// TODO(yesp) : 重构该函数，
//  应该使用static text取代这里的list,否则显示太多的信息不好。
//  并且整个发送过程应该是有一个进度条，而不是一个小任务就100%。
//  每个小过程应该动态更新static text的文本，并且更新进度条即可。
LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/,
    BOOL& /*bHandled*/) {
  WORD wTimerId = (WORD) wParam;

  if (wTimerId == 0) // Dialog update timer
  {
    // Get current progress
    int nProgressPct = 0;
    std::vector<CString> messages;
    g_ErrorReportSender.GetStatus(nProgressPct, messages);

    // Update progress bar
    m_prgProgress.SetPos(nProgressPct);

    int attempt = 0; // Sending attempt

    unsigned i;
    for (i = 0; i < messages.size(); i++) {
      if (messages[i].CompareNoCase(_T("[creating_dump]")) == 0) {
        m_ActionOnCancel = DONT_CLOSE;
        m_ActionOnClose = CLOSE_MYSELF_AND_PARENT;
        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T(
                "CollectingCrashInfo")));
      } else if (messages[i].CompareNoCase(_T("[copying_files]")) == 0) {
        m_ActionOnCancel = DONT_CLOSE;
        m_ActionOnClose = CLOSE_MYSELF_AND_PARENT;
        // Remove marquee style from progress bar
        m_prgProgress.ModifyStyle(PBS_MARQUEE, 0);
      } else if (messages[i].CompareNoCase(_T("[confirm_send_report]")) == 0) {
        m_ActionOnCancel = CLOSE_MYSELF_AND_PARENT;

        if (!g_CrashInfo.m_bSilentMode)
          ShowWindow( SW_HIDE);

        HWND hWndParent = ::GetParent(m_hWnd);
        ::PostMessage(hWndParent, WM_COMPLETECOLLECT, 0, 0);
      } else if (messages[i].CompareNoCase(_T("[exporting_report]")) == 0) {
        m_ActionOnCancel = DONT_CLOSE;
        m_ActionOnClose = DONT_CLOSE;
        CString sCaption;
        sCaption.Format(Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
            "ProgressDlg"), _T("DlgCaptionExport")), g_CrashInfo.m_sAppName);
        SetWindowText(sCaption);

        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T(
                "CompressingFiles")));

        m_btnCancel.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T("Cancel")));

        ShowWindow( SW_SHOW);
      } else if (messages[i].CompareNoCase(_T("[end_exporting_report_ok]"))
          == 0) {
        m_ActionOnCancel = CLOSE_MYSELF;
        ShowWindow( SW_HIDE);
      } else if (messages[i].CompareNoCase(_T("[end_exporting_report_failed]"))
          == 0) {
        m_ActionOnCancel = CLOSE_MYSELF;
        m_ActionOnClose = CLOSE_MYSELF;
        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T(
                "ExportedWithErrors")));
        m_btnCancel.EnableWindow(1);
        m_btnCancel.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T("Close")));
      } else if (messages[i].CompareNoCase(_T("[compressing_files]")) == 0) {
        m_ActionOnCancel = DONT_CLOSE;
        m_ActionOnClose = CLOSE_MYSELF;
        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T(
                "CompressingFiles")));
        m_btnCancel.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T("Cancel")));
      } else if (messages[i].CompareNoCase(_T("[end_compressing_files]")) == 0) {
        if (!g_CrashInfo.m_bSendErrorReport && g_CrashInfo.m_bStoreZIPArchives) {
          m_ActionOnCancel = CLOSE_MYSELF;
          m_ActionOnClose = CLOSE_MYSELF;
          HWND hWndParent = ::GetParent(m_hWnd);
          ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
        }
      } else if (messages[i].CompareNoCase(_T("[status_success]")) == 0) {
        m_ActionOnCancel = CLOSE_MYSELF_AND_PARENT;
        m_ActionOnClose = CLOSE_MYSELF_AND_PARENT;
        HWND hWndParent = ::GetParent(m_hWnd);
        ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
      } else if (messages[i].CompareNoCase(_T("[status_failed]")) == 0) {
        m_ActionOnCancel = CLOSE_MYSELF_AND_PARENT;
        m_ActionOnClose = CLOSE_MYSELF_AND_PARENT;
        KillTimer(1);
        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T(
                "CompletedWithErrors")));

        m_btnCancel.EnableWindow(1);
        m_btnCancel.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T("Close")));

        if (!g_CrashInfo.m_bSilentMode)
          ShowWindow( SW_SHOW);
        else {
          HWND hWndParent = ::GetParent(m_hWnd);
          ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
        }
      } else if (messages[i].CompareNoCase(_T("[exit_silently]")) == 0) {
        m_ActionOnCancel = CLOSE_MYSELF_AND_PARENT;
        m_ActionOnClose = CLOSE_MYSELF_AND_PARENT;
        KillTimer(1);
        HWND hWndParent = ::GetParent(m_hWnd);
        ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
      } else if (messages[i].CompareNoCase(_T("[cancelled_by_user]")) == 0) {
        m_statText.SetWindowText(Utility::GetINIString(
            g_CrashInfo.m_sLangFileName, _T("ProgressDlg"), _T("Cancelling")));
      } else if (messages[i].CompareNoCase(_T("[sending_attempt]")) == 0) {
        attempt++;
        CString str;
        str.Format(Utility::GetINIString(g_CrashInfo.m_sLangFileName, _T(
            "ProgressDlg"), _T("StatusText")), attempt);
        m_statText.SetWindowText(str);
      }

      int count = m_listView.GetItemCount();
      int indx = m_listView.InsertItem(count, messages[i]);
      m_listView.EnsureVisible(indx, TRUE);

    }
  } else if (wTimerId == 1) // The timer that hides this window
  {
    if (!g_CrashInfo.m_bSilentMode)
      AnimateWindow(m_hWnd, 200, AW_HIDE | AW_BLEND);
    KillTimer(1);
  }

  return 0;
}

//  这个主要是解决发送崩溃报告失败的情况。
// 但是如果是客户端软件，如果失败便应该退出，这样用户体验会比较好，
//  并且也不合适透露太多收集崩溃报告的具体细节。
LRESULT CProgressDlg::OnListRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
  LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) pnmh;

  POINT pt;
  GetCursorPos(&pt);

  CMenu popup_menu;
  popup_menu.LoadMenu(IDR_POPUPMENU);

  CMenu submenu = popup_menu.GetSubMenu(0);

  if (lpnmitem->iItem < 0) {
    submenu.EnableMenuItem(ID_MENU1_COPYSEL, MF_BYCOMMAND | MF_GRAYED);
  }

  CString sCopySelLines = Utility::GetINIString(g_CrashInfo.m_sLangFileName,
      _T("ProgressDlg"), _T("CopySelectedLines"));
  CString sCopyWholeLog = Utility::GetINIString(g_CrashInfo.m_sLangFileName,
      _T("ProgressDlg"), _T("CopyTheWholeLog"));

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_STRING;

  mii.dwTypeData = sCopySelLines.GetBuffer(0);
  mii.cch = sCopySelLines.GetLength();
  submenu.SetMenuItemInfo(ID_MENU1_COPYSEL, FALSE, &mii);

  mii.dwTypeData = sCopyWholeLog.GetBuffer(0);
  mii.cch = sCopyWholeLog.GetLength();
  submenu.SetMenuItemInfo(ID_MENU1_COPYLOG, FALSE, &mii);

  submenu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
  return 0;
}

LRESULT CProgressDlg::OnCopySel(WORD /*wNotifyCode*/, WORD /*wID*/,
    HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  CString sData;
  int i;
  for (i = 0; i < m_listView.GetItemCount(); i++) {
    DWORD dwState = m_listView.GetItemState(i, LVIS_SELECTED);
    if (dwState == 0)
      continue;

    TCHAR buf[4096];
    buf[0] = 0;
    int n = m_listView.GetItemText(i, 0, buf, 4095);
    sData += CString(buf, n);
    sData += "\r\n";
  }

  SetClipboard(sData);

  return 0;
}

LRESULT CProgressDlg::OnCopyLog(WORD /*wNotifyCode*/, WORD /*wID*/,
    HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  CString sData;
  int i;
  for (i = 0; i < m_listView.GetItemCount(); i++) {
    TCHAR buf[4096];
    buf[0] = 0;
    int n = m_listView.GetItemText(i, 0, buf, 4095);
    sData += CString(buf, n);
    sData += "\r\n";
  }

  SetClipboard(sData);

  return 0;
}

int CProgressDlg::SetClipboard(CString& sData) {
  if (OpenClipboard()) {
    EmptyClipboard();
    HGLOBAL hClipboardData;
    DWORD dwSize = (sData.GetLength() + 1) * sizeof(TCHAR);
    hClipboardData = GlobalAlloc(GMEM_DDESHARE, dwSize);
    TCHAR* pszData = (TCHAR*) GlobalLock(hClipboardData);
    if (pszData != NULL) {
      _TCSNCPY_S(pszData, dwSize/sizeof(TCHAR), sData, sData.GetLength()*sizeof(TCHAR));
      GlobalUnlock(hClipboardData);
#ifdef _UNICODE
      SetClipboardData(CF_UNICODETEXT, hClipboardData);
#else
      SetClipboardData(CF_TEXT, hClipboardData);
#endif
      CloseClipboard();
      return 0;
    }
    CloseClipboard();
  }

  return 1;
}
