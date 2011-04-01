struct SendData {
  //  crash_report file ,zip format
  CString filename;
  CString url;
  //  ������Ƽ򵥵�udt,tcp֮��Ĵ���Э�飬ʹ��ip�Ͷ˿�
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
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send() = 0;
private:
  SendData data;
};

class HttpMutilpartSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send() = 0;
private:
  SendData data;
};


class HttpBase64Sender {
public:
  AbstractSender();
  ~AbstractSender();
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send();
private:
  SendData data;
};

//  ���ʹ��tcp���з��ͣ���һ��sender�ǳ��򵥣�ֻ��Ϊ���ڱ������в���ʹ�á�
//  ��Ϊ���Ա�д�򵥵�server�Ϳ��Խ���demo��
//  �����Http֮��ģ����apache http server�Ƚ��鷳��
class SimpleTcpSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send();
private:
  SendData data;
};

//  ���ʹ��udtЭ����з��͡�
class UDTSender {
public:
  AbstractSender();
  ~AbstractSender();
  //  ������ͳɹ����򷵻�true,����ʧ�ܷ���false
  virtual bool Send();
private:
  SendData data;
};