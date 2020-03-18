@echo off
REM # cleanwin.cmd
REM #   clean ipmiutil for windows
REM #

REM call cleanssl (?)

set UTMAKE=ipmiutil.mak
echo %LIBPATH% |findstr /C:amd64 >NUL
if %errorlevel% EQU 0  set UTMAKE=ipmiutil64.mak

cd lib
nmake /nologo -f ipmilib.mak clean
cd ..
REM # echo make lib done

cd util
nmake /nologo -f %UTMAKE% clean
del tmp\*.obj 2>NUL
del *.pdb 2>NUL
del *.manifest 2>NUL
cd ..
REM # echo make util done

echo cleanwin ipmiutil done
