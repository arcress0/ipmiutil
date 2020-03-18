#!/bin/sh
# Makefile to build/install script files
#
MKDIR = sh ../mkinstalldirs 
INSTALL_SH = sh ../install-sh -c
INSTALL_DATA_SH = ${INSTALL_SH} -m 644
INSTALL_SCRIPT_SH =  ${INSTALL_SH} -m 755
prefix = /usr

datadir = ${prefix}/share
mandir = ${prefix}/share/man
docdir = ${prefix}/share/doc/ipmiutil-${version}
sbindir = ${prefix}/sbin
etcdir = /etc
datato = ${DESTDIR}${datadir}/ipmiutil
cronto = ${DESTDIR}${etcdir}/cron.daily
sbinto = ${DESTDIR}${sbindir}
varto  = ${DESTDIR}/var/lib/ipmiutil
initto = ${DESTDIR}@INIT_DIR@
sysdto = ${DESTDIR}@SYSTEMD_DIR@
sysvinit = ${datato}
sbinfls = ialarms ihealth ifru igetevent ireset icmd isol ilan isensor isel iserial iwdt iconfig ipicmg ifirewall ifwum ihpm iuser

all:	

clean:

clobber:
	
distclean:
	
check:

install:
	$(MKDIR) ${datato}
	$(MKDIR) ${sbinto}
	$(MKDIR) ${initto}
	$(MKDIR) ${varto}
	$(MKDIR) ${sysdto}
	${INSTALL_SCRIPT_SH} ipmi_if.sh   ${datato}
	${INSTALL_SCRIPT_SH} evt.sh       ${datato}
	if [ -f /etc/debian_version ]; then \
	   sysvinit=${initto}; fi
	${INSTALL_SCRIPT_SH} ipmiutil_wdt ${sysvinit}/ipmiutil_wdt
	${INSTALL_SCRIPT_SH} ipmiutil_asy ${sysvinit}/ipmiutil_asy
	${INSTALL_SCRIPT_SH} ipmiutil_evt ${sysvinit}/ipmiutil_evt
	${INSTALL_SCRIPT_SH} ipmi_port.sh ${sysvinit}/ipmi_port
	${INSTALL_SCRIPT_SH} ipmi_info    ${sysvinit}/ipmi_info
	${INSTALL_SCRIPT_SH} checksel     ${datato}
	${INSTALL_DATA_SH} ipmiutil_wdt.service ${sysdto}
	${INSTALL_DATA_SH} ipmiutil_asy.service ${sysdto}
	${INSTALL_DATA_SH} ipmiutil_evt.service ${sysdto}
	${INSTALL_DATA_SH} ipmi_port.service    ${sysdto}
	${INSTALL_SCRIPT_SH} ipmiutil.env.template    ${datato}
	${INSTALL_SCRIPT_SH} ipmiutil.pre     ${datato}
	${INSTALL_SCRIPT_SH} ipmiutil.setup   ${datato}
	${INSTALL_SCRIPT_SH} ipmi.init.basic  ${datato}
	for f in ${sbinfls}; do \
	    ${INSTALL_SCRIPT_SH} $$f ${sbinto}; done
	
