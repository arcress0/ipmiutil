# ipmi_sample.mak
# This makefile will build the ipmiutil ipmi_sample application
#
LIBC_RT=libcmt.lib /NODEFAULTLIB:"msvcirt.lib"
# LIBC_RT=msvcrt.lib /NODEFAULTLIB:"msvcirt.lib"

MARCH=IX86
# MARCH=X64
# The ipmiutil directory
SRC_D=%CD%
LIB_D=%CD%
INC=/I$(SRC_D) 

# Set your compiler options
# CFLAGS_O=/W3 /O2 /Zi /MT /nologo
CFLAGS_O=/W3 /Zi /MT /nologo
CF_SAM=/DWIN32 $(INC) /D_CONSOLE /D_CRT_SECURE_NO_DEPRECATE /DHAVE_STRING_H 
CFLAGS_SAM=$(CFLAGS_O) $(CF_SAM)
LFLAGS=/nologo /subsystem:console /machine:$(MARCH) /opt:ref
LIBS_W=comsuppw.lib wbemuuid.lib
LIBS_EX=advapi32.lib kernel32.lib wsock32.lib ws2_32.lib $(LIBS_W) $(LIBC_RT)
LIBS_PEF=/LIBPATH:$(LIB_D) iphlpapi.lib
CC=cl
LINK=link
RM=del

HEADER = ipmicmd.h 
SAMP_LIB = ipmiutil.lib
TARG1_EXE = ipmi_sample.exe
TARG2_EXE = ipmi_sample_evt.exe

###################################################################
all:	$(TARG1_EXE) 

clean:
	$(RM) *.obj 2>NUL
	$(RM) $(TARG1_EXE) 2>NUL

ipmi_sample.obj:    ipmi_sample.c $(HEADER)
	$(CC) /c $(CFLAGS_SAM) ipmi_sample.c

$(TARG1_EXE):  $(SAMP_LIB) ipmi_sample.obj
	$(LINK) $(LFLAGS) /OUT:$(TARG1_EXE) ipmi_sample.obj $(SAMP_LIB) $(LIBS_PEF) $(LIBS_EX) 

ipmi_sample_evt.obj:    ipmi_sample_evt.c $(HEADER)
	$(CC) /c $(CFLAGS_SAM) ipmi_sample_evt.c

isensor2.obj:    isensor.c isensor.h $(HEADER)
	$(CC) /c /Foisensor2.obj $(CFLAGS_SAM) isensor.c

ievents2.obj:    ievents.c ievents.h $(HEADER)
	$(CC) /c /Foievents2.obj $(CFLAGS_SAM) /DSENSORS_OK ievents.c

$(TARG2_EXE):  $(SAMP_LIB) ipmi_sample_evt.obj ievents2.obj isensor2.obj
	$(LINK) $(LFLAGS) /OUT:$(TARG2_EXE) ipmi_sample_evt.obj ievents2.obj isensor2.obj $(SAMP_LIB) $(LIBS_PEF) $(LIBS_EX) 


