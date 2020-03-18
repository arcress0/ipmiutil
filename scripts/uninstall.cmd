@echo off
REM uninstall Windows ipmiutil on this system
REM uninstall the DLLs, registry entries, and stop the checksel task
set installdir=%SystemDrive%\Program Files\sourceforge\ipmiutil

"%installdir%\showselun.reg"
del /Q %SystemRoot%\system32\showsel.dll

REM remove %installdir% from the PATH
set pathkey="HKLM\System\CurrentControlSet\Control\Session Manager\Environment"
REM reg query %pathkey% /v Path |findstr Path
set NEWPATH=%PATH:;%installdir%=%
reg add %pathkey% /v Path /t REG_EXPAND_SZ /d "%NEWPATH%"
set PATH=%NEWPATH%

REM type "%installdir%\checksel.id"
FOR /F "usebackq tokens=1*" %%i IN (`type "%installdir%\checksel.id"`) DO call :SETAT %%i %%j 
goto :NEXTJ

:SETAT
REM    %1  %2  %3   %4   %5    %6   %7      %8  
REM   Added  a  new  job  with  job  ID  =    2  
echo   %1  %2  %3   %4   %5    %6   %7  %8  %9
set ATJOB=%8
echo   at %ATJOB% /DELETE
at %ATJOB% /DELETE
goto :EOF

:NEXTJ

del /Q "%installdir%\*.*" 
