ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po lib src examples

.PHONY: debian rpm

debian:
	dpkg-buildpackage -rfakeroot -us -uc -sd -b

rpm: @PACKAGE@.spec
	fakeroot rpm --clean -bb @PACKAGE@.spec

EXTRA_DIST = \
  mkinstalldirs \
  MANIFEST.sources \
  debian/changelog \
  debian/control \
  debian/copyright \
  debian/rules

clean-local:
	-rm -rf debian/@PACKAGE@
	-rm -f debian/files debian/@PACKAGE@.postinst.debhelper \
	debian/@PACKAGE@.prerm.debhelper debian/@PACKAGE@.substvars \
	build-stamp configure-stamp
