@echo off
REM  checksel.cmd
REM  This at script runs ipmiutil sel, writing any new records to syslog, 
REM  and will then clear the SEL if free space is low.

REM  Get path from arg 0
set ipmiutildir=%~d0%~p0

cd %ipmiutildir%
cmd /c %ipmiutildir%\ipmiutil sel -w | findstr /c:"WARNING: free space" 
if errorlevel 1 goto done
cmd /c %ipmiutildir%\ipmiutil sel -d

:done

