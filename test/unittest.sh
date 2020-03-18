#!/bin/sh 
#    unittest.sh [node_ip]
#    Basic unit test for ipmiutil functions
#
dosol=0
doserial=0
outf=/tmp/unittest.out
# default remote node for SOL test
node=192.168.1.154
mydir=`pwd`
indir=`dirname $0`
c1=`echo $indir |cut -c1`
if [ "$c1" = "/" ]; then
   testdir=$indir
else
   testdir=$mydir/$indir
fi
# datadir=/usr/share/ipmiutil
datadir=/var/lib/ipmiutil
# sfil is used only to read from with events -p -s below
# normally, sfil=$datadir/sensor-out.txt
sfil=${testdir}/sensor-TIGW1U.txt
sfil2=${testdir}/sensor-CG2100.txt 
tmpc=/tmp/cfg.tmp
tmpin=/tmp/sol.in
# march=`rpmbuild --showrc |grep " _target_cpu" | head -n1 |awk '{print $3}'`
uarch=`uname -m`
LANPARAMS=
os=`uname -s`
if [ "$os" = "SunOS" ];then
TAIL="tail -1"
TAIL2="tail -2"
else
TAIL="tail -n1"
TAIL2="tail -n2"
fi

if [ $# -ge 1 ];then 
   node=$1
   # use canned username and password, edit as needed.
   LANPARAMS="-N $node -U admin -P password"
fi

>$outf
cd /tmp
echo "# ipmiutil sel -v $LANPARAMS" |tee -a $outf
ipmiutil sel -v $LANPARAMS  >>$outf 2>&1
if [ $? -ne 0 ]; then
	echo "No IPMI support detected"
	exit 1
fi

echo "# ipmiutil health $LANPARAMS" |tee -a $outf
ipmiutil health $LANPARAMS |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil cmd $LANPARAMS -x 00 20 18 01 "  |tee -a $outf
ipmiutil cmd $LANPARAMS -x 00 20 18 01 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil fru $LANPARAMS" |tee -a $outf
ipmiutil fru $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
MODEL=`grep "Chassis OEM Field" $outf| cut -f2 -d':'| awk '{ print $1 }'`
echo "# ipmiutil config $LANPARAMS -s $tmpc" |tee -a $outf
ipmiutil config $LANPARAMS -s $tmpc 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil lan $LANPARAMS " |tee -a $outf
ipmiutil lan $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil sel $LANPARAMS" |tee -a $outf
ipmiutil sel $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil sel -e $LANPARAMS" |tee -a $outf
ipmiutil sel -e $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
echo "# ipmiutil sensor $LANPARAMS" |tee -a $outf
ipmiutil sensor $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
# ipmiutil discover -a -b 10.243.42.255 |tee -a $outf
echo "# ipmiutil discover -a" |tee -a $outf
ipmiutil discover -a 2>&1  |tee -a $outf
$TAIL2 $outf |grep  responses >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
# Test getevt SEL method
echo "# ipmiutil getevt -s $LANPARAMS -t 3 " |tee -a $outf
ipmiutil getevt -s $LANPARAMS -t 3 2>&1 |tee -a $outf
rv=$?
# $TAIL $outf |grep  successful >/dev/null
if [ $rv -ne 0 ]; then
   echo "igetevt error $rv"
   exit 1
fi

if [ "x$LANPARAMS" = "x" ]; then
# if local, also test getevt MessageBuffer method
echo "# ipmiutil getevt $LANPARAMS -t 3 " |tee -a $outf
ipmiutil getevt $LANPARAMS -t 3 2>&1 |tee -a $outf
rv=$?
# $TAIL $outf |grep  successful >/dev/null
if [ $rv -ne 0 ]; then
   echo "igetevt error $rv"
   exit 1
fi
fi

if [ "$uarch" = "ia64" ]
then
   # Note that ireset -n causes a real reset on ia64 (?)
   echo "skipping ia64 ipmiutil reset" |tee -a $outf
else
   # Use hwreset -n to send an NMI.
   echo "# ipmiutil reset $LANPARAMS -n" |tee -a $outf
   ipmiutil reset $LANPARAMS -n 2>&1 |tee -a $outf
   $TAIL $outf |grep  successful >/dev/null
   if [ $? -ne 0 ]; then
	exit 1
   fi
fi

# Might not do SOL test if no remote server is configured for SOL.
if [ $dosol -eq 1 ]; then
   echo "# ipmiutil sol -d -N $node " |tee -a $outf
   ipmiutil sol -d -N $node 2>&1 |tee -a $outf
   cat - <<%%% >$tmpin

root
password
ls
pwd
echo success
exit
~.
%%%
   echo "# ipmiutil sol -a -N $node -i $tmpin " |tee -a $outf
   ipmiutil sol -a -N $node -i $tmpin  2>&1 |tee -a $outf
   $TAIL $outf |grep  successful >/dev/null
   if [ $? -ne 0 ]; then
	exit 1
   fi
fi

echo "# ipmiutil events 18 00 02 02 00 00 00 20 00 04 09 01 6f 44 0f ff " |tee -a $outf
ipmiutil events 18 00 02 02 00 00 00 20 00 04 09 01 6f 44 0f ff |tee -a $outf
$TAIL2 $outf |grep  "AC Lost" >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

echo "# ievents -p -s $sfil B3 E8 00 0E 0C C7 1B A0  11 08 12 7F 10 90 FF FF 20 20 00 20 02 15 01 41  0F FF " |tee -a $outf
ievents -p -s $sfil B3 E8 00 0E 0C C7 1B A0  11 08 12 7F 10 90 FF FF 20 20 00 20 02 15 01 41  0F FF |tee -a $outf
$TAIL2 $outf |grep  "Redundancy Lost" >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

echo "# ipmiutil events -s $sfil2 -p  B5 19 00 15 17 C6 C9 D0 00 02 B2 76 C1 16 FF FF 20 20 10 20 20 00 00 52 1D 20 00 00 00 00 00 00 00 00 00 00 00 00 C1 00 00 00 00 00 00 00 00 00 00" |tee -a $outf
ipmiutil events -s $sfil2 -p B5 19 00 15 17 C6 C9 D0 00 02 B2 76 C1 16 FF FF 20 20 10 20 20 00 00 52 1D 20 00 00 00 00 00 00 00 00 00 00 00 00 C1 00 00 00 00 00 00 00 00 00 00 |tee -a $outf
$TAIL2 $outf |grep  "Lo Crit thresh" >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi
 
echo "# ievents 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04" |tee -a $outf
ievents 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04 |tee -a $outf
$TAIL2 $outf |grep  DIMM >/dev/null
if [ $? -ne 0 ]; then
   echo "DIMM event FAIL"
   exit 1
fi

echo "# ipmiutil events -d 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04" |tee -a $outf
ipmiutil events -d 40 78 02 37 86 41 4e 20 00 04 0c 08 6f 20 ff 04 |tee -a $outf
$TAIL2 $outf |grep  DIMM >/dev/null
if [ $? -ne 0 ]; then
   echo "DIMM event FAIL"
   exit 1
fi

echo "# ipmiutil wdt $LANPARAMS" |tee -a $outf
ipmiutil wdt $LANPARAMS 2>&1 |tee -a $outf
$TAIL $outf |grep  successful >/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

if [ $doserial -eq 1 ]; then
 # some platforms do not support IPMI serial channels
 echo "# ipmiutil serial $LANPARAMS"  |tee -a $outf
 ipmiutil serial $LANPARAMS 2>&1 |tee -a $outf
 $TAIL $outf |grep  successful >/dev/null
 if [ $? -ne 0 ]; then
	exit 1
 fi
fi

# The model can be used to detect if the chassis has an alarm panel. 
echo "MODEL=$MODEL" |tee -a $outf
case "$MODEL" in
  TIGW1U) tamok=1
	;;
  TIGH2U) tamok=1
	;;
  TIGI2U) tamok=1
	;;
  TIGPR2U) tamok=1
	;;
  TIGPT1U) tamok=1
	;;
  TSRLT2) tamok=1
	;;
  TSRMT2) tamok=1
	;;
  CG2100) tamok=1
	;;
  *) tamok=0
	;;
esac
# Run it anyway, the alarms command has better detection now.
tamok=1
if [ $tamok -eq 1 ]; then
   echo "# ipmiutil alarms $LANPARAMS" |tee -a $outf
   ipmiutil alarms $LANPARAMS  2>&1 |tee -a $outf
   $TAIL $outf |grep  successful >/dev/null
   if [ $? -ne 0 ]; then
	exit 1
   fi
fi
cd $mydir

