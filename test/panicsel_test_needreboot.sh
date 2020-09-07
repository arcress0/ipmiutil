#!/bin/sh
# pkg=panicsel
# isel=showsel
# ipef=pefconfig
# ireset=hwreset
pkg=ipmiutil
isel=isel
ipef=ilan
ireset=ireset
env()
{
	if [ -x /usr/local/bin/$isel ];then
		PATH_EXEC="/usr/local/bin"
	fi
	if [ -x /usr/sbin/$isel ];then
		PATH_EXEC="/usr/sbin"
	fi
}
check_os()
{
        if [ -d "/etc/redhat-release" ];then
                linux_type="redhat"
        fi
        if [ -d "/etc/SuSE-release" ];then
                linux_type="suse"
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
				return 0
                fi
        fi

        modprobe "ipmi_comb" > /dev/null 2>&1
        if [ $? -eq  0 ];then
                echo "The ipmi_comb driver be inserted corretly"
        else
                # echo "There are no ipmi driver or ipmi driver can not be inserted correctly"
		return 0
        fi

        lsmod|grep "ipmi_comb" > /dev/null 2>&1
        if [ $? -eq 0 ];then
                Major=`cat /proc/devices|grep imb|awk '{print $1}'`
                if [ ! -c /dev/imb ];then
                                mknod /dev/imb c $Major 0
                fi
        fi

}

prepare()
{
	env
	# check_driver
}
Test_kernelpatch()
{
  	oldnum=0
	newnum=0
	prepare
	DIR=`pwd`
	if [ -f /tmp/mark ];then
		MARK=`cat /tmp/mark`
	else
		MARK=0
	fi
	$PATH_EXEC/$ipef -A $1 -C public
	if [ $? -ne 0 ];then
		echo "Pefconfig incorrectly,pls check your machine"
		exit 1
	fi	
	cd /usr/share/$pkg
	if [ "$MARK" -ne "1" ];then
		cd $DIR
	   	cp -f ./dopanic  /usr/share/$pkg/
		echo "Init a panic for $pkg..."
		$PATH_EXEC/$isel -s|grep -c "OS Critical Stop" > /tmp/ocs_num	
		sync
		sleep 2 
		echo 1 > /tmp/mark
		cd /usr/share/$pkg
		insmod -f ./dopanic
	fi
	echo "check the result for $pkg..."
	read oldnum < /tmp/ocs_num
	$PATH_EXEC/$isel -s |grep -c "OS Critical Stop" > /tmp/ocs_num	
	read newnum < /tmp/ocs_num
	let oldnum=oldnum+1
	if [ $newnum -eq $oldnum ] 		
	then
		echo "PASS:KernelPactch:Panic sel insert record"
		return 0
	else
		echo  "FAIL: Kernel PatchPanic sel insert record"
		return 1
	fi
	rm -f  /tmp/mark
		
}
ts_hwresetr()
{
	prepare
	if [ -f /tmp/count ];then
		COUNT=`cat /tmp/count`	
	else
		COUNT=0
	fi
	if [ "$COUNT" -ne "1"  ];then
		$PATH_EXEC/$isel -c >/dev/null 2>&1   
		echo 1 > /tmp/count
		$PATH_EXEC/$ireset  		#reset the system
	fi
	$PATH_EXEC/$isel > /tmp/hwreset 2>&1
	isevent=`cat /tmp/hwreset|grep "System Event"`
	isboot=`cat /tmp/hwreset|grep "System Boot"`
	if [ -n "$isevent" ] && [ -n "$isboot" ];then
		echo  "PASS: $pkg power reset"
		return 0
	else	
		echo  "FAIL: $pkg power reset"
		return 1
	fi
	rm -f /tmp/count
}

ts_hwresetc()
{
	prepare
	if [ -f /tmp/hwresetc ];then
		MARK=`cat /tmp/hwresetc`
	else
		MARK=0
	fi
	if [ "$MARK" -ne "1" ];then
		$PATH_EXEC/$isel -c >/dev/null 2>&1   
		echo 1 > /tmp/hwresetc
		$PATH_EXEC/$ireset -c 		#reset the system
	fi

	$PATH_EXEC/$isel >/tmp/hwresetc 2>&1
	power=`cat /tmp/hwresetc|grep "Power Off/Down"`
	if [ -n "$power" ];then
		echo  "PASS: $pkg power cycle"
		return 0
	else	
		echo "FAIL: $pkg power cycle"
		return 1
	fi
	rm -f /tmp/hwresetc
}

