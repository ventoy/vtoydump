# vtoydump

#### Introduction
This is a tool used with "Ventoy Compatible". See [https://www.ventoy.net/en/compatible.html](https://www.ventoy.net/en/compatible.html) for detail.

#### Usage
For Linux:  
```
vtoydump [ -lL ] [ -v ]  
    none   Only print ventoy runtime data  
    -l     Print ventoy runtime data and image location table  
    -L     Only print image location table (used to generate dmsetup table)  
    -v     Verbose, print additional debug info  
```

  
For Windows:  
```
vtoydump.exe [ -m=[K/0xFFFF] ] [ -i filepath ] [ -v ]  
    none  Only print ventoy runtime data  
    -m    Mount the iso file (Not supported before Windows 8 and Windows Server 2012)  
    -i    Load .sys driver  
    -v    Verbose, print additional debug info 


Examples for -m option:
vtoydump.exe -m
Mount the iso file to any free drive letter

vtoydump.exe -m=Y
Mount the iso file to Y: drive

vtoydump.exe -m=0x7FFFF8
Select a free drive from D E F G H I J K L M N O P Q R S T U V W and use it to mount the ISO file.

```


