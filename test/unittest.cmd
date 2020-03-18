@echo off
REM #!/bin/sh 
REM #    unittest.cmd
REM #    Basic unit test for ipmiutil functions
REM #
REM # default remote node for SOL test
set node=192.168.134.52
set testdir=c:\temp
set LANPARAMS=-N %node% -U ADMIN -P ADMIN
REM set LANPARAMS=
REM # datadir=/usr/share/ipmiutil
REM # sfil is used only to read from with events -p -s below
REM # normally, sfil=%datadir%/sensor-out.txt
set outf=%testdir%\unittest.out
set sfil=%testdir%\sensor-TIGW1U.txt
set sfil2=%testdir%\sensor-CG2100.txt 
set tmpc=%testdir%\cfg.tmp
set tmpin=%testdir%\sol.in

echo "# ipmiutil health %LANPARAMS%" > %outf%
ipmiutil health %LANPARAMS% >> %outf%
   findstr /C:"health, completed successfully" %outf% 

echo "# ipmiutil cmd %LANPARAMS% -x 00 20 18 01 "   >> %outf%
ipmiutil cmd %LANPARAMS% -x 00 20 18 01    >> %outf%
   findstr /C:"cmd, completed successfully" %outf% 

echo "# ipmiutil fru %LANPARAMS%"  >> %outf%
ipmiutil fru %LANPARAMS%    >> %outf%
   findstr /C:"fru, completed successfully" %outf% 

echo "# ipmiutil config %LANPARAMS% -s %tmpc%"  >> %outf%
ipmiutil config %LANPARAMS% -s %tmpc%   >> %outf%
   findstr /C:"config, completed successfully" %outf% 

echo "# ipmiutil lan %LANPARAMS% "  >> %outf%
ipmiutil lan %LANPARAMS%    >> %outf%
   findstr /C:"lan, completed successfully" %outf% 

echo "# ipmiutil sel %LANPARAMS%"  >> %outf%
ipmiutil sel %LANPARAMS%    >> %outf%
   findstr /C:"sel, completed successfully" %outf% 

echo "# ipmiutil sel -e %LANPARAMS%"  >> %outf%
ipmiutil sel -e %LANPARAMS%    >> %outf%
   findstr /C:"sel, completed successfully" %outf% 

echo "# ipmiutil sensor %LANPARAMS%"  >> %outf%
ipmiutil sensor %LANPARAMS%    >> %outf%
   findstr /C:"sensor, completed successfully" %outf% 

REM # ipmiutil discover -a -b 10.243.42.255  >> %outf%
echo "# ipmiutil discover -a"  >> %outf%
ipmiutil discover -a     >> %outf%
   findstr /C:"responses" %outf% 

REM # Test getevt SEL method
echo "# ipmiutil getevt -s %LANPARAMS% -t 3 "  >> %outf%
ipmiutil getevt -s %LANPARAMS% -t 3    >> %outf%
echo "getevt -s errorlevel %errorlevel%" >> %outf%
REM if errorlevel

REM # if local, also test getevt MessageBuffer method
echo "# ipmiutil getevt %LANPARAMS% -t 3 "  >> %outf%
ipmiutil getevt %LANPARAMS% -t 3    >> %outf%
echo "getevt errorlevel %errorlevel%" >> %outf%
REM if errorlevel

   REM # Use hwreset -n to send an NMI.
   echo "# ipmiutil reset %LANPARAMS% -n"  >> %outf%
   ipmiutil reset %LANPARAMS% -n    >> %outf%
   findstr /C:"reset, completed successfully" %outf% 

REM # Might skip SOL test if no remote server is configured for SOL.
   echo "# ipmiutil sol -d %LANPARAMS% "  >> %outf%
   ipmiutil sol -d %LANPARAMS%    >> %outf%

   echo " " >%tmpin%
   echo "root"     >>%tmpin%
   echo "password" >>%tmpin%
   echo "ls"   >>%tmpin%
   echo "pwd"  >>%tmpin%
   echo "exit" >>%tmpin%
   echo "~."   >>%tmpin%
   echo "# ipmiutil sol -a %LANPARAMS% -i %tmpin%"  >> %outf%
   ipmiutil sol -a %LANPARAMS% -i %tmpin%    >> %outf%
   findstr /C:"sol, completed successfully" %outf% 

echo "# ipmiutil events 18 00 02 02 00 00 00 20 00 04 09 01 6f 44 0f ff "  >> %outf%
ipmiutil events 18 00 02 02 00 00 00 20 00 04 09 01 6f 44 0f ff  >> %outf%
 findstr /C:"AC Lost" %outf% 

echo "# ievents -p -s %sfil% B3 E8 00 0E 0C C7 1B A0  11 08 12 7F 10 90 FF FF 20 20 00 20 02 15 01 41  0F FF "  >> %outf%
ievents -p -s %sfil% B3 E8 00 0E 0C C7 1B A0  11 08 12 7F 10 90 FF FF 20 20 00 20 02 15 01 41  0F FF  >> %outf%
 findstr /C:"Redundancy Lost" %outf% 

echo "# ipmiutil events -s %sfil2% -p  B5 19 00 15 17 C6 C9 D0 00 02 B2 76 C1 16 FF FF 20 20 10 20 20 00 00 52 1D 20 00 00 00 00 00 00 00 00 00 00 00 00 C1 00 00 00 00 00 00 00 00 00 00"  >> %outf%
ipmiutil events -s %sfil2% -p B5 19 00 15 17 C6 C9 D0 00 02 B2 76 C1 16 FF FF 20 20 10 20 20 00 00 52 1D 20 00 00 00 00 00 00 00 00 00 00 00 00 C1 00 00 00 00 00 00 00 00 00 00  >> %outf%
 findstr /C:"Lo Crit thresh" %outf% 
 
echo "# ievents 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04"  >> %outf%
ievents 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04  >> %outf%
 findstr /C:"DIMM" %outf% 

echo "# ipmiutil events -d 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04"  >> %outf%
ipmiutil events -d 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04  >> %outf%
 findstr /C:"DIMM" %outf% 

echo "# ipmiutil wdt %LANPARAMS%"  >> %outf%
ipmiutil wdt %LANPARAMS%    >> %outf%
   findstr /C:"wdt, completed successfully" %outf% 

REM # some platforms do not support IPMI serial channels
 echo "# ipmiutil serial %LANPARAMS%"   >> %outf%
 ipmiutil serial %LANPARAMS%    >> %outf%
 findstr /C:"serial, completed successfully" %outf% 

REM # The model can be used to detect if the chassis has an alarm panel. 
REM echo "MODEL=%MODEL%"  >> %outf%
REM # Run it anyway, the alarms command has better detection now.

 echo "# ipmiutil alarms %LANPARAMS%"  >> %outf%
 ipmiutil alarms %LANPARAMS%     >> %outf%
 findstr /C:"alarms, completed successfully" %outf% 

