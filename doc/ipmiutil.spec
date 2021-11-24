# spec file for package ipmiutil 
#
# Copyright (c) 2012 Andy Cress
#
Name:      ipmiutil
Version: 3.1.8
Release: 1%{?dist}
Summary:   Easy-to-use IPMI server management utilities
License:   BSD
Source:    http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
URL:       http://ipmiutil.sourceforge.net
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
%define group  System/Management
# Suggests: cron or vixie-cron or cronie or similar
%{!?_unitdir: %define _unitdir  /usr/lib/systemd/system}
%define unit_dir  %{_unitdir}
%define init_dir  %{_initrddir}
%define systemd_fls %{_datadir}/%{name}
%define bldreq0   openssl-devel gcc gcc-c++
%define bldreq_extra libtool 
%if 0%{?fedora} >= 15
%define bldreq_extra systemd autoconf automake systemd-units
%define group System Environment/Base
Requires:  systemd-units
%if 0%{?fedora} == 16
%define unit_dir  /lib/systemd/system
%endif
%if 0%{?fedora} >= 25
%define bldreq_extra systemd autoconf automake systemd-units qrencode-libs
%endif
%endif
%if 0%{?rhel} >= 7
%define bldreq_extra autoconf automake systemd-units
%endif
%if 0%{?sles_version} > 10
%define bldreq0 libopenssl-devel 
%endif
%if 0%{?suse_version} >= 1210
%define bldreq_extra libtool systemd
%define req_systemd 1
%define systemd_fls %{unit_dir}
# Requires: %{?systemd_requires}
%endif
BuildRequires: %{bldreq0} %{bldreq_extra}
Group:     %{group}

%description
The ipmiutil package provides easy-to-use utilities to view the SEL,
perform an IPMI chassis reset, set up the IPMI LAN and Platform Event Filter
entries to allow SNMP alerts, Serial-Over-LAN console, event daemon, and 
other IPMI tasks.
These can be invoked with the metacommand ipmiutil, or via subcommand
shortcuts as well.  IPMIUTIL can also write sensor thresholds, FRU asset tags, 
and has a full IPMI configuration save/restore.
An IPMI driver can be provided by either the OpenIPMI driver (/dev/ipmi0)
or the Intel IPMI driver (/dev/imb), etc.  If used locally and no driver is
detected, ipmiutil will use user-space direct I/Os instead.

%package devel
Group:    Development/Libraries
Summary:  Includes libraries and headers for the ipmiutil package
Requires: ipmiutil

%description devel
The ipmiutil-devel package contains headers and libraries which are
useful for building custom IPMI applications.

%package static
Group:    Development/Libraries
Summary:  Includes static libraries for the ipmiutil package

%description static
The ipmiutil-static package contains static libraries which are
useful for building custom IPMI applications.

%prep
%setup -q

%build
%if 0%{?fedora} >= 15
autoconf
%endif
%if 0%{?req_systemd}
%configure --enable-systemd
%else
%configure
%endif
%if 0%{?fedora} >= 15
make %{?_smp_mflags}
%else
make 
%endif

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root, -)
%dir %{_datadir}/%{name}
%dir %{_var}/lib/%{name}
%{_bindir}/ipmiutil
%{_bindir}/idiscover
%{_bindir}/ievents
%{_sbindir}/iseltime
%{_sbindir}/ipmi_port
%{_sbindir}/ialarms 
%{_sbindir}/iconfig
%{_sbindir}/icmd 
%{_sbindir}/ifru 
%{_sbindir}/igetevent 
%{_sbindir}/ihealth 
%{_sbindir}/ilan 
%{_sbindir}/ireset 
%{_sbindir}/isel 
%{_sbindir}/isensor 
%{_sbindir}/iserial 
%{_sbindir}/isol 
%{_sbindir}/iwdt 
%{_sbindir}/ipicmg 
%{_sbindir}/ifirewall 
%{_sbindir}/ifwum 
%{_sbindir}/ihpm 
%{_sbindir}/iuser 
%{_libdir}/libipmiutil.so.1
%{_datadir}/%{name}/ipmiutil_evt
%{_datadir}/%{name}/ipmiutil_asy
%{_datadir}/%{name}/ipmiutil_wdt
%{_datadir}/%{name}/ipmi_port
%{_datadir}/%{name}/ipmi_info
%{_datadir}/%{name}/checksel
%{systemd_fls}/ipmiutil_evt.service
%{systemd_fls}/ipmiutil_asy.service
%{systemd_fls}/ipmiutil_wdt.service
%{systemd_fls}/ipmi_port.service
%{_datadir}/%{name}/ipmiutil.env.template
%{_datadir}/%{name}/ipmiutil.pre
%{_datadir}/%{name}/ipmiutil.setup
%{_datadir}/%{name}/ipmi_if.sh
%{_datadir}/%{name}/evt.sh
%{_datadir}/%{name}/ipmi.init.basic
%{_datadir}/%{name}/bmclanpet.mib
%{_mandir}/man8/isel.8*
%{_mandir}/man8/isensor.8*
%{_mandir}/man8/ireset.8*
%{_mandir}/man8/igetevent.8*
%{_mandir}/man8/ihealth.8*
%{_mandir}/man8/iconfig.8*
%{_mandir}/man8/ialarms.8*
%{_mandir}/man8/iwdt.8*
%{_mandir}/man8/ilan.8*
%{_mandir}/man8/iserial.8*
%{_mandir}/man8/ifru.8*
%{_mandir}/man8/icmd.8*
%{_mandir}/man8/isol.8*
%{_mandir}/man8/ipmiutil.8*
%{_mandir}/man8/idiscover.8*
%{_mandir}/man8/ievents.8*
%{_mandir}/man8/ipmi_port.8*
%{_mandir}/man8/ipicmg.8*
%{_mandir}/man8/ifirewall.8*
%{_mandir}/man8/ifwum.8*
%{_mandir}/man8/ihpm.8*
%{_mandir}/man8/isunoem.8*
%{_mandir}/man8/idelloem.8*
%{_mandir}/man8/ismcoem.8*
%{_mandir}/man8/iekanalyzer.8*
%{_mandir}/man8/itsol.8*
%{_mandir}/man8/idcmi.8*
%{_mandir}/man8/iuser.8*
%{_mandir}/man8/iseltime.8*
%doc AUTHORS ChangeLog COPYING NEWS README TODO 
%doc doc/UserGuide

%files devel
%defattr(-,root,root)
# .{_datadir}/.{name} is used by both ipmiutil and ipmiutil-devel
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/ipmi_sample.c
%{_datadir}/%{name}/ipmi_sample_evt.c
%{_datadir}/%{name}/isensor.c
%{_datadir}/%{name}/ievents.c
%{_datadir}/%{name}/isensor.h
%{_datadir}/%{name}/ievents.h
%{_datadir}/%{name}/Makefile
%{_libdir}/libipmiutil.so
%{_includedir}/ipmicmd.h

%files static
%defattr(-,root,root)
%{_libdir}/libipmiutil.a

%post devel
/sbin/ldconfig

%postun devel
/sbin/ldconfig

%pre
%if 0%{?req_systemd}
%service_add_pre ipmi_port.service ipmiutil_evt.service ipmiutil_asy.service ipmiutil_wdt.service
%endif

%post
/sbin/ldconfig
# POST_INSTALL, $1 = 1 if rpm -i, $1 = 2 if rpm -U
vardir=%{_var}/lib/%{name}
scr_dir=%{_datadir}/%{name}

# Install right scripts/service files no matter install or upgrade
%if 0%{?req_systemd}
%service_add_post ipmi_port.service ipmiutil_evt.service ipmiutil_asy.service ipmiutil_wdt.service
%else
   if [ -x /bin/systemctl ] && [ -d %{unit_dir} ]; then
      # Replace if exists, append if not.
      # Use # as the sed delimiter to prevent handling slash in the path.
      if [ ! -f %{_datadir}/%{name}/ipmiutil.env ]; then
         cp %{_datadir}/%{name}/ipmiutil.env.template %{_datadir}/%{name}/ipmiutil.env
      fi
      grep -q 'IINITDIR' %{_datadir}/%{name}/ipmiutil.env \
         && sed -i 's#^IINITDIR=.*#IINITDIR=%{init_dir}#' %{_datadir}/%{name}/ipmiutil.env \
         || echo "IINITDIR=%{init_dir}" >> %{_datadir}/%{name}/ipmiutil.env
      cp -f ${scr_dir}/ipmiutil_evt.service %{unit_dir}
      cp -f ${scr_dir}/ipmiutil_asy.service %{unit_dir}
      cp -f ${scr_dir}/ipmiutil_wdt.service %{unit_dir}
      cp -f ${scr_dir}/ipmi_port.service    %{unit_dir}
      # systemctl enable ipmi_port.service >/dev/null 2>&1 || :
   else 
      cp -f ${scr_dir}/ipmiutil_wdt %{init_dir}
      cp -f ${scr_dir}/ipmiutil_asy %{init_dir}
      cp -f ${scr_dir}/ipmiutil_evt %{init_dir}
      cp -f ${scr_dir}/ipmi_port    %{init_dir}
      cp -f ${scr_dir}/ipmi_info    %{init_dir}
   fi
%endif

if [ "$1" = "1" ]
then
   # Test whether an IPMI interface is known to the motherboard
   IPMIret=1
   which dmidecode >/dev/null 2>&1 && IPMIret=0
   if [ $IPMIret -eq 0 ]; then
    IPMIret=1
    %{_sbindir}/dmidecode |grep -q IPMI && IPMIret=0
    if [ $IPMIret -eq 0 ]; then
     # Run some ipmiutil command to see if any IPMI interface works.
     # Some may not have IPMI on the motherboard, so need to check, but
     # some kernels may have IPMI driver partially loaded, which breaks this
     IPMIret=1
     %{_bindir}/ipmiutil sel -v >/dev/null 2>&1 && IPMIret=0
     if [ $IPMIret -eq 0 ]; then
      if [ ! -x %{init_dir}/ipmi ]; then
         cp -f ${scr_dir}/ipmi.init.basic  %{init_dir}/ipmi
      fi
      # If IPMI is enabled, automate managing the IPMI SEL
      if [ -d %{_sysconfdir}/cron.daily ]; then
         cp -f %{_datadir}/%{name}/checksel %{_sysconfdir}/cron.daily
      fi
      # IPMI_IS_ENABLED, so enable services, but only if Red Hat
      if [ -f /etc/redhat-release ]; then
         if [ -x /bin/systemctl ]; then
            touch ${scr_dir}/ipmi_port.service
         elif [ -x /sbin/chkconfig ]; then
            /sbin/chkconfig --add ipmi_port
            /sbin/chkconfig --add ipmi_info
            # /sbin/chkconfig --add ipmiutil_wdt
            # /sbin/chkconfig --add ipmiutil_evt 
         fi
      fi
   
      # Capture a snapshot of IPMI sensor data once now for later reuse.
      sensorout=$vardir/sensor_out.txt
      if [ ! -f $sensorout ]; then
         %{_bindir}/ipmiutil sensor -q >$sensorout || :
         if [ $? -ne 0 ]; then
           # remove file if error, try again in ipmi_port on reboot.
           rm -f $sensorout
         fi
      fi
     fi
    fi
   fi
else
   # postinstall, doing rpm update
   IPMIret=1
   which dmidecode >/dev/null 2>&1 && IPMIret=0
   if [ $IPMIret -eq 0 ]; then
    IPMIret=1
    %{_sbindir}/dmidecode |grep -q IPMI && IPMIret=0
    if [ $IPMIret -eq 0 ]; then
      if [ -d %{_sysconfdir}/cron.daily ]; then
         cp -f %{_datadir}/%{name}/checksel %{_sysconfdir}/cron.daily
      fi
    fi
   fi
fi
%if 0%{?fedora} >= 18 || 0%{?rhel} >= 7
%systemd_post  ipmiutil_evt.service
%systemd_post  ipmiutil_asy.service
%systemd_post  ipmiutil_wdt.service
%systemd_post  ipmi_port.service
%endif

%preun
# before uninstall,  $1 = 1 if rpm -U, $1 = 0 if rpm -e
if [ "$1" = "0" ]
then
%if 0%{?req_systemd}
%service_del_preun ipmi_port.service ipmiutil_evt.service ipmiutil_asy.service ipmiutil_wdt.service
%else
   if [ -x /bin/systemctl ]; then
     if [ -f %{unit_dir}/ipmiutil_evt.service ]; then
%if 0%{?fedora} >= 18 || 0%{?rhel} >= 7
%systemd_preun  ipmiutil_evt.service
%systemd_preun  ipmiutil_asy.service
%systemd_preun  ipmiutil_wdt.service
%systemd_preun  ipmi_port.service
%else
   ret=1
   which systemctl >/dev/null 2>&1 && ret=0
   if [ $ret -eq 0 ]; then
        systemctl disable ipmi_port.service >/dev/null 2>&1 || :
        systemctl disable ipmiutil_evt.service >/dev/null 2>&1 || :
        systemctl disable ipmiutil_asy.service >/dev/null 2>&1 || :
        systemctl disable ipmiutil_wdt.service >/dev/null 2>&1 || :
        systemctl stop ipmiutil_evt.service >/dev/null 2>&1 || :
        systemctl stop ipmiutil_asy.service >/dev/null 2>&1 || :
        systemctl stop ipmiutil_wdt.service >/dev/null 2>&1 || :
        systemctl stop ipmi_port.service    >/dev/null 2>&1 || :
   fi
%endif
     fi
   else 
     if [ -x /sbin/service ]; then
        /sbin/service ipmi_port stop       >/dev/null 2>&1 || :
        /sbin/service ipmiutil_wdt stop    >/dev/null 2>&1 || :
        /sbin/service ipmiutil_asy stop    >/dev/null 2>&1 || :
        /sbin/service ipmiutil_evt stop    >/dev/null 2>&1 || :
     fi
     if [ -x /sbin/chkconfig ]; then
        /sbin/chkconfig --del ipmi_port    >/dev/null 2>&1 || :
        /sbin/chkconfig --del ipmiutil_wdt >/dev/null 2>&1 || :
        /sbin/chkconfig --del ipmiutil_asy >/dev/null 2>&1 || :
        /sbin/chkconfig --del ipmiutil_evt >/dev/null 2>&1 || :
     fi
   fi
%endif
   if [ -f %{_sysconfdir}/cron.daily/checksel ]; then
        rm -f %{_sysconfdir}/cron.daily/checksel
   fi
fi

%postun
# after uninstall,  $1 = 1 if update, $1 = 0 if rpm -e
/sbin/ldconfig
# Remove files from scr_dir only when uninstall
if [ "$1" = "0" ]
then
   if [ -x /bin/systemctl ] && [ -d %{unit_dir} ]; then
      if [ -f %{unit_dir}/ipmiutil_evt.service ]; then
         rm -f %{unit_dir}/ipmiutil_evt.service  2>/dev/null || :
         rm -f %{unit_dir}/ipmiutil_asy.service  2>/dev/null || :
         rm -f %{unit_dir}/ipmiutil_wdt.service  2>/dev/null || :
         rm -f %{unit_dir}/ipmi_port.service     2>/dev/null || :
      fi
   else
      if [ -f %{init_dir}/ipmiutil_evt.service ]; then
         rm -f %{init_dir}/ipmiutil_wdt 2>/dev/null || :
         rm -f %{init_dir}/ipmiutil_asy 2>/dev/null || :
         rm -f %{init_dir}/ipmiutil_evt 2>/dev/null || :
         rm -f %{init_dir}/ipmi_port    2>/dev/null || :
      fi
   fi
fi

%if 0%{?req_systemd}
%service_del_postun ipmi_port.service ipmiutil_evt.service ipmiutil_asy.service ipmiutil_wdt.service
%else
%if 0%{?fedora} >= 18 || 0%{?rhel} >= 7
%systemd_postun_with_restart ipmi_port.service
%systemd_postun_with_restart ipmiutil_evt.service
%systemd_postun_with_restart ipmiutil_asy.service
%systemd_postun_with_restart ipmiutil_wdt.service
%else
ret=1
which systemctl >/dev/null 2>&1 && ret=0
if [ $ret -eq 0 ]; then
 systemctl daemon-reload  || :
 if [ $1 -ge 1 ]; then
   # Package upgrade, not uninstall
   systemctl try-restart ipmi_port.service     || :
   systemctl try-restart ipmiutil_evt.service  || :
   systemctl try-restart ipmiutil_asy.service  || :
   systemctl try-restart ipmiutil_wdt.service  || :
 fi
fi
%endif
%endif

%changelog
* Mon Oct 29 2018 Andrew Cress <arcress at users.sourceforge.net> 3.1.3-1
- clean up systemd scripts with macros, fix update for systemd (Aska Wu)
* Fri Jul 20 2018 Andrew Cress <arcress at users.sourceforge.net> 3.1.2-1
- resolve doubly-defined Group for Fedora
* Mon Jun 29 2015 Andrew Cress <arcress at users.sourceforge.net> 2.9.7-1
- move libipmiutil.so from devel into ipmiutil base package (RH#1177213)
* Mon Nov 03 2014 Andrew Cress <arcress at users.sourceforge.net> 2.9.5-1
- separate libipmiutil.a into ipmiutil-static package
* Thu Aug 28 2014 Andrew Cress <arcress at users.sourceforge.net> 2.9.4-1
- Updated to ipmiutil-2.9.4
* Tue Aug 21 2012 Andrew Cress <arcress at users.sourceforge.net> 2.8.5-2
  Added F18 systemd macros for RH bug #850163
* Fri May 04 2012 Andrew Cress <arcress at users.sourceforge.net> 2.8.4-1
  Fixups for devel rpm (RH bug #818910)
* Tue Apr 24 2012 Andrew Cress <arcress at users.sourceforge.net> 2.8.3-1
  Use service_* macros if req_systemd is set
* Thu Mar 08 2012 Andrew Cress <arcress at users.sourceforge.net> 2.8.2-1
  reworked systemd logic/macros, moved ipmiutil from sbindir to bindir
* Mon Dec 12 2011 Andrew Cress <arcress at users.sourceforge.net> 2.8.0-1
  added devel package files
* Fri Nov 11 2011 Andrew Cress <arcress at users.sourceforge.net> 2.7.9-3
  fix RH bug #752319 to not copy checksel to cron.daily if IPMI not enabled
* Tue Sep 13 2011 Andrew Cress <arcress at users.sourceforge.net> 2.7.8-1
  added systemd scripts, added idelloem.8
* Mon Jun 06 2011 Andrew Cress <arcress at users.sourceforge.net> 2.7.7-1
  add gcc,gcc-c++ to BuildRequires to detect broken build systems
* Mon May 09 2011 Andrew Cress <arcress at users.sourceforge.net> 2.7.6-1
  updated ipmiutil
* Fri Nov 12 2010 Andrew Cress <arcress at users.sourceforge.net> 2.7.3-1
  updated package description
* Fri Oct 15 2010 Andrew Cress <arcress at users.sourceforge.net> 2.7.1-1
  skip chkconfig --add if not Red Hat
* Mon Sep 27 2010 Andrew Cress <arcress at users.sourceforge.net> 2.7.0-1
  added fwum, hpm, sunoem, ekanalyzer man pages
* Mon Jul 19 2010 Andrew Cress <arcress at users.sourceforge.net> 2.6.8-1
  cleaned up two more rpmlint issues 
* Mon Jul 12 2010 Andrew Cress <arcress at users.sourceforge.net> 2.6.7-1
  cleaned up some rpmlint issues, include ipmiutil_evt in chkconfig's
* Thu Apr 29 2010 Andrew Cress <arcress at users.sourceforge.net> 2.6.4-1
  cleaned up some style issues
* Fri Mar  5 2010 Andrew Cress <arcress at users.sourceforge.net> 2.6.1-1
  cleaned up some style issues
* Tue Feb 16 2010 Andrew Cress <arcress at users.sourceforge.net> 2.6.0-1
  cleaned up some script clutter, changed naming scheme for sub-commands
* Tue Jan 26 2010 Andrew Cress <arcress at users.sourceforge.net> 2.5.3-1
  cleaned up some rpmlint issues, removed bmclanaol.mib
* Mon Nov  9 2009 Andrew Cress <arcress at users.sourceforge.net> 2.5.1-1
  do not gzip man files, clean up scripts, move distro specifics to configure
* Tue Jun 23 2009 Andrew Cress <arcress at users.sourceforge.net> 2.4.0-1
  moved all progs to sbin, install init/cron scripts via files not post
* Wed Dec 10 2008 Andrew Cress <arcress at users.sourceforge.net> 2.3.2-1
  changes for Fedora with ipmiutil-2.3.2
* Fri Jun 08 2007 Andrew Cress <arcress at users.sourceforge.net>
  rpmlint tweaks for ipmiutil-1.9.8
* Mon May 21 2007 Andrew Cress <arcress at users.sourceforge.net>
  added isroot flag for chroot cases
* Fri May 18 2007 Andrew Cress <arcress at users.sourceforge.net>
  added ipmi_port init handling
* Mon Jul 10 2006 Andrew Cress <arcress at users.sourceforge.net>
  changed to libfreeipmi.so.2, include and run ipmi_if.sh
* Tue Aug 02 2005 Andrew Cress <arcress at users.sourceforge.net> 
  changed not to run pefconfig if already configured
* Thu Feb 03 2005 Andrew Cress <arcress at users.sourceforge.net> 
  changed /usr/man to /usr/share/man,
  fixed postun to recognize rpm -U via arg 1 
* Mon Nov 1 2004 Andrew Cress <arcress at users.sourceforge.net> 
  added freeipmi install files and logic
* Mon Aug 23 2004 Andrew Cress <arcress at users.sourceforge.net> 
- added MIB links to /usr/share/snmp/mibs
* Tue Aug 10 2004 Andrew Cress <arcress at users.sourceforge.net> 
- added icmd utility to the rpm
* Thu Aug 05 2004 Andrew Cress <arcress at users.sourceforge.net> 
- added special logic for SuSE snmpd.conf
* Fri Apr 02 2004 Andrew Cress <arcress at users.sourceforge.net> 
- added checksel cron job
* Tue Jan 28 2003 Andrew Cress <arcress at users.sourceforge.net> 
- added sensor & fruconfig for ipmiutil 1.2.8
* Fri Aug  2 2002 Andrew Cress <arcress at users.sourceforge.net> 
- fixed bug 793 (dont need Require:ipmidrvr) for ipmiutil 1.2.2
* Tue Jul  2 2002 Andrew Cress <arcress at users.sourceforge.net> 
- fixed bug 555 in showsel for ipmiutil 1.2.1
* Fri May 10 2002 Andrew Cress <arcress at users.sourceforge.net> 
- fixed bug 504 in pefconfig for ipmiutil 1.1.5
* Thu Apr 11 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated pathnames for ipmiutil 1.1.4, some cleanup
* Mon Mar 18 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.1.3-2, added checking for grub vs. lilo to .spec
* Tue Mar 12 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.1.3, added source rpm, changed license, etc.
* Thu Jan 31 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.1.0-2, changed selpef to pefconfig
* Fri Jan 25 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.1.0, changed to ipmidrvr rather than isc dependency
* Wed Jan 16 2002 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.1.0, added hwreset utility
* Fri Dec 14 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 1.0.0, man page updates
* Mon Nov 19 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 0.9.0, uses new OSS bmc_panic, so don't install module.
* Tue Nov 13 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 0.8.5, add "Requires: isc" (#32), hide selpef output (#38)
* Thu Nov  8 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 0.8.4, eliminate "file exists" messages by fixing removal
* Thu Oct 25 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 0.8.2, run selpef (objdump:applypatch gives bogus warning)
* Thu Oct 25 2001 Andrew Cress <arcress at users.sourceforge.net> 
- updated for 0.8.2, run selpef (objdump:applypatch gives bogus warning)
* Wed Oct 24 2001 Andrew Cress <arcress at users.sourceforge.net> 
- created ipmiutil package 0.8.1 without kbuild
* Tue Oct 23 2001 Andrew Cress <arcress at users.sourceforge.net> 
- created ipmiutil package 0.8.0
