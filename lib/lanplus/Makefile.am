#  Makefile.am for libipmi_lanplus.so and libipmi_lanplus.a

ODIR 	= obj
CFLAGS_STATIC  = @OS_CFLAGS@ -DSTATIC -fno-strict-aliasing -fPIC $(CFLAGS) 
AM_CFLAGS  = @OS_CFLAGS@ -fPIC  @LANPLUS_CFLAGS@ -I. -I./inc

MAINTAINERCLEANFILES    = Makefile.in

lanplus_liba      = libipmi_lanplus.a

noinst_LIBRARIES = $(lanplus_liba)

libipmi_lanplus_a_SOURCES  = lanplus.c lanplus.h rmcp.h asf.h \
                               lanplus_dump.c lanplus_dump.h \
                               lanplus_crypt.c lanplus_crypt.h \
                               lanplus_crypt_impl.c lanplus_crypt_impl.h \
                               lanplus_strings.c \
 			       helper.c ipmi_strings.c

lib_LTLIBRARIES       = 
# lib_LTLIBRARIES       = libipmi_lanplus.la 

# libipmi_lanplus_la_LIBADD    = -lcrypto

# libipmi_lanplus_la_SOURCES   = lanplus.c lanplus.h rmcp.h asf.h \
#                                lanplus_dump.c lanplus_dump.h \
#                                lanplus_crypt.c lanplus_crypt.h \
#                                lanplus_crypt_impl.c lanplus_crypt_impl.h \
#                                lanplus_strings.c \
# 			       helper.c ipmi_strings.c


#am_libipmi_lanplus_a_OBJECTS = $(ODIR)/lanplus.o $(ODIR)/lanplus_strings.o \
#		$(ODIR)/lanplus_crypt.o $(ODIR)/lanplus_crypt_impl.o \
#               	$(ODIR)/lanplus_dump.o \
#		$(ODIR)/helper.o $(ODIR)/ipmi_strings.o 

clean-generic:
	rm -rf $(ODIR)
	rm -f $(lanplus_liba) $(lib_LTLIBRARIES) *.o *.so

clean:		clean-generic

#$(lanplus_liba):	$(am_libipmi_lanplus_a_OBJECTS)
#	$(AR) cru $(lanplus_liba) $(am_libipmi_lanplus_a_OBJECTS)
#	$(RANLIB) $(lanplus_liba)
		
#$(ODIR):
#	mkdir -p $(ODIR)

#$(ODIR)/%.o:    %.c $(ODIR)
#	$(CC) -c $(CFLAGS_STATIC) $(INCLUDES) -o $@ $<

