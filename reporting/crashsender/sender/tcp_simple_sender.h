//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#ifndef TCP_SIMPLE_SENDER_H_
#define TCP_SIMPLE_SENDER_H_
#include "abstract_sender.h"

#include <string>

//  这个使用tcp进行发送，这一个sender非常简单，只是为了在本机进行测试使用。
//  因为可以编写简单的server就可以进行demo。
//  如果是Http之类的，则搭apache http server比较麻烦。
class SimpleTcpSender : public AbstractSender {
public:
  SimpleTcpSender(SendData* data, AssyncNotification* assync);
  ~SimpleTcpSender();
  virtual bool Send();
private:
  bool ConnectToServer();
  bool SendCrashReport();
  void ReceiveAskData();
  bool SendKeyValue(std::string key, std::string value);
  SOCKET sock_;
  bool status_;
};

#endif //  TCP_SIMPLE_SENDER_H_
