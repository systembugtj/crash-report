you can use crprober as follows:

C:\Users\shunpingye\Desktop\crash-report\bin>crproberd.exe --input_file=a5b7cfec
-058b-488f-889a-cf954d169d39.zip

C:\Users\shunpingye\Desktop\crash-report\bin>crproberd.exe --input_file=a5b7cfec
-058b-488f-889a-cf954d169d39.zip --format=text

需要注意到是：
1，输出格式已经被重新修改了，text格式基本没有改变，但是Html格式现在已经使用表格
显示，阅读起来更加方便。
2，使用了一个抽象类。这样可以方便之后扩展，比如可以使用矢量图显示，可以使用xml显示等。
