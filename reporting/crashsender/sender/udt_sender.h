//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#ifndef UDT_SENDER_H_
#define UDT_SENDER_H_
#include "abstract_sender.h"

//  这个使用udt协议进行发送。
class UDTSender : public AbstractSender {
public:
  UDTSender(SendData* data, AssyncNotification* assync);
  ~UDTSender();
  virtual bool Send();
};


#endif //  UDT_SENDER_H_