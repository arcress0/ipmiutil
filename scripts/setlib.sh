#!/bin/sh
#     setup sequence for ipmiutil
# 
# Resolve whatever libcrypto.so is present to the one that ipmiutil 
# was built to reference.
# The default build on RHEL4.4 references libcrypto.so.4
libver=4
found=0

# check where the libcrypto.so is located
dirs="/usr/lib64 /lib64 /usr/lib /lib"
for d in $dirs
do
   libt=`ls $d/libcrypto.so.0.* 2>/dev/null |tail -n1`
   if [ "x$libt" != "x" ]; then
	# Found a libcrypto.so
	libcry=$libt
	libdir=$d
	found=1
	libnew=$libdir/libcrypto.so.$libver
	echo "libcry=$libcry libdir=$libdir libnew=$libnew"
	if [ ! -f $libnew ]; then
	   # Need a sym-link to resolve it
	   echo "ln -s $libcry $libnew"
	   ln -s $libcry $libnew
	   ldconfig
	fi
   fi
done

if [ $found -eq 0 ]; then
   echo "libcrypto.so not found, install openssl rpm"
   rv=1
else
   rv=0
fi
exit $rv

