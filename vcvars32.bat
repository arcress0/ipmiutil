@echo off
REM Copy saved 32bit libraries into place, if present
REM iphlpapi.lib libeay32.dll libeay32.lib ssleay32.dll ssleay32.lib
REM Modify SAVLIB32 to a directory where these libs are saved.
set SAVLIB32=lib32
IF NOT EXIST %SAVLIB32% GOTO NOSAVLIB
copy /Y %SAVLIB32%\*.lib lib
copy /Y %SAVLIB32%\*.dll util
REM (do not copy mak) copy /Y util\x32\ipmiutil.mak util
:NOSAVLIB
REM sample VCINSTALLDIR from VS 2008
set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC
IF DEFINED VCINSTALLDIR GOTO HAVEVC
IF DEFINED VSINSTALLDIR GOTO HAVEVS
GOTO HAVEDEF
:HAVEVS
set VCDIR=%VSINSTALLDIR%\VC
:HAVEVC
set VCDIR=%VCINSTALLDIR%
:HAVEDEF
REM Run MS 32bit vcvars
set VCBAT="%VCDIR%\bin\vcvars32.bat"
set VCBATDIR="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build"
set VCBAT=%VCBATDIR%\vcvars32.bat 
IF EXIST %VCBAT% GOTO RUNVC
echo "Cannot locate vcvars32.bat, please run it manually."
GOTO DONE
:RUNVC
%VCBAT%
:DONE
