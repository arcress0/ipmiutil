REM ipmiutil_wdt.cmd
REM wdt          Reset the IPMI watchdog timer every NSEC seconds

set NSEC  60

:WDTLOOP
ipmiutil wdt -r
sleep %NSEC%
goto WDTLOOP

