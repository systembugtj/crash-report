//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#ifndef HTTP_BASE64_SENDER_H_
#define HTTP_BASE64_SENDER_H_
#include "abstract_sender.h"
#include "reporting/crashsender/sender/http_request_sender.h"

class HttpBase64Sender : public AbstractSender {
public:
  HttpBase64Sender(SendData* data, AssyncNotification* assync);
  ~HttpBase64Sender();
  virtual bool Send();
private:
  int Base64EncodeAttachment(CString sFileName, std::string& sEncodedFileData);
  CHttpRequestSender http_sender_; // Used to send report over HTTP
};

#endif // HTTP_BASE64_SENDER_H_