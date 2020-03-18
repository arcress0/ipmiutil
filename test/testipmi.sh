#!/bin/sh 
#    testipmi.sh [node_ip]
#    Main test script for ipmiutil functions
# Other files used:
#    ./ipmievt.sh
#    ./sensor-TIGW1U.txt
#    ./sensor-CG2100.txt 
# 
# If a remote node is specified as the target, it must be confiugred for 
# IPMI LAN and should have the ipmiutil_asy service running.
#
outf=/tmp/testipmi.out
remote=0
dosol=0
# default remote node for SOL test
node=192.168.134.52
# use canned username and password, edit as needed.
user=admin
pswd=password
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
evtsh=${testdir}/ipmievt.sh
tmpc=/tmp/cfg.tmp
tmpin=/tmp/sol.in
npass=0
nfail=0
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

errexit() {
   rv=$1
   echo "FAIL $rv"
   nfail=`expr $nfail + 1`
   echo "##  Passed=$npass, Failed=$nfail, see $outf for detailed results"
   exit $rv
}

waitready() {
   i=0
   while [ 1 ]; do
      if [ $i -gt 15 ]; then
         return 1
      fi
      echo "wait for ready: loop $i ..."
      ipmiutil wdt $LANPARAMS
      if [ $? -eq 0 ]; then
         return 0
      else
         sleep 1
      fi
      i=`expr $i + 1`
   done
}

runcmd() {
   # runcmd checks for the tag 'successful' in the output and exits otherwise.
   CMD="$*"
   TAG=`echo $CMD |cut -f1-3 -d' '`
   tmpcmd=/tmp/cmdout.$$
   echo -n "$TAG ... "
   echo "# $CMD" > $tmpcmd
   $CMD >>$tmpcmd 2>&1
   rv=$?
   $TAIL $tmpcmd | grep successful >/dev/null
   success=$?
   cat $tmpcmd >>$outf
   if [ $success -ne 0 ]; then
	if [ $rv -eq 0 ]; then 
	   rv=1
	fi
	echo "FAIL $rv"
	echo "=== $CMD FAIL $rv" >>$outf
	nfail=`expr $nfail + 1`
   else
	echo "PASS"
	echo "=== $CMD PASS" >>$outf
	npass=`expr $npass + 1`
   fi
   return $rv
}

runcmdr() {
   # runcmdr checks only the return value for 0, and exits otherwise.
   CMD="$*"
   TAG=`echo $CMD |cut -f1-3 -d' '`
   tmpcmd=/tmp/cmdoutr.$$
   echo -n "$TAG ... "
   echo "# $CMD" > $tmpcmd
   $CMD >>$tmpcmd 2>&1
   rv=$?
   cat $tmpcmd >>$outf
   if [ $rv -ne 0 ]; then 
	echo "FAIL $rv"
	echo "=== $CMD FAIL $rv" >>$outf
	nfail=`expr $nfail + 1`
   else
	echo "PASS"
	echo "=== $CMD PASS" >>$outf
	npass=`expr $npass + 1`
   fi
   return $rv
}

if [ $# -ge 1 ];then 
   node=$1
   LANPARAMS="-N $node -U $user -P $pswd"
   remote=1
   dosol=1
fi

>$outf
pushd /tmp

echo "##  testipmi.sh $LANPARAMS" | tee -a $outf
if [ ! -f $sfil ]; then
   echo "Cannot find required file $sfil"
   echo "Make sure that $sfil, $sfil2, and $evtsh are present."
   exit 1
fi
if [ ! -f $sfil2 ]; then
   echo "Cannot find required file $sfil2"
   echo "Make sure that $sfil, $sfil2, and $evtsh are present."
   exit 1
fi
if [ ! -x $evtsh ]; then
   echo "Cannot find required file $evtsh"
   echo "Make sure that $sfil, $sfil2, and $evtsh are present."
   exit 1
fi

runcmd "ipmiutil health $LANPARAMS"
runcmd "ipmiutil health -x $LANPARAMS"
runcmd "ipmiutil health -c $LANPARAMS"
runcmd "ipmiutil health -f -g -h -s -c $LANPARAMS"

runcmd "ipmiutil cmd -x $LANPARAMS 00 20 18 01 "
runcmdr "ipmiutil cmd -q $LANPARAMS 00 20 18 01 "

runcmd "ipmiutil fru -b $LANPARAMS"
MODEL=`grep "Chassis OEM Field" $outf| cut -f2 -d':'| awk '{ print $1 }'`
ASSET=`grep "Product Asset Tag" $outf| cut -f2 -d':'| awk '{ print $1 }'`
runcmd "ipmiutil fru $LANPARAMS"
runcmd "ipmiutil fru -c $LANPARAMS"
runcmd "ipmiutil fru -i 00 -m 002000s $LANPARAMS"
runcmd "ipmiutil fru -a test_asset $LANPARAMS" -V4
runcmd "ipmiutil fru -a ${ASSET} $LANPARAMS" -V4

runcmd "ipmiutil config $LANPARAMS -s $tmpc"

runcmd "ipmiutil lan -c $LANPARAMS " 
IPADDR=`grep "IP address" $outf| tail -n1|cut -f2 -d'|'| awk '{ print $1 }'`
if [ "x$IPADDR" = "x0.0.0.0" ]; then
   # if not already configured, use value from node variable.
   IPADDR=$node
fi
if [ $remote -eq 0 ]; then
   runcmd "ipmiutil lan -e -I $IPADDR -u $user -p $pswd $LANPARAMS " 
fi
runcmd "ipmiutil lan $LANPARAMS " 
if [ $remote -eq 0 ]; then
   # restore previous IPMI LAN settings
   runcmd "ipmiutil config $LANPARAMS -r $tmpc"
fi

ipmiutil serial | grep "No serial channel" >/dev/null 2>&1
if [ $? -ne 0 ]; then
   runcmd "ipmiutil serial $LANPARAMS"
   runcmd "ipmiutil serial -c $LANPARAMS"
fi

tmpsel=/tmp/selout.$$
runcmd "ipmiutil sel $LANPARAMS"
runcmd "ipmiutil sel -e $LANPARAMS"
runcmd "ipmiutil sel -v $LANPARAMS"
runcmd "ipmiutil sel -l5 $LANPARAMS"
echo -n "ipmiutil sel -r"
ipmiutil sel -r -l20 $LANPARAMS >$tmpsel
if [ $? -ne 0 ]; then
   echo "... FAIL $rv"  |tee -a $outf
   nfail=`expr $nfail + 1`
fi
echo " ... PASS"  |tee -a $outf
npass=`expr $npass + 1`
runcmd "ipmiutil sel -f $tmpsel $LANPARAMS "

runcmd "ipmiutil sensor $LANPARAMS"
runcmd "ipmiutil sensor -v $LANPARAMS"
runcmd "ipmiutil sensor -g temp,fan,voltage $LANPARAMS"
runcmd "ipmiutil sensor -c $LANPARAMS"
if [ $remote -eq 0 ]; then
   runcmd "$evtsh"
fi

# Test getevt SEL method
runcmdr "ipmiutil getevt -s $LANPARAMS -t 3 " 
if [ $remote -eq 0 ]; then
   # if local, also test getevt MessageBuffer method
   runcmdr "ipmiutil getevt $LANPARAMS -t 3 " 
fi

# Do not run SOL test if no remote server is configured for SOL.
if [ $dosol -eq 1 ]; then
   # runcmd "ipmiutil sol -d $LANPARAMS "  (do not check success)
   echo -n "ipmiutil sol -d ... "
   echo "# ipmiutil sol -d $LANPARAMS " >> $outf
   ipmiutil sol -d $LANPARAMS >>$outf 2>&1
   if [ $? -eq -3 ]; then
	echo "FAIL"
	echo "=== ipmiutil sol -d FAIL" >>$outf
	nfail=`expr $nfail + 1`
   else
	echo "PASS"
	echo "=== ipmiutil sol -d PASS" >>$outf
	npass=`expr $npass + 1`
   fi
   
   cat - <<%%% >$tmpin

root
password
ls
pwd
echo success
exit
~.
%%%
   runcmd "ipmiutil sol -a $LANPARAMS -i $tmpin" 
fi


runcmd "ipmiutil wdt $LANPARAMS"
runcmd "ipmiutil wdt -e -a0 -t 5 -p 2 $LANPARAMS"
runcmd "ipmiutil wdt -c $LANPARAMS"
runcmd "ipmiutil wdt -r $LANPARAMS"
sleep 5
runcmd "ipmiutil wdt -d $LANPARAMS"
runcmd "ipmiutil sel -l5 $LANPARAMS"

runcmdr "ipmiutil discover -a"

CMD="ipmiutil events 18 00 02 02 00 00 00 20 00 04 09 01 6f 44 0f ff"
echo -n "ipmiutil events (AC Lost)"
echo "# $CMD" >>$outf
$CMD >>$outf
$TAIL2 $outf |grep  "AC Lost" >/dev/null
if [ $? -ne 0 ]; then
   echo " ... FAIL 1"  |tee -a $outf
   nfail=`expr $nfail + 1`
fi
echo " ... PASS"  |tee -a $outf
npass=`expr $npass + 1`

CMD="ievents -p -s $sfil B3 E8 00 0E 0C C7 1B A0  11 08 12 7F 10 90 FF FF 20 20 00 20 02 15 01 41  0F FF"
echo -n "ievents -p (Redundancy Lost)"
echo "# $CMD" >>$outf
$CMD >>$outf
$TAIL2 $outf |grep  "Redundancy Lost" >/dev/null
if [ $? -ne 0 ]; then
   echo " ... FAIL 1"  |tee -a $outf
   nfail=`expr $nfail + 1`
fi
echo " ... PASS"  |tee -a $outf
npass=`expr $npass + 1`

CMD="ipmiutil events -s $sfil2 -p  B5 19 00 15 17 C6 C9 D0 00 02 B2 76 C1 16 FF FF 20 20 10 20 20 00 00 52 1D 20 00 00 00 00 00 00 00 00 00 00 00 00 C1 00 00 00 00 00 00 00 00 00 00" 
echo -n "ipmiutil events -p (Lo Crit)"
echo "# $CMD" >>$outf
$CMD >>$outf
$TAIL2 $outf |grep  "Lo Crit thresh" >/dev/null
if [ $? -ne 0 ]; then
   echo " ... FAIL 1"  |tee -a $outf
   nfail=`expr $nfail + 1`
fi
echo " ... PASS"  |tee -a $outf
npass=`expr $npass + 1`

# The alarms command will fail on systems without an alarm panel.
echo "MODEL=$MODEL" >> $outf
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
if [ $tamok -eq 1 ]; then
   picmg=0
   runcmd "ipmiutil alarms $LANPARAMS"
   runcmdr "ipmiutil alarms -m1 -n1 $LANPARAMS" 
   runcmdr "ipmiutil alarms -o $LANPARAMS" 
fi
runcmd "ipmiutil alarms -i10 $LANPARAMS"

# The rackmount servers with TAM are not PICMG, 
# so if the have TAM, skip the PICMG test.
# otherwise try a PICMG test.
if [ $tamok -eq 0 ]; then
   picmg=1
   CMD="ipmiutil picmg $LANPARAMS properties"
   echo -n "$CMD"
   echo "# $CMD" >>$outf
   $CMD >>$outf
   rv=$?
   if [ $rv -eq 193 ]; then
	echo " ... N/A"
	picmg=0
   elif [ $? -ne 0 ]; then
	echo " ... FAIL $rv"  |tee -a $outf
	nfail=`expr $nfail + 1`
   else
	echo " ... PASS"  |tee -a $outf
	npass=`expr $npass + 1`
   fi
fi

# The firmware firewall feature is only for PICMG
if [ $picmg -eq 1 ]; then
   CMD="ipmiutil firewall $LANPARAMS info"
   echo -n "$CMD"
   echo "# $CMD" >>$outf
   $CMD >>$outf
   rv=$?
   if [ $rv -ne 0 ]; then
	echo " ... FAIL $rv"  |tee -a $outf
	nfail=`expr $nfail + 1`
   else
	echo " ... PASS"  |tee -a $outf
	npass=`expr $npass + 1`
   fi
fi

if [ "$uarch" = "ia64" ]
then
   # Note that ireset -n (NMI) causes a full reset on ia64 
   echo "ia64, so skip ipmiutil reset -n" |tee -a $outf
else
   # Use hwreset -n to send an NMI.
   runcmd "ipmiutil reset -n $LANPARAMS " 
fi
if [ $remote -eq 1 ]; then
   # reset if the system is remote
   runcmd "ipmiutil reset -o  $LANPARAMS "
   # after soft-reboot, could do waitready here instead
   sleep 80
   runcmd "ipmiutil reset -r -w $LANPARAMS " 
   sleep 1
   runcmd "ipmiutil reset -c -w $LANPARAMS " 
   sleep 1
   runcmd "ipmiutil reset -d -w $LANPARAMS " 
   sleep 1
   runcmd "ipmiutil reset -w -u $LANPARAMS " 
   # if the system isn't fully down yet, the up request may be ignored.
   sleep 1
   runcmd "ipmiutil reset -u  $LANPARAMS " 
   # wait for init to complete
   sleep 80
   # Should be up now, but some systems take a bit longer for 
   # everything to come back up.  So we do this last.
fi

echo "##  Passed=$npass, Failed=$nfail, see $outf for detailed results"
# remove temp files (tmpcmdr=/tmp/cmdoutr.$$ tmpcmd=/tmp/cmdout.$$)
rm -f $tmpc  $tmpin  /tmp/cmdout* /tmp/selout*
popd
exit 0

