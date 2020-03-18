# This makefile will build the ipmiutil util directory (x64)
#
# First download getopt.c getopt.h
# Then download and build openssl for Windows 
#
LIBC_RT=libcmt.lib /NODEFAULTLIB:"msvcirt.lib"
# LIBC_RT=msvcrt.lib /NODEFAULTLIB:"msvcirt.lib"

# The ipmiutil directory
SRC_D=.
LIB_D=..\lib
L2_D=$(LIB_D)\lanplus
L3_D=$(LIB_D)\lanplus\inc
INSTALLTOP=install
TMP_D=tmp
INC=/I$(SRC_D) /I$(L2_D) /I$(L3_D)
CMD_OBJ  = getopt.obj ipmicmd.obj imbapi.obj md5.obj md2.obj  \
           ipmilan.obj ipmims.obj subs.obj
CMD_OBJ = $(CMD_OBJ) ipmilanplus.obj
# To remove lanplus support use the empty LANPLUS variables
# L2_OBJ=
# LF_LANPLUS=
# CF_LANPLUS=
L2_OBJ = $(L2_D)\helper.obj $(L2_D)\ipmi_strings.obj $(L2_D)\lanplus.obj \
	$(L2_D)\lanplus_crypt_impl.obj $(L2_D)\lanplus_dump.obj \
	$(L2_D)\lanplus_strings.obj $(L2_D)\lanplus_crypt.obj
LF_LANPLUS=/LIBPATH:$(LIB_D) /LIBPATH:$(L2_D) $(L2_OBJ) ssleay32.lib libeay32.lib 
CF_LANPLUS=/D HAVE_LANPLUS

# Set your compiler options
# To remove any GPL dependencies, use the CF_EX line with NON_GPL
# CFLAGS_O=/W3 /O2 /Zi /MD /GF /Gy /nologo 
# CFLAGS_O=/W3 /O2 /Zi /MD /nologo 
# CFLAGS_O=/W3 /O2 /Zi /MT /nologo 
CFLAGS_O=/W3 /O2 /Zi /MT /nologo
CF_EX=/DWIN32 $(CF_LANPLUS) $(INC) /D_CONSOLE /D_CRT_SECURE_NO_DEPRECATE /D_CRT_NONSTDC_NO_DEPRECATE /DHAVE_STRING_H
CF_SAM=/DWIN32 $(INC) /D_CONSOLE /D_CRT_SECURE_NO_DEPRECATE /DHAVE_STRING_H $(CF_LANPLUS)
CFLAGS=$(CFLAGS_O) $(CF_EX) /DSKIP_MD2
CFLAGS_M=$(CFLAGS_O) $(CF_EX) /DSKIP_MD2 /DMETACOMMAND
CFLAGS_SAM=$(CFLAGS_O) $(CF_SAM) 
LFLAGS=/nologo /subsystem:console /machine:$(MARCH) /opt:ref
#LFLAGS=/nologo /subsystem:console /machine:IX86 /opt:ref
#LFLAGS=/nologo /subsystem:console /machine:IX86 /opt:ref /debug
# LFLAGS_D=/nologo /subsystem:console /machine:I386 /opt:ref /dll

# CFLAGS_W=/O2 /D_CONSOLE /D_MBCS /EHsc /ML /W3 /Zi /TP 
CFLAGS_W=/TP /EHsc $(CFLAGS)
LFLAGS_W=/nologo /subsystem:console /machine:$(MARCH) /opt:ref 
LIBS_W=comsuppw.lib wbemuuid.lib 
# gdi32.lib comdlg32.lib shell32.lib uuid.lib

CC=cl
LINK=link
MKDIR=-mkdir
MKLIB=lib
RM=del
CP=copy

LIBS_EX  = advapi32.lib kernel32.lib wsock32.lib ws2_32.lib $(LIBS_W) $(LIBC_RT)
LIBS_PEF = /LIBPATH:$(LIB_D) iphlpapi.lib
# LIBS_EX+=wsock32.lib user32.lib gdi32.lib 

HEADER=ipmicmd.h imb_api.h ipmilan.h ipmidir.h ipmilanplus.h \
       ipmiutil.h 

SHOWSEL = showsel
TARG_EXE=ievents.exe $(SHOWSEL)msg.dll ipmi_sample.exe ipmi_sample2.exe ipmi_sample_evt.exe $(SAMP_DLL)
#  alarms.exe ihealth.exe $(SHOWSEL).exe $(SHOWSEL)msg.dll \
#  ireset.exe ifru.exe ilan.exe iserial.exe wdt.exe \
#  getevent.exe sensor.exe icmd.exe isolconsole.exe idiscover.exe \
#  ievents.exe
SAMP_LIB = ipmiutil.lib 
SAMP_DLL = ipmiutillib.dll

E_EXE=ipmiutil.exe
E_OBJ=$(TMP_D)\ipmiutil.obj \
      $(TMP_D)\ialarms.obj  $(TMP_D)\ihealth.obj $(TMP_D)\iwdt.obj \
      $(TMP_D)\ireset.obj   $(TMP_D)\ifru.obj    $(TMP_D)\ilan.obj \
      $(TMP_D)\iserial.obj  $(TMP_D)\icmd.obj    $(TMP_D)\isol.obj \
      $(TMP_D)\isolwin.obj  $(TMP_D)\AnsiTerm.obj $(TMP_D)\idiscover.obj \
      $(TMP_D)\iconfig.obj  $(TMP_D)\igetevent.obj $(TMP_D)\isensor.obj \
      $(TMP_D)\isel.obj     $(TMP_D)\ievents.obj   \
      $(TMP_D)\ipicmg.obj   $(TMP_D)\ifirewall.obj \
      $(TMP_D)\iekanalyzer.obj $(TMP_D)\ifru_picmg.obj \
      $(TMP_D)\oem_kontron.obj $(TMP_D)\ihpm.obj $(TMP_D)\ifwum.obj \
      $(TMP_D)\oem_fujitsu.obj $(TMP_D)\oem_intel.obj $(TMP_D)\oem_lenovo.obj \
      $(TMP_D)\oem_asus.obj  $(TMP_D)\iuser.obj   \
      $(TMP_D)\oem_sun.obj     $(TMP_D)\oem_dell.obj $(TMP_D)\oem_hp.obj \
      $(TMP_D)\oem_supermicro.obj   $(TMP_D)\itsol.obj $(TMP_D)\idcmi.obj \
      $(TMP_D)\oem_quanta.obj  $(TMP_D)\oem_newisys.obj  $(CMD_OBJ) mem_if.obj

###################################################################
all: banner $(TMP_D) exe 

banner:
	@echo Building ipmiutil

$(TMP_D):
	$(MKDIR) $(TMP_D)
	@echo created $(TMP_D)

lib:    $(L2_OBJ)
    cd $(LIB_D)
    nmake /nologo -f ipmilib.mak
    cd ../util

exe: $(E_EXE) $(TARG_EXE)

install:
	$(MKDIR) $(INSTALLTOP)
	$(MKDIR) $(INSTALLTOP)\bin
	$(CP) $(E_EXE)    $(INSTALLTOP)\bin
	xcopy $(TARG_EXE) $(INSTALLTOP)\bin
	xcopy *.dll       $(INSTALLTOP)\bin

clean:
	$(RM) *.obj 2>NUL
	$(RM) $(TARG_EXE) 2>NUL
	$(RM) *.exe 2>NUL
	-$(RM) $(TMP_D)\*.obj 2>NUL

distclean:
	$(RM) *.obj 2>NUL
	$(RM) $(TARG_EXE) 2>NUL
	$(RM) *.exe 2>NUL
	-$(RM) $(TMP_D)\*.* 2>NUL
	rmdir $(TMP_D) 2>NUL
	$(RM) *.rc 2>NUL
	$(RM) *.bin 2>NUL
	$(RM) *.RES 2>NUL
	$(RM) getopt.* 2>NUL

getopt.obj:    getopt.c
    $(CC) /c $(CFLAGS) getopt.c

imbapi.obj:    imbapi.c
    $(CC) /c $(CFLAGS_M) imbapi.c

ipmicmd.obj:    ipmicmd.c
    $(CC) /c $(CFLAGS) ipmicmd.c

ipmilan.obj:    ipmilan.c
    $(CC) /c $(CFLAGS) ipmilan.c

ipmilanplus.obj:    ipmilanplus.c
    $(CC) /c $(CFLAGS_M) ipmilanplus.c

md5.obj:    md5.c
    $(CC) /c $(CFLAGS) md5.c

md2.obj:    md2.c
    $(CC) /c $(CFLAGS) md2.c

ievents.obj:    ievents.c
    $(CC) /c $(CFLAGS) ievents.c

ialarms.obj:    ialarms.c
    $(CC) /c $(CFLAGS) ialarms.c

ihealth.obj:    ihealth.c
    $(CC) /c $(CFLAGS) ihealth.c

igetevent.obj:    igetevent.c
    $(CC) /c $(CFLAGS) igetevent.c

mem_if.obj:    mem_if.c
    $(CC) /c $(CFLAGS_W) mem_if.c

ipmims.obj:    ipmims.cpp
    $(CC) /c $(CFLAGS_W) ipmims.cpp

isel.obj:    isel.c
    $(CC) /c $(CFLAGS) isel.c

ireset.obj:    ireset.c
    $(CC) /c $(CFLAGS) ireset.c

ireset.exe:     ireset.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:ireset.exe ireset.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

ifru.obj:    ifru.c
    $(CC) /c $(CFLAGS) ifru.c

ifru.exe:     ifru.obj mem_if.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:ifru.exe ifru.obj mem_if.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX) 

ilan.obj:    ilan.c
    $(CC) /c $(CFLAGS) ilan.c

ilan.exe:     ilan.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:ilan.exe ilan.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_PEF) $(LIBS_EX) 

iserial.obj:    iserial.c
    $(CC) /c $(CFLAGS) iserial.c

iserial.exe:     iserial.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:iserial.exe iserial.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

isensor.obj:    isensor.c
    $(CC) /c $(CFLAGS) isensor.c

isensor.exe:     isensor.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:isensor.exe isensor.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

iwdt.obj:    iwdt.c
    $(CC) /c $(CFLAGS) iwdt.c

iwdt.exe:     iwdt.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:iwdt.exe iwdt.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

isol.obj:    isol.c
    $(CC) /c $(CFLAGS) isol.c

isolwin.obj:    isolwin.c
    $(CC) /c $(CFLAGS) isolwin.c

isol.exe:     isol.obj isolwin.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:isol.exe isol.obj isolwin.obj \
		$(CMD_OBJ) $(LF_LANPLUS) $(LIBS_EX)

icmd.obj:    icmd.c
    $(CC) /c $(CFLAGS) icmd.c

icmd.exe:     icmd.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:icmd.exe icmd.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

idiscover.obj:    idiscover.c
    $(CC) /c $(CFLAGS) idiscover.c

idiscover.exe:     idiscover.obj getopt.obj
    $(LINK) $(LFLAGS) /OUT:idiscover.exe idiscover.obj getopt.obj \
            $(LF_LANPLUS) $(LIBS_EX)

ialarms.exe:     ialarms.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:ialarms.exe ialarms.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX)

ihealth.exe:     ihealth.obj mem_if.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:ihealth.exe ihealth.obj mem_if.obj $(CMD_OBJ) \
            $(LF_LANPLUS) $(LIBS_EX) 

igetevent.exe:     igetevent.obj ievents.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:igetevent.exe igetevent.obj ievents.obj \
            $(CMD_OBJ) $(LF_LANPLUS) $(LIBS_EX) 

isel.exe:     isel.obj ievents.obj $(CMD_OBJ)
    $(LINK) $(LFLAGS) /OUT:isel.exe isel.obj ievents.obj \
            $(CMD_OBJ) $(LF_LANPLUS) $(LIBS_EX)

$(SHOWSEL).mc:     
	$(CP) ..\scripts\$(SHOWSEL).mc .

$(SHOWSEL)msg.dll:     $(SHOWSEL).mc
    mc -U $(SHOWSEL).mc
    rc -r $(SHOWSEL).rc
    $(LINK) /machine:$(MARCH) -dll -noentry -out:$(SHOWSEL)msg.dll $(SHOWSEL).res

mem_if.exe:      $(TMP_D)\mem_if.obj 
    $(LINK) $(LFLAGS_W) /OUT:mem_if.exe $(TMP_D)\mem_if.obj $(LIBS_EX) 

$(TMP_D)\mem_if.obj:    mem_if.c
    $(CC) /c $(CFLAGS_W) /DCOMP_BIN /Fo$(TMP_D)\mem_if.obj mem_if.c

ievents.exe:     ievents.c 
    $(CC) /c $(CFLAGS) /DALONE ievents.c
    $(LINK) $(LFLAGS) /OUT:ievents.exe ievents.obj $(LIBS_EX)
    $(RM) ievents.obj

ipmims.exe:     ipmims.cpp
    $(CC) /c $(CFLAGS_W) /DALONE /DTEST_BIN ipmims.cpp
    $(LINK) $(LFLAGS_W) /OUT:ipmims.exe ipmims.obj $(LIBS_EX)
    $(RM) ipmims.obj

$(TMP_D)\ievents.obj:    ievents.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ievents.obj ievents.c

$(TMP_D)\ipmiutil.obj:    ipmiutil.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ipmiutil.obj ipmiutil.c

$(TMP_D)\ialarms.obj:    ialarms.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ialarms.obj ialarms.c

$(TMP_D)\ihealth.obj:    ihealth.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ihealth.obj ihealth.c

$(TMP_D)\iconfig.obj:    iconfig.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\iconfig.obj iconfig.c

$(TMP_D)\ipicmg.obj:    ipicmg.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ipicmg.obj ipicmg.c

$(TMP_D)\ifirewall.obj:    ifirewall.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ifirewall.obj ifirewall.c

$(TMP_D)\ifwum.obj:    ifwum.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ifwum.obj ifwum.c

$(TMP_D)\ihpm.obj:    ihpm.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ihpm.obj ihpm.c

$(TMP_D)\idcmi.obj:    idcmi.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\idcmi.obj idcmi.c

$(TMP_D)\iuser.obj:    iuser.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\iuser.obj iuser.c

$(TMP_D)\oem_fujitsu.obj:    oem_fujitsu.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_fujitsu.obj oem_fujitsu.c

$(TMP_D)\oem_kontron.obj:    oem_kontron.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_kontron.obj oem_kontron.c

$(TMP_D)\oem_intel.obj:    oem_intel.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_intel.obj oem_intel.c

$(TMP_D)\oem_sun.obj:    oem_sun.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_sun.obj oem_sun.c

$(TMP_D)\oem_dell.obj:    oem_dell.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_dell.obj oem_dell.c

$(TMP_D)\oem_hp.obj:    oem_hp.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_hp.obj oem_hp.c

$(TMP_D)\oem_supermicro.obj:    oem_supermicro.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_supermicro.obj oem_supermicro.c

$(TMP_D)\oem_lenovo.obj:    oem_lenovo.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_lenovo.obj oem_lenovo.c

$(TMP_D)\oem_asus.obj:    oem_asus.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_asus.obj oem_asus.c

$(TMP_D)\oem_quanta.obj:    oem_quanta.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_quanta.obj oem_quanta.c

$(TMP_D)\oem_newisys.obj:    oem_newisys.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\oem_newisys.obj oem_newisys.c

$(TMP_D)\iekanalyzer.obj:    iekanalyzer.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\iekanalyzer.obj iekanalyzer.c

$(TMP_D)\ifru_picmg.obj:    ifru_picmg.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ifru_picmg.obj ifru_picmg.c

$(TMP_D)\ifru.obj:    ifru.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ifru.obj ifru.c

$(TMP_D)\ireset.obj:    ireset.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ireset.obj ireset.c

$(TMP_D)\ilan.obj:    ilan.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\ilan.obj ilan.c

$(TMP_D)\iserial.obj:    iserial.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\iserial.obj iserial.c

$(TMP_D)\isensor.obj:    isensor.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\isensor.obj isensor.c

$(TMP_D)\icmd.obj:    icmd.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\icmd.obj icmd.c

$(TMP_D)\igetevent.obj:    igetevent.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\igetevent.obj igetevent.c

$(TMP_D)\isel.obj:    isel.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\isel.obj isel.c

$(TMP_D)\isol.obj:    isol.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\isol.obj isol.c

$(TMP_D)\isolwin.obj:    isolwin.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\isolwin.obj isolwin.c

$(TMP_D)\itsol.obj:    itsol.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\itsol.obj itsol.c

$(TMP_D)\AnsiTerm.obj:    AnsiTerm.cpp
    $(CC) /c $(CFLAGS_W) /Fo$(TMP_D)\AnsiTerm.obj AnsiTerm.cpp

$(TMP_D)\idiscover.obj:    idiscover.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\idiscover.obj idiscover.c

$(TMP_D)\iwdt.obj:    iwdt.c
    $(CC) /c $(CFLAGS_M) /Fo$(TMP_D)\iwdt.obj iwdt.c

$(E_EXE):  $(E_OBJ) 
  $(LINK) $(LFLAGS) /OUT:$(E_EXE) $(E_OBJ) $(LF_LANPLUS) $(LIBS_PEF) $(LIBS_EX) 

ipmi_sample.obj:    ipmi_sample.c
    $(CC) /c $(CFLAGS_SAM) ipmi_sample.c

$(SAMP_LIB):    $(CMD_OBJ) mem_if.obj
    $(CC) /c $(CFLAGS_SAM) ipmilanplus.c
    $(MKLIB) /OUT:$(SAMP_LIB) /nologo $(CMD_OBJ)  mem_if.obj
    del ipmilanplus.obj

$(SAMP_DLL):    $(CMD_OBJ) mem_if.obj
    $(CC) /D_WINDLL /D_USRDLL /c $(CFLAGS_SAM) ipmilanplus.c
    $(LINK) /DLL $(LFLAGS) /OUT:$(SAMP_DLL) /def:ipmiutillib.def $(CMD_OBJ) mem_if.obj $(LIBS_PEF) $(LF_LANPLUS) $(LIBS_EX) 
    del ipmilanplus.obj

ipmi_sample.exe:  $(SAMP_LIB) ipmi_sample.obj
    $(LINK) $(LFLAGS) /OUT:ipmi_sample.exe ipmi_sample.obj $(SAMP_LIB) $(LIBS_PEF) $(LF_LANPLUS) $(LIBS_EX) 

ipmi_sample2.exe:  $(SAMP_LIB) ipmi_sample.c isensor.c ievents.c
    $(CC) /c $(CFLAGS_SAM) /DGET_SENSORS ipmi_sample.c
    $(CC) /c $(CFLAGS_SAM) isensor.c
    $(CC) /c $(CFLAGS_SAM) ievents.c
    $(LINK) $(LFLAGS) /OUT:ipmi_sample2.exe ipmi_sample.obj isensor.obj ievents.obj $(SAMP_LIB) $(LIBS_PEF) $(LF_LANPLUS)  $(LIBS_EX)
    del isensor.obj ievents.obj ipmi_sample.obj

ifruset.obj:	ifruset.c
	$(CC) /c $(CFLAGS_SAM) ifruset.c

ifruset.exe:	$(SAMP_LIB) ifruset.obj  ifru_picmg.obj
	$(LINK) $(LFLAGS) /OUT:ifruset.exe ifruset.obj ifru_picmg.obj $(SAMP_LIB) $(LIBS_PEF) $(LF_LANPLUS) $(LIBS_EX) 

ipmi_sample_evt.obj:    ipmi_sample_evt.c $(HEADER)
        $(CC) /c $(CFLAGS_SAM) ipmi_sample_evt.c

isensor2.obj:    isensor.c isensor.h $(HEADER)
        $(CC) /c /Foisensor2.obj $(CFLAGS_SAM) isensor.c

ievents2.obj:    ievents.c ievents.h $(HEADER)
        $(CC) /c /Foievents2.obj $(CFLAGS_SAM) /DSENSORS_OK ievents.c

ipmi_sample_evt.exe:  $(SAMP_LIB) ipmi_sample_evt.obj ievents2.obj isensor2.obj
        $(LINK) $(LFLAGS) $(LF_LANPLUS) /OUT:ipmi_sample_evt.exe ipmi_sample_evt.obj ievents2.obj isensor2.obj $(SAMP_LIB) $(LIBS_PEF) $(LIBS_EX)

