//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#include "http_mutilpart_sender.h"
#include "base/strconv.h"


HttpMutilpartSender::HttpMutilpartSender(SendData* data, AssyncNotification* assync)
: AbstractSender(data, assync){

}
HttpMutilpartSender::~HttpMutilpartSender() {

}

//  如果发送成功，则返回true,发送失败返回false
bool HttpMutilpartSender::Send() {
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
  CHttpRequestFile f;
  f.m_sSrcFileName = data_->filename;
  f.m_sContentType = _T("application/zip");
  request.m_aIncludedFiles[_T("crashrpt")] = f;
  BOOL bSend = http_sender_.SendAssync(request, assync_);
  return (bool)bSend;
}
