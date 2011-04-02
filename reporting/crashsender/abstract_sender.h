#include <iostream>
#include <string>
#include "assync_notification.h"
#include "http_request_sender.h"
#include "tinyxml.h"
#include "crash_info_reader.h"

//  TODO(yesp) : 应该将所有的子类都独立成一个文件。
//  之后需要的时候加入该文件，不需要移除该文件即可。
//  如果应用程序开发者需要实现自己的发送方案，那么可以通过
//  重写自己的sender class去实现，比如需要更高的安全性，需要更好的性能等。
struct SendData {
  SendData() {
    ip = 0;
    port = 0;
  }
  SendData& operator = (const SendData& RightSides)
  {
    filename = RightSides.filename;
    url = RightSides.url;
    ip = RightSides.ip;
    port = RightSides.port;
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
  //  可以设计简单的udt,tcp之类的传输协议，使用ip和端口
  int ip;
  int port;
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

class HttpMutilpartSender : public AbstractSender {
public:
  HttpMutilpartSender(SendData* data, AssyncNotification* assync);
  ~HttpMutilpartSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send();
private:
  CHttpRequestSender http_sender_; // Used to send report over HTTP
};


class HttpBase64Sender : public AbstractSender {
public:
  HttpBase64Sender(SendData* data, AssyncNotification* assync);
  ~HttpBase64Sender();
  virtual bool Send();
private:
  int Base64EncodeAttachment(CString sFileName, std::string& sEncodedFileData);
  CHttpRequestSender http_sender_; // Used to send report over HTTP
};

//  这个使用tcp进行发送，这一个sender非常简单，只是为了在本机进行测试使用。
//  因为可以编写简单的server就可以进行demo。
//  如果是Http之类的，则搭apache http server比较麻烦。
class SimpleTcpSender : public AbstractSender {
public:
  SimpleTcpSender(SendData* data, AssyncNotification* assync);
  ~SimpleTcpSender();
  virtual bool Send();
};

//  这个使用udt协议进行发送。
class UDTSender : public AbstractSender {
public:
  UDTSender(SendData* data, AssyncNotification* assync);
  ~UDTSender();
  virtual bool Send();
};