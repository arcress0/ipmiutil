
             ------------------------------
                   ipmiutil README
	       IPMI Management Utilities
             ------------------------------

The ipmiutil IPMI utilities below allow the user to access the firmware 
System Event Log and configure the Platform Event Filter table for the new 
'OS Critical Stop' records, as well as other common IPMI system management
functions.  

ipmiutil       - a meta-command to invoke all of the below as sub-commands
ievents        - a standalone utility to interpret IPMI and PET event data
isel           - show/set the firmware System Event Log records
isensor        - show Sensor Data Records, sensor readings, and thresholds
ireset         - cause the BMC to hard reset or power down the system    
ilan           - show and configure the BMC LAN port and Platform Event Filter
                 table to allow BMC LAN alerts from firmware events and 
                 OS Critical Stop messages,
iserial        - show and configure the BMC Serial port for various modes, 
                 such as Terminal Mode.  
ifru           - show the FRU chassis, board, and product inventory data, 
                 and optionally write a FRU asset tag.
ialarms	       - show and set front panel alarms (LEDs and relays)
iwdt	       - show and set watchdog timer parameters
igetevent      - receive any IPMI events and display them
ihealth        - check and report the basic health of the IPMI BMC
iconfig        - list/save/restore the BMC configuration parameters
icmd           - send specific IPMI commands to the BMC,
                 mainly for testing and debug purposes.
idiscover      - discover the available IPMI LAN nodes on a subnet
isol           - start/stop an IPMI Serial-Over-LAN Console session
ipicmg         - show/set the IPMI PICMG parameters
ifirewall      - show/set the IPMI firmware firewall configuration
iekanalyzer    - run FRU-EKeying analyzer on FRU files
ifwum          - OEM firmware update manager extensions
ihpm           - HPM firmware update manager extensions
isunoem        - Sun OEM functions
idelloem       - Dell OEM functions
itsol          - Tyan SOL console start/stop session
idcmi          - get/set DCMI parameters, if supporting the DCMI spec

Other supporting files:
checksel       = cron script using ipmiutil sel to check the SEL, write new 
		 events to the OS system log, and clear the SEL if nearly full.
ipmi_port      = daemon to bind the RMCP port and sleep to prevent 
                 Linux portmap from stealing the RMCP port
ipmi_port.sh   = init script to reserve the RMCP port from portmap, 
		 this also restores saved sensor thresholds, if any.
ipmiutil_wdt   = init script to restart watchdog timer every 60 sec via cron
ipmiutil_asy   = init script runs 'ipmiutil getevt -a' for remote shutdown
ipmiutil_evt   = init script runs 'ipmiutil getevt -s' for monitoring events
evt.sh         = sample script which can be invoked by ipmiutil_evt
ipmi_if.sh     = script using dmidecode to determine the IPMI Interface Type
bmclanpet.mib  = SNMP MIB for BMC LAN Platform Event Traps
test/*         = scripts and utilities used in testing ipmiutil/panicsel
kern/*         = kernel patches for panic handling

--------------------
   Dependencies:
--------------------
The IPMI Management Utilities currently work with platforms that 
support the IPMI 1.5 or 2.0 specification.   IPMI servers can be managed 
locally, or remotely via IPMI LAN, even when the OS or main CPU is not 
functional.

The IPMI 1.5 spec, Table 36-3 defines the sensor types for SEL records, 
as used by showsel.
The IPMI 1.5 spec, Table 15-2 defines the Platform Event Filter table
entries, as used by pefconfig.
The IPMI 1.5 spec, Table 19-4 defines the LAN Configuration Parameters,
as used by pefconfig.

The ipmiutil utilities will use an IPMI Driver, either the Intel IPMI package
(ipmidrvr, /dev/imb), MontaVista OpenIPMI (/dev/ipmi0), the valinux IPMI 
Driver (/dev/ipmikcs), or the LANDesk ldipmi daemon.  The ipmiutil utilities 
can also use direct user-space I/Os in Linux or FreeBSD if no IPMI driver 
is detected.

If ipmiutil is compiled with LANPLUS enabled, then it does depend upon
libcrypto.so, which is provided by the openssl package.

------------------------
   Build Instructions
------------------------

See notes in the INSTALL file from the ipmiutil*.tar.gz archive.

------------------------
   IPMI Configuration
------------------------
See http://ipmiutil.sourceforge.net/docs/UserGuide

Note that ipmiutil can autodetect the IPMI interface using SMBIOS/dmi 
and use driverless KCS or SSIF if no IPMI driver is loaded.

Various vendor IPMI firmware versions should support all of the 
mandatory IPMI functions, but there are variations in which of
the optional IPMI functions that are supported.

The 'ipmiutil lan' utility can be used to easily set up a working 
configuration of BMC LAN, SOL, and PEF Alerting while Linux is running,  
instead of a series of 40-50 commands with ipmitool, or a proprietary
vendor tool, which may even require booting to DOS.

The bmc_panic patch has been obsoleted by the CONFIG_IPMI_PANIC option
in the OpenIPMI driver, which is included in kernel.org.
The bmc_panic functionality is also included in the Intel imb IPMI driver
build 28 and greater.
To apply the bmc_panic patch (in kern/) to a different version of kernel source:
   kver=2.4.18
   cd /usr/src/linux-${kver}
   cat bmcpanic-${kver}.patch | patch -p1
   make menuconfig     (make sure CONFIG_BMCPANIC=y)
   make oldconfig
   make dep
   make bzImage
   make modules
   make modules_install
   mkinitrd -f /boot/initrd-${kver}.img ${kver}
   make install
   reboot

The ipmiutil package does not set the Linux kernel panic timeout.
If a specific panic timeout is desired, do 
"echo 10 >/proc/sys/kernel/panic" to set it to 10 seconds, for instance.
or add panic=10 to the kernel line in grub.conf.

------------------------------
   KNOWN PROBLEMS
------------------------------

See http://sourceforge.net/p/ipmiutil/_list/tickets for a list of bugs.

Contact for best-effort support: arcress at users.sourceforge.net or 
ipmiutil-developers at lists.sourceforge.net or http://ipmiutil.sf.net

--------------------
  LICENSING:
--------------------
The BSD License in the COPYING file applies to all source files 
herein, except for 
  * util/md5.c (Aladdin unrestricted license, compatible with BSD) 
  * util/md2.h (GPL, not used unless do configure --enable-gpl)
  * util/ipmi_ioctls.h (GPL, now defunct and removed)
While the BSD License allows code reuse in both open and non-open 
applications, the md2.h and ipmi_ioctls.h files would have to be removed 
if used in a non-open application.  There is a ALLOW_GPL compile flag
for this that is disabled by default, but can be enabled for GPL 
open-source by running "./configure --enable-gpl". 

--------------------
  CHANGE HISTORY:
--------------------
See http://ipmiutil.sourceforge.net/docs/ChangeLog
