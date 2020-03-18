#!/bin/sh
#    init.sh - run this at install time if rpm postinstall not used
#
sbindir=/usr/bin
vardir=/var/lib/ipmiutil
datadir=/usr/share/ipmiutil
sensorout=$vardir/sensor_out.txt

  if [ -x $datadir/setlib.sh ]; then
     $datadir/setlib.sh 
  fi

  mkdir -p $vardir
  if [ ! -f $vardir/ipmi_if.txt ]; then
      $datadir/ipmi_if.sh
  fi

  # Run some ipmiutil command to see if any IPMI interface works.
  $sbindir/ipmiutil wdt >/dev/null 2>&1
  IPMIret=$?

  # If IPMIret==0, the IPMI cmd was successful, and IPMI is enabled locally.
  if [ $IPMIret -eq 0 ]; then
     # IPMI_IS_ENABLED, so enable services
     if [ -x /sbin/chkconfig ]; then
	/sbin/chkconfig --add ipmi_port
	/sbin/chkconfig --add ipmiutil_wdt
	/sbin/chkconfig --add ipmiutil_asy 
     fi

     # Capture a snapshot of IPMI sensor data once now for later reuse.
     if [ ! -f $sensorout ]; then
        $sbindir/ipmiutil sensor -q >$sensorout
     fi
  fi

