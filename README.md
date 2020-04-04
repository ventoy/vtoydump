# vtoydump

#### Introduction
This is a tool used with "Ventoy Compatible". See http://www.ventoy.net/compatible.html for detail.

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
vtoydump [ -m ] [ -i filepath ] [ -v ]  
    none  Only print ventoy runtime data  
    -m    Mount the iso file (Not supported before Windows 8 and Windows Server 2012)  
    -i    Load .sys driver  
    -v    Verbose, print additional debug info 
```


