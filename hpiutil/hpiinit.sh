#!/bin/sh
#
# hpi init script to start/stop the Intel hpi library daemon
# process name: SpiLibd
#
# For RedHat and MontaVista
# chkconfig: 345 50 35
#
### BEGIN SuSE INIT INFO
# Provides: tam
# Required-Start: $network
# Default-Start:  3 5
# Default-Stop:   0 1 2 6
# Description:    tam is used to start/stop Telco Alarm Manager
### END INIT INFO
name=hpiutil
lockfile=/var/lock/subsys/$name
 
if [ ! -d /var/lock/subsys ]; then
   lockfile=/var/run/$name.pid
fi

case "$1" in 
"start")
	# Is it already started?
	ps -ef | grep -v grep | grep SpiLibd
	if [ $? -eq 0 ]
	then
	    echo "SpiLibd is already started"
	    exit 1
        fi
	SAHPI_HOME=/etc/hpi
	LD_LIBRARY_PATH=/usr/lib
	#LD_LIBRARY_PATH=$SAHPI_HOME/lib
	# Use default config locations (/etc/hpi/*.conf)
	#SPI_LIB_CFG_LOCATION=$SAHPI_HOME/spi-lib.conf
	#SPI_DAEMON_CFG_LOCATION=$SAHPI_HOME/spi-daemon.conf
	SPI_LIB_LOG_LOCATION=/var/log/spilib
	SPI_DAEMON_LOG_LOCATION=/var/log/spidaemon
	SPI_DAEMON_LOG_LEVEL=-l5
	SPI_LIB_LOG_LEVEL=-l5
#	LD_ASSUME_KERNEL=2.4.1
	# flush the logs before starting daemon
	export SAHPI_HOME LD_LIBRARY_PATH SPI_LIB_LOG_LOCATION
	export SPI_DAEMON_LOG_LOCATION SPI_DAEMON_LOG_LEVEL SPI_LIB_LOG_LEVEL
#	export SPI_LIB_CFG_LOCATION SPI_DAEMON_CFG_LOCATION
	rm -f ${SPI_DAEMON_LOG_LOCATION}.1 $SPI_LIB_LOG_LOCATION 2>/dev/null
	mv $SPI_DAEMON_LOG_LOCATION ${SPI_DAEMON_LOG_LOCATION}.1 2>/dev/null
	# see /usr/bin/SpiLibd
	SpiLibd
	touch $lockfile
	;;
"stop")
	spid=`ps -ef |greo SpiLibd | grep -v grep |awk '{ print $2 }'`
	if [ "$spid" != "" ]
	then
	   kill $spid    
	   sleep 5
	   kill -9 $spid  2>/dev/null
	fi
	rm -f $lockfile
	;;
*)
        echo "Usage: $0 start|stop"
	exit 1
	;;
esac

exit 0
