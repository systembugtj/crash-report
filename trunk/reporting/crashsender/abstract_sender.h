struct SendData {
  //  crash_report file ,zip format
  CString filename;
  CString url;
  //  可以设计简单的udt,tcp之类的传输协议，使用ip和端口
  int ip;
  int port;
  string appname;
  string appversion,
  string crashguid;
  string description;
  string md5;
};

class AbstractSender {
  public:
  AbstractSender();
  ~AbstractSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send() = 0;
private:
  SendData data;
};

class HttpMutilpartSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send() = 0;
private:
  SendData data;
};


class HttpBase64Sender {
public:
  AbstractSender();
  ~AbstractSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send();
private:
  SendData data;
};

//  这个使用tcp进行发送，这一个sender非常简单，只是为了在本机进行测试使用。
//  因为可以编写简单的server就可以进行demo。
//  如果是Http之类的，则搭apache http server比较麻烦。
class SimpleTcpSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send();
private:
  SendData data;
};

//  这个使用udt协议进行发送。
class UDTSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  如果发送成功，则返回true,发送失败返回false
  virtual bool Send();
private:
  SendData data;
};