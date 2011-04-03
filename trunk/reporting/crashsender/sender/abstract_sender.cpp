//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#include "abstract_sender.h"
#include "include/crash_report.h"

#include "reporting/crashsender/sender/http_mutilpart_sender.h"
#include "reporting/crashsender/sender/http_base64_sender.h"
#include "reporting/crashsender/sender/tcp_simple_sender.h"
#include "reporting/crashsender/sender/udt_sender.h"

AbstractSender::AbstractSender(SendData* data, AssyncNotification* assync) {
assync_ = assync;
data_ = data;
}
AbstractSender::~AbstractSender() {

}

AbstractSender* AbstractSender::GenerateSender(int send_method,
                                               SendData* data,
                                               AssyncNotification* assync) {
  //  use a manager to manage which sub class to generate
  switch (send_method)
  {
  case CR_HTTP_MutilPart:
    return new HttpMutilpartSender(data, assync);
    break;
  case CR_HTTP_Base64:
    return new HttpBase64Sender(data, assync);
    break;
  case CR_TCP_Demo:
    return new SimpleTcpSender(data, assync);
    break;
  case CR_UDT:
    return new UDTSender(data, assync);
    break;
    //  CR_FTP
  default:
    return NULL;
  }
}
