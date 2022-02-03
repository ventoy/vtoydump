# vtoydump

#### 1. Introduction
This is a tool used with "Ventoy Compatible". See [https://www.ventoy.net/en/compatible.html](https://www.ventoy.net/en/compatible.html) for detail.

#### 2. Usage
For Linux:  (must be run with root privileges)
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

#### 3. Build From Source
*For Linux:*   
Normally you can directly use the binraries in `bin/linux` directory (e.g. `bin/linux/x86_64/vtoydump`).  
These binaries are static built version and should work for most distros.  
Also you can build from source for your distro. Of course, `gcc` must be available before build.  
Just run `sh build.sh` to build vtoydump.   
If your OS is x86_64 then the output `vtoydump` is just for x86_64 architecture, so as for i386 and arm64.  

*For Windows:*   
Normally you can directly use the binraries in `bin/windows` directory (e.g. `bin/windows/NT6/64/vtoydump.exe`).  
Also you can build from source for your distro. Of course, `VisualStudio` must be installed before build.  
Just click the bat file (e.g. `build64_nt6.bat`) to build vtoydump.exe.   
