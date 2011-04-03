//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#ifndef ABSTRACT_SENDER_H_
#define ABSTRACT_SENDER_H_
#include <iostream>
#include <string>
#include "reporting/crashsender/assync_notification.h"

//  TODO(yesp) : 应该将所有的子类都独立成一个文件。
//  之后需要的时候加入该文件，不需要移除该文件即可。
//  如果应用程序开发者需要实现自己的发送方案，那么可以通过
//  重写自己的sender class去实现，比如需要更高的安全性，需要更好的性能等。
struct SendData {
  SendData() {
  }
  SendData& operator = (const SendData& RightSides)
  {
    filename = RightSides.filename;
    url = RightSides.url;
    appname = RightSides.appname;
    appversion = RightSides.appversion;
    crashguid = RightSides.crashguid;
    description = RightSides.description;
    md5 = RightSides.md5;
    return *this;
  }
  //  crash_report file ,zip format
  CString filename;
  CString url;
  std::string appname;
  std::string appversion;
  std::string crashguid;
  std::string description;
  std::string md5;
};

class AbstractSender {
  public:
  AbstractSender(SendData* data, AssyncNotification* assync);
  ~AbstractSender();
  static AbstractSender* GenerateSender(int send_method,
                                        SendData* data,
                                        AssyncNotification* assync);
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send() = 0;
public:
  SendData* data_;
  AssyncNotification* assync_;
};

#endif  // ABSTRACT_SENDER_H_
