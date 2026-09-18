@echo off
REM # buildwin64.cmd
REM #   build ipmiutil for windows 64-bit
REM #
REM # First download a copy of getopt.c getopt.h.
REM #   copy getopt.*   util
REM # Then download and build a copy of openssl for Windows,
REM # and copy the built openssl files needed to lib & inc.
REM #   copy libeay32.lib ssleay32.lib  lib
REM #   copy libeay32.dll ssleay32.dll  util
REM #   copy include\openssl\*.h   lib\lanplus\inc\openssl
REM #
REM # You should either run this from the Visual Studio Command Line, 
REM # or first run the appropriate vcvars.bat script.
set | findstr VCINSTALLDIR
if %errorlevel% EQU 1 goto vcerror

REM call vcvars64.bat 

REM TODO, prebuild checking:
REM check for getopt.c,h
REM check for ssl libs
REM check for ssl includes
REM call mkssl

set MARCH=X64
set UTMAKE=ipmiutil64.mak
REM set DSSL11=TRUE
echo %LIBPATH% 
echo %LIBPATH% |findstr /C:64 >NUL
if %errorlevel% EQU 1 goto vcerror

call cleanwin.cmd 

cd lib
nmake /nologo -f ipmilib.mak all
cd ..
REM # echo make lib done

cd util
nmake /nologo -f %UTMAKE% all
cd ..
REM # echo make util done

echo buildwin64 ipmiutil done
goto done

:vcerror
echo Either VCINSTALLDIR is missing or LIBPATH does not include 64bit
echo Need to run vcvars64.bat for 64bit from a fresh session

:done

