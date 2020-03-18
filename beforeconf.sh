#!/bin/sh
#          beforeconf.sh
# Run this to reconfigure after unpacking the tarball, 
# before running ./configure
#
am_files="mkinstalldirs missing"
am_dir=`ls -d /usr/share/automake* |tail -n1`
lt_files="ltmain.sh config.guess config.sub"
lt_dir=/usr/share/libtool/libltdl
my_dir=`pwd`

#ltver=`rpm -q libtool |cut -f2 -d'-'`
ltver=`libtool --version | head -n1 |awk '{ print $4 }'`
ltv1=`echo $ltver |cut -f1 -d'.'`
if [ "$ltv1" != "1" ]; then
   # many distros have libtool-2.x.x
   lt_dir="/usr/share/libtool/config"
   lt_files="compile config.guess config.sub depcomp install-sh ltmain.sh missing"
fi

cd $my_dir
rm -rf autom4te.cache
aclocal

# Note that MacOS Darwin returns 0 to which even if error.
which libtoolize  >/dev/null 2>&1
if [ $? -eq 0 ]; then
  # use libtoolize if possible
  cd $my_dir
  libtoolize --automake --copy --force
elif [ -d $lt_dir ]; then
  cp -f /usr/bin/libtool $my_dir
  cd $lt_dir
  cp -f ${lt_files} $my_dir
fi
# copy missing am_files if needed 
if [ -d $am_dir ]; then
  cd $am_dir
  for f in ${am_files}
  do
     if [ ! -f $my_dir/$f ]; then
        cp -f $f $my_dir
     fi
  done
fi

cd $my_dir
# autoreconf -fiv
autoheader
automake --foreign --add-missing --copy

aclocal
autoconf 
automake --foreign

