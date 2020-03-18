# ipmilib.mak
# This makefile will build the ipmiutil lib directory
#
# Make sure to download and build openssl for Windows
#

# The ipmiutil lib directory
PLUS_DIR=lanplus
PLUS_LIB=lanplus.lib

# Set your compiler options
RM=del
CP=copy

all: banner $(PLUS_LIB)

banner:
	@echo Building ipmi libs

$(PLUS_LIB):    
    cd $(PLUS_DIR)
    nmake /nologo -f ipmiplus.mak all
    cd ..
    $(CP) $(PLUS_DIR)\$(PLUS_LIB) .

clean:
    $(RM) $(PLUS_LIB) 2>NUL
    cd $(PLUS_DIR)
    nmake /nologo -f ipmiplus.mak clean
    cd ..

distclean:
    $(RM) $(PLUS_LIB) 2>NUL
    cd $(PLUS_DIR)
    nmake /nologo -f ipmiplus.mak distclean
    cd ..

