#!/bin/sh
#    ipmievt.sh - generate an IPMI temperature event
#
# Baseboard Temp is usually index 0d, 0e or 15, & sensor num 0x30
# Note that values displayed/used are in hex.
#
# sensor |grep -i "Baseboard Temp" 
# 000d SDR Full 01 39 20 a 01 snum 30 Baseboard Temp   = 2f OK   47.00 degrees C
#   ^^=ibasetemp                   ^^=nbasetemp                  ^^+4=lotemp
#
pattn="Temp"
sensorfil=/var/lib/ipmiutil/sensor_out.txt
if [ ! -f $sensorfil ]; then
   sensorfil=/usr/share/ipmiutil/sensor_out.txt
fi
# get sensor readings again, since current temp may have changed
ipmiutil sensor >$sensorfil
#ibasetemp=0d
#nbasetemp=30   # usually snum 30 on my test unit
#lotemp=33
nbasetemp=`grep -i "$pattn" $sensorfil |head -n1|awk '{print $10}'`
ibasetemp=`grep "snum $nbasetemp" $sensorfil |awk '{print $1}'`
curtemp=`grep "snum $nbasetemp" $sensorfil| cut -f2 -d'=' |awk '{print $3}'|cut -f1 -d'.'`
lotemp=`expr $curtemp + 4`
hitemp=`expr $curtemp - 4`
hinorm=`expr $curtemp + 25`

if [ "x$curtemp" = "x" ]; then
   echo "Cannot find sensor reading for /$pattn/ in $sensorfil"
   exit 1
fi

echo "Setting $pattn upper threshold to $hitemp - current temp = $curtemp"
# cause a crit-hi for Baseboard Temp
ipmiutil sensor -i $ibasetemp -t -n $nbasetemp -h $hitemp

# give it 2 secs before clearing to OK
sleep 2
# put Baseboard Temp back to OK
ipmiutil sensor -i $ibasetemp -t -n $nbasetemp -h $hinorm 

sleep 1
# show the SEL events
ipmiutil sel -l6 

