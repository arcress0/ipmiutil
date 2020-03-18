#!/bin/sh
#            ipmi_if.sh
# detect IPMI Interface type, usually KCS or SSIF 
#
# Future: also include preferred Driver type on another line
#
# ifdir=/usr/share/ipmiutil
ifdir=/var/lib/ipmiutil
ifout=$ifdir/ipmi_if.txt
dmiout=/tmp/dmi.out.$$

mkdir -p $ifdir
which dmidecode  >/dev/null 2>&1
if [ $? -ne 0 ]
then
   # if no dmidecode, old, so assume KCS 
   echo "Interface type: KCS" >$ifout
   exit 0
fi
dmidecode >$dmiout
# dmidecode |grep IPMI >/dev/null 2>&1
grep IPMI $dmiout >/dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "Interface type: None" >$ifout
   exit 0
fi
iftype=`grep "Interface Type:" $dmiout |cut -f2 -d':'`
echo $iftype |grep KCS >/dev/null 2>&1
if [ $? -eq 0 ]
then
   echo "Interface type: KCS" >$ifout
else
   echo $iftype |grep "OUT OF SPEC" >/dev/null 2>&1
   if [ $? -eq 0 ]
   then
      echo "Interface type: SSIF" >$ifout
   else 
      echo "Interface type: $iftype" >$ifout
   fi
fi
# echo "IPMI `cat $ifout` interface found"

sa=`grep "I2C Slave Address:" $dmiout |cut -f2 -d':'`
echo "I2C Slave Address: $sa" >>$ifout
base=`grep "Base Address:" $dmiout |tail -n1 |cut -f2 -d':'`
echo "Base Address: $base" >>$ifout

spacing=1
spac_str=`grep "Register Spacing:" $dmiout |cut -f2 -d':'`
echo $spac_str | grep "Successive Byte" >/dev/null 2>&1
if [ $? -eq 0 ]; then
   spacing=1
else
   echo $spac_str | grep "32-bit" >/dev/null 2>&1
   if [ $? -eq 0 ]; then
      spacing=4
   else
      spacing=2
   fi
fi
echo "Register Spacing: $spacing" >>$ifout

biosver=`grep "Version: " $dmiout |head -n1 |cut -f2 -d':'`
echo "BIOS Version: $biosver" >>$ifout
exit 0
