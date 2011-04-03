//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#include "http_base64_sender.h"
#include "base/strconv.h"
#include "base/base64.h"

HttpBase64Sender::HttpBase64Sender(SendData* data, AssyncNotification* assync)
: AbstractSender(data, assync){

}
HttpBase64Sender::~HttpBase64Sender() {

}
bool HttpBase64Sender::Send() {
  if (data_->url.IsEmpty()) {
    return false;
  }

  CHttpRequest request;
  request.m_sUrl = data_->url;
  request.m_aTextFields[_T("appname")] = data_->appname;
  request.m_aTextFields[_T("appversion")] = data_->appversion;
  request.m_aTextFields[_T("crashguid")] = data_->crashguid;
  request.m_aTextFields[_T("description")] = data_->description;
  request.m_aTextFields[_T("md5")] = data_->md5;
  assync_->SetProgress(_T("Base-64 encoding file \
                          attachment, please wait..."), 1);
  std::string sEncodedData;
  int nRet = Base64EncodeAttachment(data_->filename, sEncodedData);
  if (nRet != 0) {
    return FALSE;
  }
  request.m_aTextFields[_T("crashrpt")] = sEncodedData;
  BOOL bSend = http_sender_.SendAssync(request, assync_);
  return (bool)bSend;
}

int HttpBase64Sender::Base64EncodeAttachment(CString sFileName,
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
