#!/bin/sh
# Makefile to build/install doc files
#
MKDIR = sh ../mkinstalldirs 
INSTALL_SH = sh ../install-sh -c
INSTALL_DATA_SH = ${INSTALL_SH} -m 644
prefix = /usr

datadir = ${prefix}/share
mandir = ${prefix}/share/man
docdir = ${prefix}/share/doc/ipmiutil-${version}
etcdir = /etc
datato = ${DESTDIR}${datadir}/ipmiutil
cronto = ${DESTDIR}${etcdir}/cron.daily
manto  = ${DESTDIR}${mandir}/man8
varto  = ${DESTDIR}/var/lib/ipmiutil

all:	

clean:

clobber:
	
distclean:
	
check:

install:
	$(MKDIR) ${manto}
	$(MKDIR) ${datato}
	$(MKDIR) ${varto}
	${INSTALL_DATA_SH} isel.8      ${manto}
	${INSTALL_DATA_SH} isensor.8   ${manto}
	${INSTALL_DATA_SH} ireset.8    ${manto}
	${INSTALL_DATA_SH} ialarms.8   ${manto}
	${INSTALL_DATA_SH} iwdt.8      ${manto}
	${INSTALL_DATA_SH} ifru.8      ${manto}
	${INSTALL_DATA_SH} ilan.8      ${manto}
	${INSTALL_DATA_SH} iserial.8   ${manto}
	${INSTALL_DATA_SH} icmd.8      ${manto}
	${INSTALL_DATA_SH} igetevent.8 ${manto}
	${INSTALL_DATA_SH} ihealth.8   ${manto}
	${INSTALL_DATA_SH} ipmiutil.8  ${manto}
	${INSTALL_DATA_SH} isol.8      ${manto}
	${INSTALL_DATA_SH} idiscover.8 ${manto}
	${INSTALL_DATA_SH} ievents.8   ${manto}
	${INSTALL_DATA_SH} iconfig.8   ${manto}
	${INSTALL_DATA_SH} ipmi_port.8 ${manto}
	${INSTALL_DATA_SH} ipicmg.8    ${manto}
	${INSTALL_DATA_SH} ifirewall.8 ${manto}
	${INSTALL_DATA_SH} ifwum.8     ${manto}
	${INSTALL_DATA_SH} ihpm.8      ${manto}
	${INSTALL_DATA_SH} isunoem.8     ${manto}
	${INSTALL_DATA_SH} itsol.8    	 ${manto}
	${INSTALL_DATA_SH} iekanalyzer.8 ${manto}
	${INSTALL_DATA_SH} idelloem.8    ${manto}
	${INSTALL_DATA_SH} idcmi.8 	 ${manto}
	${INSTALL_DATA_SH} ismcoem.8 	 ${manto}
	${INSTALL_DATA_SH} iuser.8 	 ${manto}
	${INSTALL_DATA_SH} iseltime.8 	 ${manto}
	${INSTALL_DATA_SH} bmclanpet.mib ${datato}
	cd ${manto}; gzip -nf *.8
	
