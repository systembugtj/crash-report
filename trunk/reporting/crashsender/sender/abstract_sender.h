//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#ifndef ABSTRACT_SENDER_H_
#define ABSTRACT_SENDER_H_
#include <iostream>
#include <string>
#include "reporting/crashsender/assync_notification.h"

//  TODO(yesp) : Ӧ�ý����е����඼������һ���ļ���
//  ֮����Ҫ��ʱ�������ļ�������Ҫ�Ƴ����ļ����ɡ�
//  ���Ӧ�ó��򿪷�����Ҫʵ���Լ��ķ��ͷ�������ô����ͨ��
//  ��д�Լ���sender classȥʵ�֣�������Ҫ���ߵİ�ȫ�ԣ���Ҫ���õ����ܵȡ�
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
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send() = 0;
public:
  SendData* data_;
  AssyncNotification* assync_;
};

#endif  // ABSTRACT_SENDER_H_
