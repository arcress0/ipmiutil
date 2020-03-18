#!/bin/sh
# Makefile.am to build/install ipmiutil lib files
#
# If --enable-lanplus (-DHAVE_LANPLUS), then this script
# does a make from lib/lanplus/ for the lanplus library.
# If --enable-landesk (-DLINK_LANDESK), then this script
# assumes that these files are copied into this lib/ directory:
#   libipmiapi32.a  libipmiapi64.a.suse  libipmiapi64.a.redhat
#
MKDIR = sh ../mkinstalldirs 
INSTALL = sh ../install-sh -c
# INSTALL_DATA = $(INSTALL) -m 644
INSTALL_SCRIPT =  $(INSTALL) -m 755
INSTALL_BIN = /usr/bin/install -c
iprefix = @prefix@
iexec_prefix = @exec_prefix@
PLUSFLAGS = @LANPLUS_CFLAGS@

sbindir = $(iexec_prefix)/sbin
mandir = $(iprefix)/man
libdir = $(iprefix)/lib
datadir = $(iprefix)/share
datato = ${DESTDIR}${datadir}/ipmiutil
lib32  = ${DESTDIR}$(iprefix)/lib
lib64  = ${DESTDIR}$(iprefix)/lib64

all:	
	if [ -f libipmiapi32.a ]; then \
	  rm -f libipmiapi.a;  \
	  if [ "`uname -m`" = "x86_64" ]; then \
	    if [ -f /etc/SuSE-release ]; then  \
	      cp -f libipmiapi64.a.suse libipmiapi.a; \
	    else cp -f libipmiapi64.a.redhat  libipmiapi.a; fi \
          else cp -f libipmiapi32.a libipmiapi.a; fi \
	fi
	if [ "$(PLUSFLAGS)" = "-DHAVE_LANPLUS" ]; then \
	  cd lanplus; make clean; make  ; \
	  pwd; cp -f libipmi_lanplus.a .. ; \
	fi

clean:
	rm -f libipmiapi.a lib*_lanplus.a lib*_lanplus.so
	cd lanplus; make clean

clobber:
	rm -f libipmiapi.a lib*_lanplus.a  lib*_lanplus.so
	cd lanplus; make clean
	
distclean:
	rm -f libipmiapi.a lib*.a  lib*_lanplus.so
	cd lanplus; make distclean
	
install:
	if [ "$(PLUSFLAGS)" = "-DHAVE_LANPLUS" ]; then \
	   $(MKDIR) ${datato} ; \
	   cd lanplus; make install ; \
	fi

check:

installso:
	@if [ -d /usr/lib64 ]; then \
	$(MKDIR) ${lib64} ; \
	echo "installing libipmi_lanplus into ${lib64}"; \
	cd lanplus; /bin/sh ../../libtool --mode=install $(INSTALL_BIN)  'libipmi_lanplus.la' '${lib64}/libipmi_lanplus.la' ; \
	$(INSTALL_BIN) .libs/libipmi_lanplus.so.0.0.0 ${lib64}/libipmi_lanplus.so.0.0.0 ; \
	(cd ${lib64} && rm -f libipmi_lanplus.so.0 && ln -s libipmi_lanplus.so.0.0.0 libipmi_lanplus.so.0) ; \
	(cd ${lib64} && rm -f libipmi_lanplus.so && ln -s libipmi_lanplus.so.0.0.0 libipmi_lanplus.so) ; \
	$(INSTALL_BIN) .libs/libipmi_lanplus.lai ${lib64}/libipmi_lanplus.la ; \
	$(INSTALL_BIN) .libs/libipmi_lanplus.a ${lib64}/libipmi_lanplus.a; \
	ranlib ${lib64}/libipmi_lanplus.a ; \
	chmod 644 ${lib64}/libipmi_lanplus.a ;  \
	(cd ${lib32} && rm -f libipmi_lanplus.* ); \
	../../libtool --finish ${lib64} \
	fi
	
