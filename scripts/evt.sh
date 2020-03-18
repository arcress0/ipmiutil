#!/bin/sh
#      evt.sh
# A sample script to use via ipmiutil getevt -s -r /path/evt.sh
#
evtdesc=$1
log=/var/log/ipmi_evt.log

echo "$0 got IPMI event `date`"  >>$log
echo "$evtdesc"     >>$log
# Check SEVerity for anything other than INFormational.
echo $evtdesc |grep -v INF >/dev/null 2>&1
if [ $? -eq 0 ]; then
   # Could take various specific actions here, like snmptrap, or other alert.
   echo "*** Non-INF IPMI Event requiring attention ***" >>$log
   snmpdest=`grep trapsink /etc/snmp/snmpd.conf 2>/dev/null |head -n1`
   if [ "x$snmpdest" = "x" ]; then
     echo "No SNMP trapsink destination" >>$log
   else
     snmpipadr=`echo $snmpdest |awk '{ print $2 }'`
     community=`echo $snmpdest |awk '{ print $3 }'`
     snmphost=`uname -n`
     uptim=`cat /proc/uptime |cut -f1 -d' '`
     trapoid="enterprises.1"
     vboid1=".enterprises.1.1"  #trapString
     vboid2=".enterprises.1.2"  #trapSeverity
     sev=`echo $evtdesc |cut -c24-26`
     case $sev in 
	MIN)
	    tsev=2
	    ;;
	MAJ)
	    tsev=3
	    ;;
	CRT)
	    tsev=4
	    ;;
	*)
	    tsev=1
	    ;;
     esac
     snmptrap -v 1 -c $community $snmpipadr $trapoid $snmphost 6 0 $uptim  $vboid1 s "$evtdesc" $vboid2 i $tsev 
     echo "Send SNMP trap to $snmpipadr for IPMI $sev event, status=$?" >>$log
   fi
fi

