
@echo off

REM set right vcvarsall.bat file path
set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"

for %%i in (7 8 9 10 11 12 13 14 15 16 17 18 19 20) do (    
    if exist "C:\Program Files (x86)\Microsoft Visual Studio %%i.0\VC\vcvarsall.bat" (
        set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio %%i.0\VC\vcvarsall.bat"
        goto Main
    )
)

:Main
if not exist %VCVARSALL% (
    echo Please set right vcvarsall.bat path
    exit /b 1
)

echo %VCVARSALL% amd64
call %VCVARSALL% amd64

if exist vtoydump.exe del /q vtoydump.exe

set CCOPT=/DVTOY_NT5 /I./src /I./src/fat_io_lib /GS /GL /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /fp:precise /DFATFS_INC_FORMAT_SUPPORT=0 /DVTOY_BIT=64 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_LIB" /D "_UNICODE" /D "UNICODE" /errorReport:prompt /WX- /Zc:forScope /Gd /Oi /MT /EHsc /nologo 

cl.exe /c src/vtoydump_windows.c %CCOPT%
cl.exe /c src/fat_io_lib/fat_access.c  %CCOPT%
cl.exe /c src/fat_io_lib/fat_cache.c   %CCOPT%
cl.exe /c src/fat_io_lib/fat_filelib.c %CCOPT%
cl.exe /c src/fat_io_lib/fat_format.c  %CCOPT%
cl.exe /c src/fat_io_lib/fat_misc.c    %CCOPT% 
cl.exe /c src/fat_io_lib/fat_string.c  %CCOPT%
cl.exe /c src/fat_io_lib/fat_table.c   %CCOPT% 
cl.exe /c src/fat_io_lib/fat_write.c   %CCOPT%

link.exe vtoydump_windows.obj fat_access.obj fat_cache.obj fat_filelib.obj fat_format.obj fat_misc.obj fat_string.obj fat_table.obj fat_write.obj /OUT:"vtoydump.exe" /MANIFEST /LTCG /NXCOMPAT /PDB:"vc120.pdb" /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /DEBUG /MACHINE:X64 /OPT:REF /INCREMENTAL:NO  /SUBSYSTEM:CONSOLE",5.02" /MANIFESTUAC:"level='asInvoker' uiAccess='false'"  /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 

del /q *.pdb
del /q *.manifest
del /q *.obj

if not exist vtoydump.exe (
    echo Failed to build vtoydump
    exit /b 1
)

del /q bin/windows/NT5/64/vtoydump.exe
copy /y vtoydump.exe bin/windows/NT5/64/

echo Build vtoydump for amd64 success ...
echo.
pause


