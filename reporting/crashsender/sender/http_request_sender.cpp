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
#include <algorithm>
#include "http_request_sender.h"
#include "base/base64.h"
#include "base/md5.h"
#include "base/utility.h"
#include "base/strconv.h"

CHttpRequestSender::CHttpRequestSender() {
  m_sBoundary = _T("AaB03x5fs1045fcc7");

  m_sTextPartHeaderFmt = _T(
      "--%s\r\nContent-disposition: form-data; name=\"%s\"\r\n\r\n");
  m_sTextPartFooterFmt = _T("\r\n");
  m_sFilePartHeaderFmt = _T("--%s\r\nContent-disposition: form-data;"
                      _T(" name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n")
                      _T("Content-Transfer-Encoding: binary\r\n\r\n"));
  m_sFilePartFooterFmt = _T("\r\n");
}

BOOL CHttpRequestSender::SendAssync(CHttpRequest& Request,
    AssyncNotification* an) {
  // Copy parameters
  m_Request = Request;
  m_Assync = an;

  // Create worker thread
  HANDLE hThread = CreateThread(NULL, 0, WorkerThread, (void*) this, 0, NULL);
  if (hThread == NULL)
    return FALSE;

  return TRUE;
}

DWORD WINAPI CHttpRequestSender::WorkerThread(VOID* pParam) {
  CHttpRequestSender* pSender = (CHttpRequestSender*) pParam;
  // Delegate further actions to CHttpRequestSender class
  pSender->InternalSend();

  return 0;
}

BOOL CHttpRequestSender::InternalSend() {
  BOOL bStatus = FALSE; // Resulting status
  strconv_t strconv; // String conversion
  HINTERNET hSession = NULL; // Internet session
  HINTERNET hConnect = NULL; // Internet connection
  HINTERNET hRequest = NULL; // Handle to HTTP request
  TCHAR szProtocol[512]; // Protocol
  TCHAR szServer[512]; // Server name
  TCHAR szURI[1024]; // URI
  DWORD dwPort = 0; // Port
  CString sHeaders = _T("Content-type: multipart/form-data, boundary=")
      + m_sBoundary;
  LPCTSTR szAccept[2] = { _T("*/*"), NULL };
  INTERNET_BUFFERS BufferIn;
  BYTE pBuffer[2048];
  BOOL bRet = FALSE;
  DWORD dwBuffSize = 0;
  CString sMsg;
  LONGLONG lPostSize = 0;
  std::map<CString, std::string>::iterator it;
  std::map<CString, CHttpRequestFile>::iterator it2;

  bRet = CalcRequestSize(lPostSize);
  if (!bRet) {
    goto cleanup;
  }
  hSession = InternetOpen(_T("CrashRpt"),
      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (hSession == NULL) {
    goto cleanup;
  }

  // Parse application-provided URL
  ParseURL(m_Request.m_sUrl, szProtocol, 512, szServer, 512, dwPort, szURI,
      1024);

  hConnect = InternetConnect(hSession, // InternetOpen handle
      szServer, // Server  name
      (WORD) dwPort, // Default HTTPS port - 443
      NULL, // User name
      NULL, //  User password
      INTERNET_SERVICE_HTTP, // Service
      0, // Flags
      0 // Context
      );
  if (hConnect == NULL) {
    goto cleanup;
  }

  if (m_Assync->IsCancelled()) {
    goto cleanup;
  }

  DWORD dwFlags = INTERNET_FLAG_NO_CACHE_WRITE;
  if (dwPort == INTERNET_DEFAULT_HTTPS_PORT)
    dwFlags |= INTERNET_FLAG_SECURE; // Use SSL

  hRequest = HttpOpenRequest(hConnect, _T("POST"), szURI, NULL, NULL, szAccept,
      dwFlags, 0);
  if (!hRequest) {
    goto cleanup;
  }

  BufferIn.dwStructSize = sizeof(INTERNET_BUFFERS); // Must be set or error will occur
  BufferIn.Next = NULL;
  BufferIn.lpcszHeader = sHeaders;
  BufferIn.dwHeadersLength = sHeaders.GetLength();
  BufferIn.dwHeadersTotal = 0;
  BufferIn.lpvBuffer = NULL;
  BufferIn.dwBufferLength = 0;
  BufferIn.dwBufferTotal = (DWORD) lPostSize; // This is the only member used other than dwStructSize
  BufferIn.dwOffsetLow = 0;
  BufferIn.dwOffsetHigh = 0;

  m_dwPostSize = (DWORD) lPostSize;
  m_dwUploaded = 0;

  if (!HttpSendRequestEx(hRequest, &BufferIn, NULL, 0, 0)) {
    goto cleanup;
  }

  // Write text fields
  for (it = m_Request.m_aTextFields.begin(); it
      != m_Request.m_aTextFields.end(); it++) {
    bRet = WriteTextPart(hRequest, it->first);
    if (!bRet)
      goto cleanup;
  }

  // Write attachments
  for (it2 = m_Request.m_aIncludedFiles.begin(); it2
      != m_Request.m_aIncludedFiles.end(); it2++) {
    bRet = WriteAttachmentPart(hRequest, it2->first);
    if (!bRet)
      goto cleanup;
  }

  bRet = WriteTrailingBoundary(hRequest);
  if (!bRet)
    goto cleanup;

  if (!HttpEndRequest(hRequest, NULL, 0, 0)) {
    goto cleanup;
  }

  InternetReadFile(hRequest, pBuffer, 2048, &dwBuffSize);
  pBuffer[dwBuffSize] = 0;
  if (atoi((const char*) pBuffer) != 200) {
    goto cleanup;
  }
  bStatus = TRUE;
  cleanup:
  // Clean up
  if (hRequest)
    InternetCloseHandle(hRequest);
  if (hConnect)
    InternetCloseHandle(hConnect);
  if (hSession)
    InternetCloseHandle(hSession);

  m_Assync->SetCompleted(bStatus ? 0 : 1);
  return bStatus;
}

BOOL CHttpRequestSender::WriteTextPart(HINTERNET hRequest, CString sName) {
  BOOL bRet = FALSE;

  /* Write part header */
  CString sHeader;
  bRet = FormatTextPartHeader(sName, sHeader);
  if (!bRet) {
    return FALSE;
  }

  strconv_t strconv;
  const char* pszHeader = strconv.t2a(sHeader);
  if (pszHeader == NULL) {
    return FALSE;
  }

  DWORD dwBytesWritten = 0;
  bRet = InternetWriteFile(hRequest, pszHeader, (DWORD) strlen(pszHeader),
      &dwBytesWritten);
  if (!bRet) {
    return FALSE;
  }
  UploadProgress(dwBytesWritten);

  /* Write form data */

  std::map<CString, std::string>::iterator it = m_Request.m_aTextFields.find(
      sName);
  if (it == m_Request.m_aTextFields.end()) {
    return FALSE;
  }

  size_t nDataSize = it->second.length();
  int pos = 0;
  DWORD dwBytesRead = 0;
  for (;;) {
    if (m_Assync->IsCancelled()) {
      return FALSE;
    }

    dwBytesRead = (DWORD)min(1024, nDataSize-pos);
    if (dwBytesRead == 0)
      break; // EOF

    std::string sBuffer = it->second.substr(pos, dwBytesRead);

    DWORD dwBytesWritten = 0;
    bRet = InternetWriteFile(hRequest, sBuffer.c_str(), dwBytesRead,
        &dwBytesWritten);
    if (!bRet) {
      return FALSE;
    }
    UploadProgress(dwBytesWritten);

    pos += dwBytesRead;
  }

  /* Write part footer */

  CString sFooter;
  bRet = FormatTextPartFooter(sName, sFooter);
  if (!bRet) {
    return FALSE;
  }

  const char* pszFooter = strconv.t2a(sFooter);
  if (pszFooter == NULL) {
    return FALSE;
  }

  bRet = InternetWriteFile(hRequest, pszFooter, (DWORD) strlen(pszFooter),
      &dwBytesWritten);
  if (!bRet) {
    return FALSE;
  }
  UploadProgress(dwBytesWritten);

  return TRUE;
}

BOOL CHttpRequestSender::WriteAttachmentPart(HINTERNET hRequest, CString sName) {
  BOOL bRet = FALSE;

  /* Write part header */
  CString sHeader;
  bRet = FormatAttachmentPartHeader(sName, sHeader);
  if (!bRet) {
    return FALSE;
  }

  strconv_t strconv;
  const char* pszHeader = strconv.t2a(sHeader);
  if (pszHeader == NULL) {
    return FALSE;
  }

  DWORD dwBytesWritten = 0;
  bRet = InternetWriteFile(hRequest, pszHeader, (DWORD) strlen(pszHeader),
      &dwBytesWritten);
  if (!bRet) {
    return FALSE;
  }
  UploadProgress(dwBytesWritten);

  /* Write attachment data */

  std::map<CString, CHttpRequestFile>::iterator it =
      m_Request.m_aIncludedFiles.find(sName);
  if (it == m_Request.m_aIncludedFiles.end()) {
    return FALSE;
  }

  CString sFileName = it->second.m_sSrcFileName.GetBuffer(0);
  HANDLE hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, NULL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  BYTE pBuffer[1024];
  DWORD dwBytesRead = 0;
  for (;;) {
    if (m_Assync->IsCancelled())
      return FALSE;

    bRet = ReadFile(hFile, pBuffer, 1024, &dwBytesRead, NULL);
    if (!bRet) {
      CloseHandle(hFile);
      return FALSE;
    }

    if (dwBytesRead == 0)
      break; // EOF

    DWORD dwBytesWritten = 0;
    bRet = InternetWriteFile(hRequest, pBuffer, dwBytesRead, &dwBytesWritten);
    if (!bRet) {
      return FALSE;
    }
    UploadProgress(dwBytesWritten);
  }

  CloseHandle(hFile);

  /* Write part footer */

  CString sFooter;
  bRet = FormatAttachmentPartFooter(sName, sFooter);
  if (!bRet) {
    return FALSE;
  }

  const char* pszFooter = strconv.t2a(sFooter);
  if (pszFooter == NULL) {
    return FALSE;
  }

  bRet = InternetWriteFile(hRequest, pszFooter, (DWORD) strlen(pszFooter),
      &dwBytesWritten);
  if (!bRet) {
    return FALSE;
  }
  UploadProgress(dwBytesWritten);

  return TRUE;
}

BOOL CHttpRequestSender::WriteTrailingBoundary(HINTERNET hRequest) {
  BOOL bRet = FALSE;

  CString sText;
  bRet = FormatTrailingBoundary(sText);
  if (!bRet)
    return FALSE;

  strconv_t strconv;
  const char* pszText = strconv.t2a(sText);
  if (pszText == NULL)
    return FALSE;

  DWORD dwBytesWritten = 0;
  bRet = InternetWriteFile(hRequest, pszText, (DWORD) strlen(pszText),
      &dwBytesWritten);
  if (!bRet)
    return FALSE;

  UploadProgress(dwBytesWritten);

  return TRUE;
}

BOOL CHttpRequestSender::FormatTextPartHeader(CString sName, CString& sPart) {
  std::map<CString, std::string>::iterator it = m_Request.m_aTextFields.find(
      sName);
  if (it == m_Request.m_aTextFields.end())
    return FALSE;

  sPart.Format(m_sTextPartHeaderFmt, m_sBoundary, it->first);

  return TRUE;
}

BOOL CHttpRequestSender::FormatTextPartFooter(CString sName, CString& sText) {
  sText = m_sTextPartFooterFmt;
  return TRUE;
}

BOOL CHttpRequestSender::FormatAttachmentPartHeader(CString sName,
    CString& sText) {
  std::map<CString, CHttpRequestFile>::iterator it =
      m_Request.m_aIncludedFiles.find(sName);
  if (it == m_Request.m_aIncludedFiles.end())
    return FALSE;

  sText.Format(m_sFilePartHeaderFmt, m_sBoundary, it->first,
      Utility::GetFileName(it->second.m_sSrcFileName),
      it->second.m_sContentType);
  return TRUE;
}

BOOL CHttpRequestSender::FormatAttachmentPartFooter(CString sName,
    CString& sText) {
  sText = m_sFilePartFooterFmt;
  return TRUE;
}

BOOL CHttpRequestSender::FormatTrailingBoundary(CString& sText) {
  sText.Format(_T("--%s--\r\n"), m_sBoundary);
  return TRUE;
}

BOOL CHttpRequestSender::CalcRequestSize(LONGLONG& lSize) {
  lSize = 0;

  // Calculate summary size of all text fields included into request
  std::map<CString, std::string>::iterator it;
  for (it = m_Request.m_aTextFields.begin(); it
      != m_Request.m_aTextFields.end(); it++) {
    LONGLONG lPartSize;
    BOOL bCalc = CalcTextPartSize(it->first, lPartSize);
    if (!bCalc)
      return FALSE;
    lSize += lPartSize;
  }

  // Calculate summary size of all files included into report
  std::map<CString, CHttpRequestFile>::iterator it2;
  for (it2 = m_Request.m_aIncludedFiles.begin(); it2
      != m_Request.m_aIncludedFiles.end(); it2++) {
    LONGLONG lPartSize;
    BOOL bCalc = CalcAttachmentPartSize(it2->first, lPartSize);
    if (!bCalc)
      return FALSE;
    lSize += lPartSize;
  }

  CString sTrailingBoundary;
  FormatTrailingBoundary(sTrailingBoundary);
  lSize += sTrailingBoundary.GetLength();

  return TRUE;
}

BOOL CHttpRequestSender::CalcTextPartSize(CString sName, LONGLONG& lSize) {
  lSize = 0;

  CString sPartHeader;
  BOOL bFormat = FormatTextPartHeader(sName, sPartHeader);
  if (!bFormat)
    return FALSE;
  lSize += sPartHeader.GetLength();

  std::map<CString, std::string>::iterator it = m_Request.m_aTextFields.find(
      sName);
  if (it == m_Request.m_aTextFields.end())
    return FALSE;

  lSize += it->second.length();

  CString sPartFooter;
  bFormat = FormatTextPartFooter(sName, sPartFooter);
  if (!bFormat)
    return FALSE;
  lSize += sPartFooter.GetLength();

  return TRUE;
}

BOOL CHttpRequestSender::CalcAttachmentPartSize(CString sName, LONGLONG& lSize) {
  lSize = 0;

  CString sPartHeader;
  BOOL bFormat = FormatAttachmentPartHeader(sName, sPartHeader);
  if (!bFormat)
    return FALSE;
  lSize += sPartHeader.GetLength();

  std::map<CString, CHttpRequestFile>::iterator it =
      m_Request.m_aIncludedFiles.find(sName);
  if (it == m_Request.m_aIncludedFiles.end())
    return FALSE;

  CString sFileName = it->second.m_sSrcFileName.GetBuffer(0);
  HANDLE hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, NULL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  LARGE_INTEGER lFileSize;
  BOOL bGetSize = GetFileSizeEx(hFile, &lFileSize);
  if (!bGetSize) {
    CloseHandle(hFile);
    return FALSE;
  }

  lSize += lFileSize.QuadPart;
  CloseHandle(hFile);

  CString sPartFooter;
  bFormat = FormatAttachmentPartFooter(sName, sPartFooter);
  if (!bFormat)
    return FALSE;
  lSize += sPartFooter.GetLength();

  return TRUE;
}

void CHttpRequestSender::UploadProgress(DWORD dwBytesWritten) {
  m_dwUploaded += dwBytesWritten;

  float progress = 100 * (float) m_dwUploaded / m_dwPostSize;
  m_Assync->SetProgress((int) progress, false);
}

// Parses URL. This method's code was taken from 
// http://www.codeproject.com/KB/IP/simplehttpclient.aspx
void CHttpRequestSender::ParseURL(LPCTSTR szURL, LPTSTR szProtocol,
    UINT cbProtocol, LPTSTR szAddress, UINT cbAddress, DWORD &dwPort,
    LPTSTR szURI, UINT cbURI) {
  cbURI;
  cbAddress;
  cbProtocol;

  DWORD dwPosition = 0;
  BOOL bFlag = FALSE;

  while (_tcslen(szURL) > 0 && dwPosition < _tcslen(szURL) && _tcsncmp((szURL
      + dwPosition), _T(":"), 1))
    ++dwPosition;

  if (!_tcsncmp((szURL + dwPosition + 1), _T("/"), 1)) { // is PROTOCOL
    if (szProtocol) {
      _TCSNCPY_S(szProtocol, cbProtocol, szURL, dwPosition);
      szProtocol[dwPosition] = 0;
    }
    bFlag = TRUE;
  } else { // is HOST
    if (szProtocol) {
      _TCSNCPY_S(szProtocol, cbProtocol, _T("http"), 4);
      szProtocol[5] = 0;
    }
  }

  DWORD dwStartPosition = 0;

  if (bFlag) {
    dwStartPosition = dwPosition += 3;
  } else {
    dwStartPosition = dwPosition = 0;
  }

  bFlag = FALSE;
  while (_tcslen(szURL) > 0 && dwPosition < _tcslen(szURL) && _tcsncmp(szURL
      + dwPosition, _T("/"), 1))
    ++dwPosition;

  DWORD dwFind = dwStartPosition;

  for (; dwFind <= dwPosition; dwFind++) {
    if (!_tcsncmp((szURL + dwFind), _T(":"), 1)) { // find PORT
      bFlag = TRUE;
      break;
    }
  }

  if (bFlag) {
    TCHAR sztmp[256] = _T("");
    _TCSNCPY_S(sztmp, 256, (szURL+dwFind+1), dwPosition-dwFind);
    dwPort = _ttol(sztmp);
    int len = dwFind - dwStartPosition;
    _TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len] = 0;
  } else if (!_tcscmp(szProtocol, _T("https"))) {
    dwPort = INTERNET_DEFAULT_HTTPS_PORT;
    int len = dwPosition - dwStartPosition;
    _TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len] = 0;
  } else {
    dwPort = INTERNET_DEFAULT_HTTP_PORT;
    int len = dwPosition - dwStartPosition;
    _TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len] = 0;
  }

  if (dwPosition < _tcslen(szURL)) {
    // find URI
    int len = (int) (_tcslen(szURL) - dwPosition);
    _TCSNCPY_S(szURI, cbURI, (szURL+dwPosition), len);
    szURI[len] = 0;
  } else {
    szURI[0] = 0;
  }

  return;
}

