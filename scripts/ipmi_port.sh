#! /bin/sh
#
# ipmi_port         
#
# chkconfig: 345 91 07
# description: ipmi_port reserves the IPMI RMCP port 623 from portmap
#		which prevents portmap from trying to use port 623.
# 		This script also applies any saved IPMI thresholds, and
#		copies the bmclanpet.mib to where SNMP can find it.
#		It is recommended that this service be started for
#		all systems with ipmiutil that also have portmap.
#
### BEGIN INIT INFO
# Provides: ipmi_port
# Required-Start: $local_fs $network $remote_fs
# Required-Stop:  $local_fs $network $remote_fs
# Default-Start: 3 4 5 3 4 5  
# Default-Stop:  0 1 2 6  0 1 2 6   
# Short-Description: ipmi_port reserves the RMCP port from portmap
# Description: ipmi_port is used to reserve the RMCP port from portmap
### END INIT INFO
#
#if [ -f /etc/init.d/functions ]; then
# Source function library.
#. /etc/init.d/functions
#fi

name=ipmi_port
progdir=/usr/sbin
prog="$progdir/$name"
lockfile=/var/lock/subsys/$name
portmap=/etc/init.d/portmap
shardir=/usr/share
mibdir=$shardir/snmp/mibs
datadir=$shardir/ipmiutil
vardir=/var/lib/ipmiutil
ifout=${vardir}/ipmi_if.txt
sensorout=${vardir}/sensor_out.txt
threshout=${vardir}/thresh_out.txt
# This threshold script could be created by ipmiutil sensor -p ...
thresh="${vardir}/thresholds.sh"

getpid () {
    # This is messy if the parent script is same name as $1
    p=`ps -ef |grep "$1" |grep -v grep |awk '{print $2}'`
    echo $p
}

start()
{
	echo -n "Starting $name: "
	echo
	retval=1
	PID=0
        
	mkdir -p $vardir
	# if [ ! -f $ifout ]; then
	   # ${datadir}/ipmi_if.sh
	# fi 
	if [ ! -f $sensorout ]; then
	   # Capture a snapshot of IPMI sensor data for later reuse.
	   ipmiutil sensor -q >$sensorout
	fi
	if [ -f $thresh ]
	then
	   # apply saved IPMI sensor thresholds, if any
	   sh $thresh >$threshout 2>&1
	fi
	if [ -d ${mibdir} ]
	then
	   # put bmclanpet MIB where SNMP can find it
	   cp -f $datadir/bmclanpet.mib ${mibdir}/BMCLAN-PET-MIB.txt
	fi

	dpc=`getpid dpcproxy`
	if [ "x${dpc}" != "x" ]
	then
	   echo "$name: dpcproxy is already running on port 623,"
	   echo "so $name is not needed."
	   retval=6
	else
	   [ -x $portmap ] || exit 6
	   [ -x $prog ] || exit 5
	   $prog -b
	   retval=$?
	   if [ $retval -eq 0 ]; then
	        PID=`getpid "$prog -b"`
		echo $PID >$lockfile
	   fi
	fi
	echo
	return $retval
}

stop()
{
	echo -n "Stopping $name: "
	echo
        retval=1
	if [ -f $lockfile ]; then
           p=`cat $lockfile`
           if [ "x$p" = "x" ]; then
	        p=`getpid "$prog -b"`
           fi
           if [ "x$p" != "x" ]; then
                kill $p
                retval=$?
           fi
        fi
	echo
	if [ -d ${mibdir} ]; then
	   rm -f ${mibdir}/BMCLAN-PET-MIB.txt
	fi
	[ $retval -eq 0 ] && rm -f $lockfile
	return $retval
}

restart()
{
	stop
	start
}

rh_status() {
    if [ -f $lockfile ]; then
        p=`cat $lockfile`
        if [ "x$p" != "x" ]; then
           pid=`getpid $p`
           if [ "x$pid" != "x" ]; then
              echo "$name (pid $pid) is running..."
              retval=0
           else
              echo "$name is dead but $lockfile exists"
              retval=1
	   fi
        else
           echo "$name $lockfile exists but is empty"
           retval=2
        fi
    else
        echo "$name is stopped"
        retval=3
    fi
    return $retval
}

rh_status_q() {
    rh_status >/dev/null 2>&1
}

if [ ! -d /var/lock/subsys ]; then
   lockfile=/var/run/$name.pid
fi

case "$1" in
  start)
        rh_status_q && exit 0
	start
	;;
  stop)
        rh_status_q || exit 0
	stop
	;;
  status)
	rh_status 
	;;
  restart)
	restart
	;;
  reload)
        rh_status_q || exit 7
	restart
	;;
  force_reload)
	restart
	;;
  condrestart|try-restart)
	rh_status_q || exit 0
	restart
	;;
  *)
	echo "Usage: $0 {start|stop|status|restart|condrestart|try-restart|reload|force-reload}"
	exit 2
esac

exit $?

