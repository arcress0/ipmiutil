Name: hpiutil
%define Version 1.1.11
Version: %Version
Release: 1
Summary: Contains HPI server management utilities and library.
License: BSD
Group: System/Management
Source: hpiutil-%Version.tar.gz
URL: http://ipmiutil.sourceforge.net
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires: gcc 

%ifarch x86_pentium3
AutoReqProv: No
%endif
%ifarch x86_pentium4
AutoReqProv: No
%endif

%description
The HPI utilities package provides system management utilities that 
conform to the SA Forum's Hardware Platform Interface specification, and
as such are hardware-independent across platforms that have an HPI
library implementation.  The HPI library on Intel platforms requires
an IPMI driver.  An IPMI driver can be provided by either the Intel 
IPMI driver (/dev/imb) or the OpenIPMI driver (/dev/ipmi0) in Linux 
kernel versions 2.4.20 and greater.

This package includes the HPI binary libraries and the following 
HPI utilities.
  hpisensor
  hpisel
  hpireset
  hpiwdt
  hpifru
  hpialarmpanel

%prep
#%setup -q

#%build
#sh configure
#make

#%install
#RPM_BUILD_ROOT=`pwd`
#make DESTDIR=${RPM_BUILD_ROOT} install
#( cd ${RPM_BUILD_ROOT}/usr/man/man8; gzip -f *.8 ) 

%files
%defattr(0755,root,root)
/usr/bin/hpifru
/usr/bin/hpisensor
/usr/bin/hpialarmpanel
/usr/bin/hpisel
/usr/bin/hpiwdt
/usr/bin/hpireset
/usr/bin/SpiLibd
/usr/lib/libSaHpi.so
/usr/lib/libSpiDaemonCore.so
/usr/lib/libSpiModGeneric.so
/usr/lib/libSpiModIpmi.so
/usr/lib/libSpiModIpmi.so-open
/usr/lib/libSpiIpmiImb.so
/usr/lib/libSpiIpmiOpenIpmi.so
/usr/lib/libSpiTsdMaplx.so
/etc/hpi/hpiinit.sh
/usr/share/hpiutil/env.hpi
%defattr(0664,root,root)
/usr/share/hpiutil/README
/usr/share/hpiutil/COPYING
/etc/hpi/spi-daemon.conf
/etc/hpi/spi-lib.conf
# %defattr(-,root,root)
# %doc README TODO COPYING ChangeLog

%pre
# before install
sdir=/usr/share/hpiutil
edir=/etc/hpi
echo "Installing HPI Utilities ..."
mkdir -p $sdir
mkdir -p $edir

# Check for an IPMI driver 
rpm -qa |grep ipmidrvr >/dev/null
if [ $? -ne 0 ]
then
  # Intel ipmidrvr package is not installed, but other IPMI drivers 
  # could also be used, so test for device files.
  dev1=/dev/imb
  dev2=/dev/ipmi0
  dev3=/dev/ipmi/0
  dev4=/dev/ipmikcs
  if [ ! -c $dev1 ]
  then
    if [ ! -c $dev2 ]
    then
       if [ ! -c $dev3 ]
       then
          echo "WARNING: No IPMI devices found ($dev1, $dev2 or $dev3)."
          echo "The HPI utilities depend on an IPMI driver. "
       fi
    fi
  fi
fi

%post
# after install
sdir=/usr/share/hpiutil
edir=/etc/hpi

echo "hpiutil install started `date`" 
# Assumes that the kernel modules are already in place.

# The spi-daemon.conf file should have 'localhost' as the
# server name.  User can modify this if remote.

# set up the init.d jobs
loadhpi=/etc/hpi/hpiinit.sh
if [ -d /etc/rc.d/init.d ]
then
        # RedHat init.d structure
        cd /etc/rc.d/init.d
        cp $loadhpi  ./hpi
	echo "To autostart hpi, enter: chkconfig --add hpi"
#	chkconfig --add hpi
#	Or manually create links
#        cd ../rc3.d
#        ln -s ../init.d/hpi S81hpi 2>/dev/null
#        ln -s ../init.d/hpi K35hpi 2>/dev/null
#        cd ../rc5.d
#        ln -s ../init.d/hpi S81hpi 2>/dev/null
#        ln -s ../init.d/hpi K35hpi 2>/dev/null
#        cd ../rc6.d
#        ln -s ../init.d/hpi K35hpi 2>/dev/null
#        cd ../rc0.d
#        ln -s ../init.d/hpi K35hpi 2>/dev/null
else
        # SuSE init.d structure
        cd /etc/init.d
        cp $loadhpi  ./hpi
	echo "To autostart hpi, enter: /usr/lib/lsb/install_initd /etc/init.d/hpi "
#	/usr/lib/lsb/install_initd /etc/init.d/hpi 
#	Or manually create links
#        cd rc3.d
#        ln -s ../hpi S81hpi 2>/dev/null
#        ln -s ../hpi K35hpi 2>/dev/null
#        cd ../rc5.d
#        ln -s ../hpi S81hpi 2>/dev/null
#        ln -s ../hpi K35hpi 2>/dev/null
#        cd ../rc6.d
#        ln -s ../hpi K35hpi 2>/dev/null
#        cd ../rc0.d
#        ln -s ../hpi K35hpi 2>/dev/null
fi

echo "done `date`"

%preun
# before uninstall
echo "Uninstalling HPI Utilities feature ..."
/etc/hpi/hpiinit.sh stop  2>/dev/null

%postun
# after uninstall, clean up anything left over
sdir=/usr/share/hpiutil
edir=/etc/hpi
tmped=/tmp/edmk.tmp

if [ -d /etc/rc.d/init.d ]
then
        # RedHat init.d structure
#	chkconfig --del hpi
	cd /etc/rc.d/init.d
	rm -f hpi ../rc?.d/S81hpi ../rc?.d/K35hpi
else
        # SuSE init.d structure
#	/usr/lib/lsb/remove_initd /etc/init.d/hpi
        cd /etc/init.d
	rm -f hpi rc?.d/S81hpi rc?.d/K35hpi
fi
rm -rf $sdir  2>/dev/null

%changelog
* Tue Apr 06 2004 Andrew Cress <arcress at users.sourceforge.net> 
  changed to not turn on hpi autostart scripts at rpm install
* Fri Mar 26 2004 Andrew Cress <arcress at users.sourceforge.net> 
- changed to include proper kill scripts and chkconfig info
* Thu Feb 12 2004 Andrew Cress <arcress at users.sourceforge.net> 
- changed naming from /etc/init.d/hpiinit.sh to /etc/init.d/hpi
* Fri Jun 27 2003 Andrew Cress <arcress at users.sourceforge.net> 
- updated to check for ipmidrvr rpm, since no /dev/imb until reboot.
* Fri Jun 20 2003 Andrew Cress <arcress at users.sourceforge.net> 
- updated for README & released file locations
* Thu Jun 12 2003 Andrew Cress <arcress at users.sourceforge.net> 
- updated for beta2 file naming
* Tue May 05 2003 Andrew Cress <arcress at users.sourceforge.net> 
- created
