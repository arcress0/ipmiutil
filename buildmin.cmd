@echo off
REM # buildmin32.cmd
REM #   build ipmiutil for windows in standalone mode (minimal, 32bit)
REM #   without IPMI LAN 2.0 libraries, so no SOL.  Use for bootables.
REM #
REM # First download a copy of getopt.c getopt.h.
REM #   copy getopt.*   util
REM #

REM TODO, prebuild checking:
REM check for getopt.c,h

set | findstr VCINSTALLDIR
if %errorlevel% EQU 1 goto vcerror

echo %LIBPATH% |findstr /C:64 >NUL
if %errorlevel% EQU 0 (
	set MARCH=X64
	set UTMAKE=ipmiutil2-64.mak
) else (
	set MARCH=IX86
	set UTMAKE=ipmiutil2.mak
)

cd util
nmake /nologo -f %UTMAKE% all
cd ..

echo buildmin %UTMAKE% done
goto done

:vcerror
echo Need to first run vcvars.bat

:done

