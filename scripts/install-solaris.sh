#!/bin/sh
#    Solaris or BSD install.sh
#
datadir=/usr/share/ipmiutil
vardir=/var/lib/ipmiutil
sbindir=/usr/bin
mandir=/usr/share/man/man8
manfiles="ialarms.8 iconfig.8 ihealth.8 ievents.8 ifru.8 igetevent.8 ireset.8 icmd.8 idiscover.8 ipmiutil.8 isol.8 ilan.8 isensor.8 isel.8 iserial.8 iwdt.8"
shfiles="checksel ipmi_if.sh ipmi_port.sh ipmiutil_wdt"
mibfiles="bmclanpet.mib"

mkdir -p $datadir
mkdir -p $vardir
mkdir -p $mandir
cp ipmiutil ievents idiscover ipmi_port  $sbindir
cp $manfiles  	 		$mandir
cp UserGuide $mibfiles $shfiles   $datadir
$sbindir/ipmiutil sensor -q >$vardir/sensor_out.txt
# Admin should set up checksel cron script also

