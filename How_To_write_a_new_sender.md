# Introduction #

为了能够支持自定义的发送方法，将Http方式发送崩溃报告的相关代码进行了重构，这样应用程序开发者可以方便地设计自己的发送方法，并集成到崩溃报告系统中,而修改的代码量最小化。

# Details #
之前考虑将要发送的所有数据封装在一个class 中，然后设计一个callback function definition,开发者只需要在自己的应用程序中实现自己的发送回调函数即可，而不必修改crash\_report本身。
但是这种设计方案是不可行的，因为crash\_report\_sender是一个独立的exe,独立的进程，
回调函数的代码并不可能夸进程调用。(一个是应用程序进程，一个是crash\_report\_sender.exe进程).

现在的设计如下：
struct SendData {
> CString filename;
> CString url;
> int ip;
> int port;
> std::string appname;
> std::string appversion;
> std::string crashguid;
> std::string description;
> std::string md5;
};

class AbstractSender {
> public:
> AbstractSender(SendData**data, AssyncNotification** assync);
> ~AbstractSender();
> static AbstractSender**GenerateSender(int send\_method,
> > SendData** data,
> > AssyncNotification**assync);

> virtual bool Send() = 0;
public:
> SendData** data**;
> AssyncNotification_assync_;
};**

这样设计的好处是，可以将发送数据全部封装起来，扩展者需要的构造函数参数比较简单,只要一个
SendData**data就可以操作所有崩溃报告的相关数据了。
sender在收集完崩溃报告数据后，传递指针该sender class，交由virtual function Send()去发送数据，成功与否由返回值确定。
之前CR\_HTTP的两种方法现在被封装为以下两个类：
HttpMutilpartSender
HttpBase64Sender
这样代码的可读性和可维护性都所有提高。**

如果用户需要编写自己的Sender class，可以参考SimpleTcpSender类。
现在的http协议基本可以了，用户可以实现自己的发送协议，去满足应用程序对安全性
等方面的特殊需求。如果编写了新的子类，则需要在GenerateSender中添加对应的代码。