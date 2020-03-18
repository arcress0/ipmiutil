@echo off
REM install Windows ipmiutil on this system
REM install the DLLs, registry entries, and start the checksel task
REM Use the current directory as the starting path
set ipmiutildir=%CD%
set orgdir=%SystemDrive%\Program Files\sourceforge
set installdir=%orgdir%\ipmiutil

REM DLLs: showsel.dll, libeay32.dll, ssleay32.dll
copy *.dll %SystemRoot%\system32
%ipmiutildir%\showsel.reg

echo Copying files to "%installdir%"
mkdir "%orgdir%" 
mkdir "%installdir%"
copy "%ipmiutildir%\*.*" "%installdir%"

REM echo PATH=%PATH%
set PATH=%PATH%;%installdir%

set pathkey="HKLM\System\CurrentControlSet\Control\Session Manager\Environment"
reg add %pathkey% /v Path /t REG_EXPAND_SZ /d "%PATH%;%installdir%"
REM reg query %pathkey% /v Path

at 23:30 /every:m,t,w,th,f,s,su  "%installdir%\checksel.cmd" >"%installdir%\checksel.id"

