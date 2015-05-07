# Introduction #

这篇短文简单描述下捕获main函数进入之前发生的异常的方法


# Details #
在运行时库中，最早调用的函数为
_DllMainCRTStartup_

或者
_mainCRTStartup_

之类，两个函数的源代码可以参考vc++自带的crt库源代码。
或者可以访问以下地址：
_DllMainCRTStartup_

http://hi.baidu.com/%D0%C2%D2%B0%BB%A8%D7%D4%B7%BC/blog/item/618ba677c0884b0eb051b9d3.html

_mainCRTStartup_

http://hi.baidu.com/%D0%C2%D2%B0%BB%A8%D7%D4%B7%BC/blog/item/ae38be3593627a9ea61e12d0.html

分析代码可以看出，两个函数分别注册了一个SEH异常处理器。他们分别为：
> _CppXcptFilter_

_XcptFilter_


这两个函数的实现分别为：
> _CppXcptFilter：_

http://hi.baidu.com/%D0%C2%D2%B0%BB%A8%D7%D4%B7%BC/blog/item/c3157cf06297a2baa40f52d5.html

_XcptFilter：_

http://hi.baidu.com/%D0%C2%D2%B0%BB%A8%D7%D4%B7%BC/blog/item/bd57fcde371b744194ee37d4.html

可以看出，这里的实现中并没有调用任何用户可以注册的回调函数。也就是说目前的crt实现，并不支持对main函数之前的异常的回调处理。

因此可能的实现就是，修改crt库，在 _CppXcptFilter_XcptFilter两个函数中，调用类似\_BasepCurrentTopLevelFilter的回调函数。
而应用程序开发者可以调用调用SetUnhandledExceptionFilter 的函数用于设置处理main函数之前的异常的回调函数。

不过修改crt库虽然不太麻烦，但是因为开发者的crt库都是使用VC++附带的，因为就是实现了这个功能，使用起来也比较麻烦。可能的解决方法是，开发一个新的运行时库，类似于crt或者是google的tmalloc库，增加对异常处理的支持，同时也可以增加对调试的支持，内存管理的支持，内存泄漏检测的支持等。

有关该功能，可以另外一个项目，专门实现一个用于解决内存泄露，性能测试，捕获main函数之前发生的异常等问题的开源的第三方运行时库。