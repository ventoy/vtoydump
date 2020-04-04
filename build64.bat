
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

cl.exe /c src/vtoydump_windows.c /I./src /GS /GL /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /fp:precise /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_LIB" /D "_UNICODE" /D "UNICODE" /errorReport:prompt /WX- /Zc:forScope /Gd /Oi /MT /EHsc /nologo 

link.exe vtoydump_windows.obj /OUT:"vtoydump.exe" /MANIFEST /LTCG /NXCOMPAT /PDB:"vc120.pdb" /DYNAMICBASE "VirtDisk.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /DEBUG /MACHINE:X64 /OPT:REF /INCREMENTAL:NO  /SUBSYSTEM:CONSOLE /MANIFESTUAC:"level='asInvoker' uiAccess='false'"  /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 

del /q *.pdb
del /q *.manifest
del /q vtoydump_windows.obj

if not exist vtoydump.exe (
    echo Failed to build vtoydump
    exit /b 1
)
echo Build vtoydump for amd64 success ...
echo.
pause


