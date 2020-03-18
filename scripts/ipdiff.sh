# ipdiff.sh
# For non-shared BMC LAN channels, take the OS IP and add an increment,
# then run pefconfig -I to set the new BMC LAN IP.
#
# eth:  Get the OS IP from eth0 by default
# incr: Increment is 128. by default
# Customers may wish to customize the $eth and $incr parameters below to
# vary these for their network environments.
#
# This produces a series of systems with IP addresses allocated like this:
#         OS IP         BMC LAN IP
# system1 192.168.0.1   192.168.0.129
# system2 192.168.0.2   192.168.0.130
# system3 192.168.0.3   192.168.0.131
# ...
eth=eth0
incr=128
ip=`ifconfig $eth |grep "inet addr:" |cut -f2 -d':' |awk '{print $1}'`
last=`echo $ip |cut -f4 -d'.'`
first=`echo $ip |cut -f1-3 -d'.'`
sysname=`uname -n`

newlast=`expr $last + $incr`
if [ $newlast -ge 255 ]
then
   # overflow, so wrap IP (could return error instead)
   newlast=`expr $newlast - 255`
fi
newip=${first}.${newlast}
echo "$sysname: $eth IP = $ip, BMC IP = $newip"
ipmiutil lan -e -I $newip
