//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#ifndef TCP_SIMPLE_SENDER_H_
#define TCP_SIMPLE_SENDER_H_
#include "abstract_sender.h"

#include <string>

//  ���ʹ��tcp���з��ͣ���һ��sender�ǳ��򵥣�ֻ��Ϊ���ڱ������в���ʹ�á�
//  ��Ϊ���Ա�д�򵥵�server�Ϳ��Խ���demo��
//  �����Http֮��ģ����apache http server�Ƚ��鷳��
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
