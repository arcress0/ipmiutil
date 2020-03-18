
localedir = $(datadir)/locale
OS_CF = @OS_CFLAGS@ @CROSS_CFLAGS@
OS_LF = @OS_LFLAGS@ @CROSS_LFLAGS@
AM_CPPFLAGS = $(OS_CF) -DLOCALEDIR=\"$(localedir)\" -I..
AM_CPPFLAGS += @IA64_CFLAGS@ @GPL_CFLAGS@
LDADD = $(OS_LF)
# usu LD_SAMX = ../lib/libipmi_lanplus.a -lcrypto
LDSAM = $(OS_LF) @LD_SAMX@
CFLAGS_SAMX = -O2 -g -I. -I.. $(OS_CF) @LANPLUS_CFLAGS@
CFLAGS_SAM  = -O2 -g -I. -I.. $(OS_CF) 
# May be /usr/bin/install or /bin/install
INSTALLBIN = install -c 
extradir = $(datadir)/ipmiutil
inc_dir = $(includedir)
tmpobj = obj
tmpwin = tmp
LIBDIR = @LIB_DIR@

CMDMOD = ipmicmd.c mem_if.c ipmidir.c imbapi.c ipmimv.c ipmild.c ipmibmc.c ipmilipmi.c subs.c
CMDMOD += md5.c md2.c ipmilan.c
CMDMOD += ipmilanplus.c
# CMDMOD = ipmicmd.c mem_if.c @OS_DRIVERS@
# am: configure substitutions are not allowed in _SOURCES variables
TMPOBJL =  $(tmpobj)/ipmilanL.o $(tmpobj)/ipmilanplusL.o
TMPOBJA =  $(tmpobj)/ipmilanA.o $(tmpobj)/ipmilanplusA.o
LIBOBJ = ipmicmd.o mem_if.o ipmidir.o imbapi.o ipmimv.o ipmild.o ipmibmc.o ipmilipmi.o subs.o md5.o md2.o @LIBSENSORS@
OEMMOD = oem_kontron.c oem_fujitsu.c oem_intel.c oem_sun.c oem_supermicro.c oem_dell.c oem_quanta.c oem_hp.c oem_newisys.c oem_lenovo.c oem_asus.c iekanalyzer.c
# usu SAM2OBJ = isensor2.o ievents2.o
L2OBJ = @SAM2OBJ@

# To build ipmiutil, need to use METACFLAGS for each .c/.o

AM_CPPFLAGS += -I. -I.. -DMETACOMMAND 
METASOURCE = ipmiutil.c ialarms.c ihealth.c ievents.c ifru.c ifru_picmg.c igetevent.c ireset.c icmd.c ilan.c isensor.c isel.c iserial.c iwdt.c isol.c idiscover.c iconfig.c ipicmg.c ifirewall.c ifwum.c ihpm.c itsol.c idcmi.c iuser.c $(OEMMOD) $(CMDMOD) 
LDADD += -lpthread 

# The LanDesk library is proprietary, so it is incompatible with ALLOW_GPL.
# To build with LanDesk support:
# First copy the library to ../lib/libipmiapi.a
# Then ./configure --enable-landesk
# Result:
#  AM_CPPFLAGS += -DLINK_LANDESK
#  LDADD += -lipmiapi -L../lib
AM_CPPFLAGS += @LANDESK_CFLAGS@
LDADD += @LANDESK_LDADD@

# For lanplus plugin support (IPMI LAN 2.0 RMCP+) used by SOL:
# If ./configure --disable-lanplus
#  AM_CPPFLAGS += 
#  LDADD +=  
# Otherwise default result is lanplus enabled:
#  AM_CPPFLAGS += -DHAVE_LANPLUS
#  LDADD +=  -L../lib -lipmi_lanplus -L/usr/local/lib -lcrypto 
#  LDADD +=  -L../lib -lintf_lanplus -L/usr/local/lib -lcrypto (older)
# Could also use CMDMOD += ipmilan2.c  instead, if completed.
AM_CPPFLAGS += @LANPLUS_CFLAGS@ 
LDADD += @LANPLUS_LIB@ @LANPLUS_CRYPTO@ 
LANPLUS_OBJ = $(shell ar t @LANPLUS_LIB@ 2>/dev/null) 

# sbin_PROGRAMS are built by default and copied to /usr/sbin at install time
# EXTRA_PROGRAMS are built by default but not included in the install package
# TESTPROGS are not built by default.  To build, do 'make ifruset', for example.
bin_PROGRAMS = ipmiutil ievents idiscover
sbin_PROGRAMS = ipmi_port iseltime
DEV_LIB = libipmiutil.a
# SHRLINK is libipmiutil.so.1 if OS supports it
SHRLINK = @SHR_LINK@
SHR_LIB  = $(SHRLINK)
SHR_BLIB = libipmiutil.so
LDNAME = -Wl,-soname,$(SHR_LIB)
EXTRA_PROGRAMS = ipmi_sample ipmi_sample_evt
TESTPROGS = libimbapi.a iconfig ipmimv ifruset ipmi_sample2 ialarms_enc 
# OLDPROGS are old/previous binaries that may exist and need to be deleted.
OLDPROGS = alarms bmchealth fruconfig getevent hwreset icmd isolconsole pefconfig sensor showsel tmconfig wdt

# To build an imb api library (libimbapi.a):
libimbapi.a:	imbapi.c
	mkdir -p $(tmpobj)
	$(CC) -I. -I.. $(OS_CF) -fPIC $(CFLAGS) -o $(tmpobj)/imbapi.o -c imbapi.c
	$(AR) cru libimbapi.a $(tmpobj)/imbapi.o
	$(RANLIB) libimbapi.a
	rm -f $(tmpobj)/imbapi.o
 
$(tmpobj)/ipmilanplusL.o:	ipmilanplus.c
	mkdir -p $(tmpobj)
	$(CC) $(CFLAGS_SAMX) -o $(tmpobj)/ipmilanplusL.o -c ipmilanplus.c; 

$(tmpobj)/ipmilanL.o: 	ipmilan.c
	mkdir -p $(tmpobj)
	$(CC) $(CFLAGS_SAMX) -o $(tmpobj)/ipmilanL.o -c ipmilan.c; 

$(tmpobj)/ipmilanplusA.o:	ipmilanplus.c
	mkdir -p $(tmpobj)
	$(CC) $(CFLAGS_SAM) -o $(tmpobj)/ipmilanplusA.o -c ipmilanplus.c; 

$(tmpobj)/ipmilanA.o: 	ipmilan.c
	mkdir -p $(tmpobj)
	$(CC) $(CFLAGS_SAM) -o $(tmpobj)/ipmilanA.o -c ipmilan.c; 

# Build a lib to support all of the IPMI drivers (libipmiutil.a)
# If LANPLUS_SAM = yes, include LANPLUS_OBJ, else do not.
# See configure --enable-liblanplus to change this
$(DEV_LIB):	$(CMDMOD:.c=.o) @LANPLUS_LIB@ @LIBSENSORS@ $(TMPOBJL) $(TMPOBJA) 
	if [ "x@LANPLUS_SAM@" = "xyes" ]; then \
	   ar x @LANPLUS_LIB@ ; \
	   $(AR) cru $(DEV_LIB) $(LIBOBJ) $(TMPOBJL) $(LANPLUS_OBJ); \
	   $(RANLIB) $(DEV_LIB); \
	else   \
	   $(AR) cru $(DEV_LIB) $(LIBOBJ) $(TMPOBJA) ; \
	   $(RANLIB) $(DEV_LIB) ; \
	fi 

$(SHR_LIB):	$(CMDMOD:.c=.o) @LANPLUS_LIB@ @LIBSENSORS@ $(TMPOBJL) $(TMPOBJA) 
	if [ "x@LANPLUS_SAM@" = "xyes" ]; then \
	   ar x @LANPLUS_LIB@ ; \
	   $(CC) $(LDFLAGS) -shared $(LDNAME) -o $(SHR_LIB) $(LIBOBJ) $(TMPOBJL) $(LANPLUS_OBJ); \
	else \
	   $(CC) $(LDFLAGS) -shared $(LDNAME) -o $(SHR_LIB) $(LIBOBJ) $(TMPOBJA) ; \
	fi 

$(SHR_BLIB):	$(SHR_LIB)
	ln -s $(SHR_LIB) $(SHR_BLIB)

idiscover$(EXEEXT):	idiscover.c
	$(CC) $(CFLAGS_SAM) $(LDFLAGS) $(OS_LF) -o idiscover idiscover.c

ipmimv$(EXEEXT): 	ipmimv.c
	$(CC) $(CFLAGS_SAM) $(LDFLAGS) -DTEST_BIN -o ipmimv ipmimv.c

ievents$(EXEEXT):         ievents.c
	$(CC) $(CFLAGS_SAM) $(LDFLAGS) -DALONE -o ievents ievents.c 

isensor2.o:	isensor.c
	$(CC) $(CFLAGS_SAM) -o isensor2.o -c isensor.c 

ievents2.o:	ievents.c
	$(CC) $(CFLAGS_SAM) -DSENSORS_OK -o ievents2.o -c ievents.c 

ifru_picmg2.o:	ifru_picmg.c
	$(CC) $(CFLAGS_SAM) -o ifru_picmg2.o -c ifru_picmg.c

ifru2.o:	ifru.c
	$(CC) $(CFLAGS_SAM) -o ifru2.o    -c ifru.c

isel2.o:	isel.c
	$(CC) $(CFLAGS_SAM) -o isel2.o    -c isel.c

ipmi_sample.o:	ipmi_sample.c
	$(CC) $(CFLAGS_SAM) -c ipmi_sample.c

ipmi_sample2.o:	ipmi_sample.c
	$(CC) $(CFLAGS_SAM) -D GET_SENSORS -D GET_FRU -o ipmi_sample2.o -c ipmi_sample.c

ipmi_sample_evt.o:	ipmi_sample_evt.c
	$(CC) $(CFLAGS_SAM) -o ipmi_sample_evt.o -c ipmi_sample_evt.c

ipmi_sample$(EXEEXT):	$(DEV_LIB) ipmi_sample.o isensor.o ievents.o
	$(CC) -g -O2 -o ipmi_sample ipmi_sample.o $(DEV_LIB) $(LDFLAGS) $(LDSAM)

# To build ipmi_sample with GET_SENSORS enabled, need isensors.o, ievents.o 
# Note that this does not include oem_intel, etc. for OEM SEL decoding.
ipmi_sample2$(EXEEXT):	$(DEV_LIB) ipmi_sample2.o ifru2.o isel2.o ifru_picmg2.o $(L2OBJ)
	$(CC) -g -O2 -o ipmi_sample2 ipmi_sample2.o $(L2OBJ) ifru2.o ifru_picmg2.o isel2.o $(DEV_LIB) $(LDFLAGS) $(LDSAM)

ipmi_sample_evt$(EXEEXT):	$(DEV_LIB) ipmi_sample_evt.o $(L2OBJ)
	$(CC) -g -O2 -o ipmi_sample_evt ipmi_sample_evt.o $(L2OBJ) $(DEV_LIB) $(LDFLAGS) $(LDSAM)

ialarms2.o:	ialarms.c
	$(CC) $(CFLAGS_SAM) -DTEST_ENC -o ialarms2.o -c ialarms.c

oem_intel2.o:	oem_intel.c
	$(CC) $(CFLAGS_SAM) -DNO_EVENTS -o oem_intel2.o -c oem_intel.c

ialarms_enc$(EXEEXT):	$(DEV_LIB) ialarms2.o oem_intel2.o
	$(CC) -g -O2 -o ialarms_enc ialarms2.o oem_intel2.o $(DEV_LIB) $(LDFLAGS) $(LDSAM)

ifruset.o:	ifruset.c
	$(CC) $(CFLAGS_SAM) -c ifruset.c

ifruset$(EXEEXT):	$(DEV_LIB) ifruset.o ifru_picmg2.o
	$(CC)  -g -O2 -o ifruset ifruset.o ifru_picmg2.o $(DEV_LIB) $(LDFLAGS) $(LDADD)

iseltime.o:	iseltime.c
	$(CC) $(CFLAGS_SAM) -c iseltime.c

iseltime$(EXEEXT):	$(DEV_LIB) iseltime.o 
	$(CC)  -g -O2 -o iseltime iseltime.o $(DEV_LIB) $(LDFLAGS) $(LDADD)

iconfig$(EXEEXT):      iconfig.c $(DEV_LIB)
	$(CC) $(CFLAGS_SAM) $(LDFLAGS) -o iconfig iconfig.c $(DEV_LIB) @LANPLUS_CRYPTO@

ipmi_port$(EXEEXT):      ipmi_port.c
	$(CC) $(CFLAGS_SAM) $(LDFLAGS) -o ipmi_port ipmi_port.c 

# @LANPLUS_LIB@ is ../lib/libipmi_lanplus.a
../lib/libipmi_lanplus.a:
	cd ../lib;  make

ipmiutil_SOURCES  = $(METASOURCE) 

ipmiutil$(EXEEXT):	$(METASOURCE:.c=.o) @LANPLUS_LIB@ 
	$(CC) $(CFLAGS) $(LDFLAGS) -o ipmiutil $(METASOURCE:.c=.o) $(LDADD) 

ievents_SOURCES    = ievents.c

idiscover_SOURCES = idiscover.c 

ipmi_port_SOURCES = ipmi_port.c 


EXTRA_DIST = imb_api.h ipmicmd.h ipmidir.h ipmilan.h ipmilanplus.h AnsiTerm.h  ifirewall.h ifwum.h ihpm.h ipicmg.h ipmiutil.h md2.h oem_dell.h oem_fujitsu.h oem_intel.h oem_kontron.h oem_sun.h idcmi.h isensor.h iekanalyzer.h

all-am: Makefile $(bin_PROGRAMS) $(sbin_PROGRAMS) $(EXTRA_PROGRAMS) $(DEV_LIB) $(SHRLINK)

install-data-am:  $(EXTRA_PROGRAMS) $(DEV_LIB) $(SHRLINK) 
	@INS_LIB@ mkdir -p $(DESTDIR)$(extradir)
	@INS_LIB@ mkdir -p $(DESTDIR)$(LIBDIR)
	@INS_LIB@ mkdir -p $(DESTDIR)$(inc_dir)
	@INS_LIB@ cp -f ipmi_sample.c $(DESTDIR)$(extradir)
	@INS_LIB@ cp -f ipmi_sample_evt.c $(DESTDIR)$(extradir)
	@INS_LIB@ cp -f ipmicmd.h   $(DESTDIR)$(inc_dir)
	@INS_LIB@ cp -f isensor.c ievents.c $(DESTDIR)$(extradir)
	@INS_LIB@ cp -f isensor.h ievents.h $(DESTDIR)$(extradir)
	@INS_LIB@ cp -f Makefile.sample $(DESTDIR)$(extradir)/Makefile
	@INS_LIB@ cp -f $(DEV_LIB)      $(DESTDIR)$(LIBDIR)
	@INS_LIB@ if [ "x$(SHRLINK)" != "x" ]; then \
	   @INS_LIB@ cp -f $(SHRLINK)     $(DESTDIR)$(LIBDIR) ; \
	   @INS_LIB@ cd $(DESTDIR); ln -sf $(LIBDIR)/$(SHRLINK) $(DESTDIR)$(LIBDIR)/$(SHR_BLIB);  \
	@INS_LIB@ fi

clean-generic:   
	rm -f $(DEV_LIB) $(EXTRA_PROGRAMS) $(OLDPROGS) $(TESTPROGS) $(SHRLINK)
	if [ -d $(tmpobj) ]; then rm -rf $(tmpobj) ;  fi
	if [ -d $(tmpwin) ]; then rm -rf $(tmpwin) ;  fi
	rm -f *.log *.tmp debug*.list *.o *.pdb *.lo *.la *.so
