
set | findstr VCINSTALLDIR
if %errorlevel% EQU 1 goto vcerror

cd util
nmake -f ipmi_sample.mak all
cd ..
goto done

:vcerror
echo Need to first run vcvars.bat

:done
