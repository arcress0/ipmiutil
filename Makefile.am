
SUBDIRS = @SUBDIR_S@
PKGDIR = @PKG_DIR@
SOURCEDIR = $(PKGDIR)/SOURCES
BUILDDIR = $(PKGDIR)/BUILD
RSPEC = doc/$(PACKAGE).spec
PNAME  = $(PACKAGE)-$(VERSION)
TARDIR = /tmp/iu
TMPDIR = $(TARDIR)/$(PNAME)
BDIR = $(TMPDIR)-1-root
RPMB = rpmbuild
# $(RPMB) -bb --buildroot $(BDIR) $(RSPEC)

# Note: put files here, until lib/Makefile.am supports
# pkg-config support rules
# if WITH_PKG_CONFIG
 # pkgconfig_DATA = lib/libipmiutil.pc
# endif

rpm:	$(RSPEC)
	make tarball
	mkdir -p $(SOURCEDIR)
	cp -f $(TARDIR)/$(PNAME).tar.gz $(SOURCEDIR)
	$(RPMB) -bb  $(RSPEC)
	$(RPMB) -bs  $(RSPEC)

tarball:
	rm -f $(TMPDIR)/../$(PNAME).tar.gz 
	rm -rf $(TMPDIR)
	mkdir -p $(TMPDIR)
	find . -print | cpio -pdumv $(TMPDIR)
	cd $(TMPDIR)/test; make clean; cd ..
	cd $(TMPDIR)/hpiutil; make clean; cd ..
	cd $(TMPDIR); make distclean
	cd $(TMPDIR); rm -rf util/.deps lib/lanplus/.deps 
	cd $(TMPDIR); rm -f lib/libipmiapi*.a* lib/libipmi*.a
	cd $(TMPDIR); rm -rf autom4te.cache
	cd $(TMPDIR); rm -f  util/events
	cd $(TMPDIR); sed -i 's/\r//' ChangeLog AUTHORS COPYING NEWS README TODO doc/UserGuide
	cd $(TMPDIR); chmod 644 doc/UserGuide
	cd $(TMPDIR); rm -f  cprpm  debug*.list *~ .*.swp 
	cd $(TMPDIR); find . -name '*.orig' -delete
	cd $(TMPDIR); find . -name '*.rej' -delete
	cd $(TMPDIR); cd ..; tar -czvf $(TARDIR)/$(PNAME).tar.gz $(PNAME)
	ls $(TARDIR)/*.gz
	rm -rf $(TMPDIR)
# chown -R root:root $(TMPDIR)

tarsol:        util/ipmiutil
	rm -f /tmp/$(PNAME)-solaris.tar
	rm -rf $(TMPDIR)
	mkdir -p $(TMPDIR)
	cd util; cp ipmiutil idiscover ievents ipmi_port $(TMPDIR)
	cd doc; cp UserGuide *.8 *.mib  $(TMPDIR)
	cd scripts; cp checksel ipmiutil_??? *.sh $(TMPDIR)
	cd $(TMPDIR); mv install-solaris.sh install.sh
	cd $(TMPDIR); tar cvf /tmp/$(PNAME)-solaris.tar .
	rm -rf $(TMPDIR)

tarbsd:        util/ipmiutil
	rm -f /tmp/$(PNAME)-bsd.tar
	rm -rf $(TMPDIR)
	mkdir -p $(TMPDIR)
	cd util; cp ipmiutil idiscover ievents ipmi_port $(TMPDIR)
	cd doc; cp UserGuide *.8 *.mib  $(TMPDIR)
	cd scripts; cp checksel ipmiutil_??? *.sh $(TMPDIR)
	cd $(TMPDIR); mv install-solaris.sh install.sh
	cd $(TMPDIR); tar -cvf /tmp/$(PNAME)-bsd.tar .
	rm -rf $(TMPDIR)

clean-generic:
	rm -f *.log *.tmp debug*.list
