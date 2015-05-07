a new crash report system for windows,based on crashrpt.

please see http://code.google.com/p/crashrpt/ for crashrpt'details.

我在crashrpt的基础上进行修改。主要的修改方向如下：

1，编写详细的文档，做深入浅出的理论阐述。

2，对UI的良好支持，使得界面更加美观，可定制性更强，更容易集成到
> 用户开发的Windows客户端软件中。

3，对代码进行较大幅度的重构，使用glog, gtest ,gflags,curl等库简化代码：

> a,删除crash\_report\_sender部分繁琐的代码，

> b,废弃对wtl的依赖。

> c,废弃crash\_report\_annalyzer部分繁琐的commandline解析部分，
> > 删除可读性很低的output\_document部分代码。


> d,废弃对SMTP，SMAPI等的支持，因为在客户端发送到服务器并不合适使用邮件进行发送。
> > 对sender部分进行重构，使得发送这个传输层的细节有一个公共的接口，
> > 可以直接多协议发送，并支持对新的传输协议的扩展。
4，增强了部分功能：
> > （1）crash\_report\_annalyzer支持导出文件格式的html格式，
> > > 使用基类，支持输出格式的扩展。

> > （2）crash\_report增加了CheckHealthyBeforeStart  接口，用于检测软件运行的健康状况。
> > > 让应用程序可以去处理上次的崩溃(比如可以提示，可以检测日志，
> > > 检查加载的模块版本兼容性等)
> > > (3) error\_report\_sender.h/cc进行了重构，支持更多的发送方式，如果需要增加自己的发送方法，
> > > > 只需要继承AbstractSender类,实现自己的发送方式即可。不需要对代码进行太多了解和修改。

> > > (4) 增加了UDTSender ,SimpleTcpSender,其中使用SimpleTcpSender可以在不设置apache环境的
> > > > 情况下，学习体验崩溃报告的整个流程。


