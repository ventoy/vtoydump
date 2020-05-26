=== XP/WinServer 2003 32位 说明===

ventoy的启动信息保存在物理内存最低1MB范围之内。在高版本Windows系统中，vtoydump使用系统API GetSystemFirmwareTable获取iBFT表信息的形式获取到。
但是在XP/WinServer 2003系统中实测这个接口无法返回信息。

因此，这里需要采取另外一种方式，即借助第三方的驱动，直接把1MB数据dump出来，然后vtoydump从dump出来的数据中解析即可。
vtoydump增加了一个 -f 参数，指定一个数据文件从里面解析。

剩下的问题就是如何把1MB内的数据dump出来的问题了。


这里使用的是一个开源的 phymem驱动，地址为：
https://www.codeproject.com/Articles/35378/Access-Physical-Memory-Port-and-PCI-Configuration

使用说明：

1.首先还是调用 vtoydump 如果能正确获取到信息就结束了。

2.如果vtoydump获取失败 (errorlevel不为0) 则
    vtoymem.exe  data.bin
    vtoydump.exe -f data.bin
    
  即先使用 vtoymem.exe dump数据，然后再利用vtoydump从dump出的数据中直接解析。
  
  注意 vtoymem.exe 和 phymem.sys 必须放在同一目录下。
  

  
