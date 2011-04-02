#include <iostream>
#include <string>
#include "assync_notification.h"
#include "http_request_sender.h"
#include "tinyxml.h"
#include "crash_info_reader.h"

//  TODO(yesp) : Ӧ�ý����е����඼������һ���ļ���
//  ֮����Ҫ��ʱ�������ļ�������Ҫ�Ƴ����ļ����ɡ�
//  ���Ӧ�ó��򿪷�����Ҫʵ���Լ��ķ��ͷ�������ô����ͨ��
//  ��д�Լ���sender classȥʵ�֣�������Ҫ���ߵİ�ȫ�ԣ���Ҫ���õ����ܵȡ�
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
  //  ������Ƽ򵥵�udt,tcp֮��Ĵ���Э�飬ʹ��ip�Ͷ˿�
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
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send() = 0;
public:
  SendData* data_;
  AssyncNotification* assync_;
};

class HttpMutilpartSender : public AbstractSender {
public:
  HttpMutilpartSender(SendData* data, AssyncNotification* assync);
  ~HttpMutilpartSender();
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
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

//  ���ʹ��tcp���з��ͣ���һ��sender�ǳ��򵥣�ֻ��Ϊ���ڱ������в���ʹ�á�
//  ��Ϊ���Ա�д�򵥵�server�Ϳ��Խ���demo��
//  �����Http֮��ģ����apache http server�Ƚ��鷳��
class SimpleTcpSender : public AbstractSender {
public:
  SimpleTcpSender(SendData* data, AssyncNotification* assync);
  ~SimpleTcpSender();
  virtual bool Send();
};

//  ���ʹ��udtЭ����з��͡�
class UDTSender : public AbstractSender {
public:
  UDTSender(SendData* data, AssyncNotification* assync);
  ~UDTSender();
  virtual bool Send();
};