#!/bin/sh
linux_type="redhat"
# pkg=panicsel
# isel=showsel
# ipef=pefconfig
# iserial=tmconfig
pkg=ipmiutil
isel=isel
ipef=ilan
iserial=iserial
iselout=/tmp/isel.out
ipefout=/tmp/pefconfigoutput

basic_test()
{
	$isel > /dev/null 2>&1
	if [ "$?" -eq 0 ];then
		echo "PASS_BASIC: The $isel tool can run correctly"
		return 0
	else
		echo "FAIL_BASIC: The $isel tool can not run correctly"
		return 1
	fi
}
showsel_ov()
{
	$isel -v|grep version|grep -v BMC > /dev/null
	if [ "$?" -eq "0" ];then 
		$isel -v|grep "Free" > /dev/null 2>&1
		if [ "$?" -eq 0 ];then
			echo "PASS: $isel option -v testing"
			return 0
		else
			echo "FAIL: $isel testing -v fail"
		fi
	else
		echo "FAIL: $isel testing -v fail"
		return 1
	fi
}

showsel_oc()
{
	message_original=`$isel|wc -l`
	$isel -c > /dev/null 2>&1
	message_now=`$isel|wc -l`
	if [ "$message_original" -ge "$message_now" ];then
		echo "PASS: $isel option -c testing"
		return 0
	else
		echo "FAIL: $isel option -c testing"
		return 1
	fi
}

check_os()
{
	if [ -d "/usr/src/redhat" ];then
		linux_type="redhat"
	fi 
	if [ -d "/usr/src/hardhat" ];then
		linux_type="hardhat"
	fi
}

check_driver()
{
	lsmod|grep "ipmi_comb" > /dev/null 2>&1
	if [ $? -eq 0 ];then
		echo "The ipmi_comb driver has been inserted already"
       		Major=`cat /proc/devices|grep imb|awk '{print $1}'`
		if [ ! -c /dev/imb ];then
                    		mknod /dev/imb c $Major 0
       		fi
		return 1
	fi
	modprobe "ipmi_comb" > /dev/null 2>&1
	if [ $? -eq  0 ];then
		echo "The ipmi_comb driver be inserted corretly"
	else
		# echo "There are no ipmi driver or ipmi driver can not be inserted correctly"
		return 1
	fi
		
	lsmod|grep "ipmi_comb" > /dev/null 2>&1
       	if [ $? -eq 0 ];then
       		Major=`cat /proc/devices|grep imb|awk '{print $1}'`
		if [ ! -c /dev/imb ];then
                    		mknod /dev/imb c $Major 0
       		fi
	fi

}
showsel_ow()
{
	cp -f  /var/log/messages /tmp/tempbackup
	if [  ! -d /usr/share/$pkg ];then
		mkdir /usr/share/$pkg
	fi
	if [ -f /usr/share/$pkg/sel.idx ];then
		rm -f sel.idx	
	fi
	$isel -w > /dev/null 2>&1
	idx=`cat /usr/share/$pkg/sel.idx |awk '{ print $2}'`
	diff /var/log/messages /tmp/tempbackup > /tmp/diff_log
	log=`cat /tmp/diff_log|grep $idx`
	if [ -n "$idx"  -a  -n "$log" ];then
		echo "PASS: $isel option w"	
	else
		echo "FAIL: $isel option w"
	fi
		
}
###########################################
#test show OS critical message correctly###
###########################################
showsel_os()
{
	$isel  > $iselout
	osnumber=`grep "OS Critical Stop" $iselout|wc -l`
	if [ $osnumber -eq 0 ];then
		echo "SKIP: $isel option s - no OS critical messages"
		return 1
	fi
	$isel -s >$iselout
	osnumber=`grep "OS Critical Stop" $iselout|wc -l`
	if [ $? -eq 0 -a  $osnumber -gt 0 ];then
		echo " PASS: $isel option s "
		return 0
	else
		echo "FAIL: $isel option s"
		return 1 
	fi

}
showsel_test()
{
	check_os
	check_driver
	basic_test
	if [ "$?" -eq 0 ];then
		showsel_ov
		showsel_oc
		showsel_ow
		showsel_os	
	fi
}
#####################################################################
#test pefconfig can read PEF entry                                   ###
#####################################################################
pefconfig_basic()
{
	if [ -f $ipefout ];then
		rm -rf $ipefout	
	fi;
	$ipef -r > $ipefout 2>&1
	pefnumber=`grep PEFilter $ipefout|wc -l`
	lannumber=`grep Lan $ipefout|wc -l`
	if [ $pefnumber -ge 12 ];then
		echo  "PASS: $ipef basic requirement "
		return 0
	else
		echo "FAIL: $ipef basic requirement"
		return 1
	fi;
	if [ -f $ipefout ];then
		rm -rf $ipefout	
	fi;
}

#####################################################################
#test pefconfig can enable or disable the new PEF 0x20 events        ###
#####################################################################
pefconfig_od()
{
	ipefdout=/tmp/pefconfigoutputd
	if [ -f $ipefdout ];then
		rm $ipefdout
	fi
	$ipef -d > $ipefdout 2>&1
	disable=`cat $ipefdout|grep "Access = Disabled"`
	if [ -n "$disable" ];then
		echo "PASS:  pefconfig disable OS critical Event "
		return 0
	else
		echo "FAIL:  pefconfig disable OS critical Event "
		return 1
	fi
}

#####################################################################
#test pefconfig can write new entry at different offset than 12      ###
#####################################################################
ts_pefconfign()
{
total=`$ipef -r | grep PEFilter | wc -l`
total=`echo $((total))`
PASS="PASS"
tmp_num=$1
num=`echo $((tmp_num))`
peftmp=/tmp/ipefn
if [ "$num" -le "$total" ];then
        $ipef -n $num |grep PEFilter|tail -n $num > ${peftmp}_$num
        $ipef -r |grep PEFilter|head -n $num > ${peftmp}_r$num
        dif=`diff ${peftmp}_$num ${peftmp}_r$num`
        if [ -n "$dif" ];then
                PASS="FAIL"
        fi
        rm -f ${peftmp}_$num
        rm -f ${peftmp}_r$num
else
        succnum=`$ipef e -x -n $num |grep SetPefEntry|awk '{print $2}'|grep "successful"|wc -l`
        succnum=`echo $((succnum))`
        eqnum=`expr $num \* 2 - 1`
        if [ "$succnum" -eq  "$eqnum" ];then
                lastrecord=`$ipef -n 15 |grep PEFilter|tail -n 1|awk '{print $3}'`
                if [ $lastrecord -ne "80" ];then
                        PASS="FAIL"
                fi
        fi
fi
        if [ $PASS == "$FAIL" ];then
                echo "FAIL: $ipef -n" $1
        else
                echo "PASS: $ipef -n" $1
        fi
}
#####################################################################
#test pefconfig can output more debug information                    ###
#####################################################################
ts_pefconfigx()
{
	ipefxout=/tmp/pefconfigoutputx
	if [ -f $ipefout ];then
		rm -rf $ipefout
	fi
	if [ -f $ipefxout ];then
		rm -rf $ipefxout
	fi
	$ipef -x >> $ipefxout
	test=`grep "PEF record" $ipefxout`
	if [ ! -z test ];then
		echo "PASS: $ipef parameter x "
		return 0
	else
		echo "FAIL: $ipef parameter x"
		return 1
	fi
	if [ -f $ipefxout ];then
		rm -rf $ipefxout
	fi
		
}
##############################################################
#Basic function for testing pefconfig configuration 
################################################################
get_host_ip_address()
{
	 ifconfig|grep "inet addr"|grep -v "127.0.0.1"|awk '{print $2}'|cut -d: -f2 
}
get_host_mac_address()
{
	 ifconfig|grep eth0|awk '{print $5}'
}
get_route_ip_address()
{
	 netstat -rn|grep eth0|grep UG|awk '{print $2}'
	#cat /proc/net/arp|grep -v "HW"|awk '{print $1}'
}
get_route_mac_address()
{
	 routeip=`get_route_ip_address`
	 cat /proc/net/arp|grep -v "HW"|grep $routeip|awk '{print $4}'
}
get_subnet_mask_address()
{
        ifconfig|grep Mask|grep -v "127.0.0.1"|awk '{print $4}'|cut -d: -f2
}
get_alert_ip_address()
{
        cat /etc/snmp/snmpd.conf|grep trapsink|cut -f2 -d' '
}
get_alert_mac_address()
{
        routip=get_route_ip_address
        arping -c 1 $routeip > /dev/null 2>&1
        arping -c 2 $1|grep reply|tail -1|awk '{print $5}'|cut -c2-18
}

get_pef_ip_address()
{
	cat $1|grep $2|grep -v "Param"|awk '{print $2}'|cut -d= -f2
}
get_pef_mac_address()
{
	cat $1|grep $2|grep -v "Param"|awk '{print $3}'|cut -d= -f2
}
check_pef()
{
	cat $1|grep "SetLanEntry"|grep "($2"|awk '{print $4}'
}
upper()
{
	echo $1|tr 'a-z' 'A-Z'
}
####################################################
#Test pefconfig set host ip and mac addresses
###################################################

ts_pefconfig_sethost()
{
	rm /tmp/pefconfig_host	
	ip=`get_host_ip_address`
	mac=`get_host_mac_address`
	ip=192.168.2.222
	# $ipef -I $ip -M $mac > /tmp/pefconfig_host 2>&1
	$ipef -e -I $ip  > /tmp/pefconfig_host 2>&1
	setip=`get_pef_ip_address /tmp/pefconfig_host eth0`
	setmac=`get_pef_mac_address /tmp/pefconfig_host eth0`
	setmac=`upper $setmac`
	isip=`check_pef /tmp/pefconfig_host 3` 
	ismac=`check_pef /tmp/pefconfig_host 4`
	# if [ $mac == $setmac ] && [ $ismac -eq 0 ]; then
	if [ $ip == $setip ] && [ $isip -eq 0 ] ; then
		echo "PASS: $ipef Set host ip address to $ip"
		return 0
	else
		echo "FAIL: $ipef Set host ip address"
		return 1
	fi 
}
###############################################################
##Test pefconfig set route ip and mac addresses
###############################################################
ts_pefconfig_setroute()
{
	ip=`get_route_ip_address`
	count=`get_route_ip_address`
	mac=`get_route_mac_address`
	$ipef -e -G $ip -H $mac > /tmp/pefconfig_route 2>&1
	setip=`get_pef_ip_address /tmp/pefconfig_route gateway`
        setmac=`get_pef_mac_address /tmp/pefconfig_route gateway`
	setmac=`upper $setmac`
        isip=`check_pef /tmp/pefconfig_route 12`
        ismac=`check_pef /tmp/pefconfig_route 13`
	if [ $ip == $setip ] && [ $mac == $setmac ] \
                 && [ $isip -eq 0 ] && [ $ismac -eq 0 ];then
                echo "PASS: $ipef Set route ip $ip and mac address"
		return 0
        else
                echo "FAIL: $ipef Set route ip and mac address"
        	return 1
	fi
}
get_alert_setip_address()
{
	cat $1|grep $2|awk '{print $3}'|cut -d= -f2
}
get_alert_setmac_address()
{
	 cat $1|grep $2|awk '{print $4}'|cut -d= -f2
}
ps_pefconfig_setalert()
{
	rm -f /tmp/pefconfig_alert > /dev/null 2>&1
	ip=`get_host_ip_address`
	mac=`get_host_mac_address`
 	$ipef -A $ip -B $mac > /tmp/pefconfig_alert 2>&1
 	setip=`get_alert_setip_address /tmp/pefconfig_alert alert`
 	setmac=`get_alert_setmac_address /tmp/pefconfig_alert alert`
	setmac=`upper $setmac`
	isip=`check_pef /tmp/pefconfig_alert 18` 
	ismac=`check_pef /tmp/pefconfig_alert 19`
	if [ $ip == $setip ] && [ $mac == $setmac ] \
	 && [ $isip -eq 0 ] && [ $ismac -eq 0 ];then
		 echo "PASS: $ipef Set alert" 
		 return 0
 	else
		echo "FAIL: $ipef Set Alert"
		return 1
	fi
}
###########################################################
#Test pefconfig set community 
###########################################################
ts_pefconfig_setcommunity()
{
	rm -f  /tmp/pefconfig_comm
	$ipef -A 127.0.0.1 -C private > /tmp/pefconfig_comm 2>&1
	pef_comm=`cat /tmp/pefconfig_comm|grep Community|awk '{print $5}'`
	if [ $pef_comm == "private" ];then
		echo "PASS: $ipef set community"
		return 0		
	else
		echo "FAIL: $ipef set community"
		return 1		
	fi
}
get_pef_subnet()
{
	a=`cat $1|grep Subnet|awk '{print $5}'`
	b=`cat $1|grep Subnet|awk '{print $6}'`
	c=`cat $1|grep Subnet|awk '{print $7}'`
	d=`cat $1|grep Subnet|awk '{print $8}'`
	echo $a.$b.$c.$d
}
##############################################################
#Test pefconfig set subnet mask
###############################################################
ts_pefconfig_setsubnet()
{

	subnet=`get_subnet_mask_address`;
	$ipef -S $subnet > /tmp/pefconfig_subnet 2>&1
	pef_subnet=`get_pef_subnet /tmp/pefconfig_subnet`
	if [ $subnet == $pef_subnet ];then
		echo "PASS: $ipef get sub net mask address"
		return 0		
	else
		echo "FAIL: $ipef get sub net mask address"
		return 1
	fi
}

pefconfig_test()
{
	pefconfig_basic
	if [ "$?" -eq 0 ];then 
		ts_pefconfign 9
		ts_pefconfign 12
		ts_pefconfign 16
		ts_pefconfigx
		ts_pefconfig_sethost
		ts_pefconfig_setroute
		ts_pefconfig_setsubnet
	else
		echo "System does not support pefconfig function"
	fi
}
#*****************************************************/
#Test the tmconfig util******************************/
#*****************************************************/
EntryTest()
{
	which $iserial > /dev/null 2>&1
	if [ $? -ne 0 ];then
		if [ -f /usr/share/$pkg/$iserial ];then
			cp -f /usr/share/$pkg/$iserial /usr/sbin
		else
			echo "$iserial: there is no $iserial util"
			exit 1
		fi
	fi
}
TestPara()
{
	result=`cat $1|grep $2|awk '{print $4}'|cut -d , -f 1`
	if [ "x$result" = "x" ]; then
		echo "FAIL: $iserial $2, no result"
		return 1
	fi
	if [ $result -eq 0 ];then
		echo "PASS: $iserial $2"
		return 0
	else
		echo "FAIL: $iserial $2"
		return 1
	fi 	

}
ts_tmconfigr()
{
	infoline="$iserial Test parameter r"
	EntryTest
	$iserial -r > /dev/null 2>&1
	if [ $? -eq 0 ];then
		echo  "PASS: $infoline"
		return 0
	else
		echo "FAIL: $infoline"
		return 1
	fi
}

ts_tmconfigs()
{
	infoline="$iserial Test parameter n"
	EntryTest
	$iserial -s > /tmp/tmconfigs 2>&1
	TestPara /tmp/tmconfigs "SetChanAcc(ser)" $infoline
}
ts_tmconfigup()
{
        infoline="$iserial Test parameter u and p"
        EntryTest
        $iserial -u root -p password >/dev/null 2>&1
        if [ $? -eq 0 ];then
                echo PASS: $infoline
        else
                echo FAIL: $infoline
        fi
}
LanTest()
{
        infoline="$iserial Test parameter l"
        $iserial -l > /tmp/tmconfigl
        line=`cat /tmp/tmconfigl|grep "Lan Param"|wc -l`
        if [ $line -ge 22 ];then
                echo "PASS: $infoline"
        else
                echo "FAIL: $infoline"
        fi
}
ts_tmconfigl()
{
        EntryTest
        LanTest
}

ts_tmconfign()
{
        infoline="$iserial Test parameter m0"
        EntryTest
        $iserial -m0 >/tmp/tmconfign 2>&1
        TestPara /tmp/tmconfign "SetSerialMux(System)" $infoline
}
ts_tmconfigt()
{
        infoline="$iserial Test parameter m1"
        EntryTest
        $iserial -m1 >/tmp/tmconfigt 2>&1
        TestPara /tmp/tmconfigt "SetSerialMux(BMC)" $infoline
}

tmconfig_test()
{
	ts_tmconfigr
	if [ $? -eq 0 ];then
		ts_tmconfigs
		ts_tmconfigl
		ts_tmconfign
		ts_tmconfigup
		ts_tmconfigt
	else
		echo "FAIL:Basic tmconfig testing"
	fi
}
showsel_test
pefconfig_test
tmconfig_test
