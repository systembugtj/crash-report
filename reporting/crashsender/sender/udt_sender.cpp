//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#include "reporting/crashsender/sender/udt_sender.h"
#include "base/strconv.h"
// TODO(yesp) : ʵ�ָú���

UDTSender::UDTSender(SendData* data, AssyncNotification* assync)
: AbstractSender(data, assync){

}
UDTSender::~UDTSender() {

}
bool UDTSender::Send() {
  return false;
}
