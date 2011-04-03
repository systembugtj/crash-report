//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#ifndef HTTP_MUTILPART_SENDER_H_
#define HTTP_MUTILPART_SENDER_H_

#include "abstract_sender.h"
#include "reporting/crashsender/sender/http_request_sender.h"

class HttpMutilpartSender : public AbstractSender {
public:
  HttpMutilpartSender(SendData* data, AssyncNotification* assync);
  ~HttpMutilpartSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send();
private:
  CHttpRequestSender http_sender_; // Used to send report over HTTP
};

#endif  // HTTP_MUTILPART_SENDER_H_
