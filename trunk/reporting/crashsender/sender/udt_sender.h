//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#ifndef UDT_SENDER_H_
#define UDT_SENDER_H_
#include "abstract_sender.h"

//  ���ʹ��udtЭ����з��͡�
class UDTSender : public AbstractSender {
public:
  UDTSender(SendData* data, AssyncNotification* assync);
  ~UDTSender();
  virtual bool Send();
};


#endif //  UDT_SENDER_H_