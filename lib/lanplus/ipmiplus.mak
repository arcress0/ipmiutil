# ipmiplus.mak
# This makefile will build the ipmiutil lib\lanplus directory
#
# Make sure to download and build openssl for Windows first
#

# The ipmiutil lanplus directory
SRC_D=.
INC=/I$(SRC_D) /I$(SRC_D)\inc 
O_LIB=lanplus.lib

# Set your compiler options
CC=cl
!ifndef DSSL11
CF_EX=/DWIN32 $(INC) /D_CONSOLE /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE 
!else
CF_EX=/DWIN32 $(INC) /D_CONSOLE /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /DSSL11
!endif
# CFLAGS= /MD /W3 /WX /Ox /O2 /Ob2 /Gs0 /GF /Gy /nologo $(CF_EX)
# CFLAGS= /W3 /Ox /O2 /Ob2 /Gs0 /GF /Gy /nologo $(CF_EX)
# CFLAGS= /W3 /O2 /Zi /MD /GF /Gy /nologo $(CF_EX)
# CFLAGS= /W3 /O2 /Zi /MD /nologo $(CF_EX)
#CFLAGS= /W3 /O2 /Zi /MT /nologo $(CF_EX) /DHAVE_IPV6
CFLAGS= /W3 /O2 /Zi /MT /nologo $(CF_EX) 
MKLIB=lib
RM=del

LIB_OBJ = lanplus.obj lanplus_crypt.obj lanplus_crypt_impl.obj \
          lanplus_dump.obj lanplus_strings.obj helper.obj ipmi_strings.obj

HEADERS = 

all: banner $(O_LIB)

banner:
	@echo Building ipmi lanplus library

install:

clean:
	$(RM) *.obj 2>NUL
	$(RM) $(O_LIB) 2>NUL

distclean:
	$(RM) *.obj 2>NUL
	$(RM) $(O_LIB) 2>NUL
	$(RM) *.lib 2>NUL

lanplus.obj:    lanplus.c
    $(CC) /c $(CFLAGS) lanplus.c

lanplus_crypt.obj:    lanplus_crypt.c
    $(CC) /c $(CFLAGS) lanplus_crypt.c

lanplus_crypt_impl.obj:    lanplus_crypt_impl.c
    $(CC) /c $(CFLAGS) lanplus_crypt_impl.c

lanplus_dump.obj:    lanplus_dump.c
    $(CC) /c $(CFLAGS) lanplus_dump.c

lanplus_strings.obj:    lanplus_strings.c
    $(CC) /c $(CFLAGS) lanplus_strings.c

ipmi_strings.obj:    ipmi_strings.c
    $(CC) /c $(CFLAGS) ipmi_strings.c

helper.obj:    helper.c
    $(CC) /c $(CFLAGS) helper.c

$(O_LIB):    $(LIB_OBJ)
    $(MKLIB) /OUT:$(O_LIB) /nologo $(LIB_OBJ)

