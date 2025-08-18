# Housecgi - A simple home web service to support CGI applications.
#
# Copyright 2025, Pascal Martin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#
# WARNING
#
# This Makefile depends on echttp and houseportal (dev) being installed.

prefix=/usr/local
SHARE=$(prefix)/share/house

INSTALL=/usr/bin/install

HAPP=housecgi
HCAT=infrastructure

# Application build. --------------------------------------------

OBJS=housecgi.o housecgi_route.o housecgi_execute.o
LIBOJS=

all: housecgi example

main: housecgi.o

clean:
	rm -f *.o *.a housecgi

rebuild: clean all

%.o: %.c
	gcc -c -g -Os -o $@ $<

housecgi: $(OBJS)
	gcc -g -Os -o housecgi $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lmagic -lm -lrt

example:
	cd test ; cc -O -o cgiexample cgiexample.c

# Application files installation --------------------------------

install-ui: install-preamble
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/public/cgi
	$(INSTALL) -m 0644 public/* $(DESTDIR)$(SHARE)/public/cgi

install-runtime: install-preamble
	$(INSTALL) -m 0755 -s housecgi $(DESTDIR)$(prefix)/bin
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/cgi
	$(INSTALL) -m 0755 githttp.sh $(DESTDIR)$(SHARE)/cgi
	$(INSTALL) -m 0755 -s test/cgiexample $(DESTDIR)$(SHARE)/cgi
	$(INSTALL) -m 0755 -T housecgiadd.sh $(DESTDIR)$(prefix)/bin/housecgiadd
	$(INSTALL) -m 0755 -T housecgiremove.sh $(DESTDIR)$(prefix)/bin/housecgiremove
	$(INSTALL) -m 0755 -T housecgigit.sh $(DESTDIR)$(prefix)/bin/housecgigit
	$(INSTALL) -m 0755 -d $(DESTDIR)/var/lib/house/cgi-bin
	touch $(DESTDIR)/etc/default/housecgi

install-app: install-ui install-runtime

uninstall-app:
	rm -rf $(DESTDIR)$(SHARE)/cgi
	rm -rf $(DESTDIR)$(SHARE)/public/cgi
	rm -f $(DESTDIR)$(prefix)/bin/housecgi
	rm -f $(DESTDIR)$(prefix)/bin/housecgiadd
	rm -f $(DESTDIR)$(prefix)/bin/housecgiremove
	rm -rf $(DESTDIR)/var/lib/house/cgi-bin

purge-app:

purge-config:
	rm -rf $(DESTDIR)/etc/default/housecgi

# Build a private Debian package. -------------------------------
#
# The user running the housecgi git instance is not defined here,
# because it would be the name of the user who built the package,
# not the name of the user who is installing it.
# Assigning the use here is obviously wrong, but k

install-package: install-ui install-runtime install-systemd

debian-package:
	rm -rf build
	install -m 0755 -d build/$(HAPP)/DEBIAN
	cat debian/control | sed "s/{{arch}}/`dpkg --print-architecture`/" > build/$(HAPP)/DEBIAN/control
	install -m 0644 debian/copyright build/$(HAPP)/DEBIAN
	install -m 0644 debian/changelog build/$(HAPP)/DEBIAN
	install -m 0755 debian/postinst build/$(HAPP)/DEBIAN
	install -m 0755 debian/prerm build/$(HAPP)/DEBIAN
	install -m 0755 debian/postrm build/$(HAPP)/DEBIAN
	make DESTDIR=build/$(HAPP) install-package
	cd build/$(HAPP) ; find etc -type f | sed 's/etc/\/etc/' > DEBIAN/conffiles
	cd build ; fakeroot dpkg-deb -b $(HAPP) .

# System installation. ------------------------------------------

include $(SHARE)/install.mak

# Docker installation -------------------------------------------

docker: all
	rm -rf build
	mkdir -p build
	cp Dockerfile build
	mkdir -p build$(prefix)/bin
	$(INSTALL) -m 0755 -s housecgi build$(prefix)/bin
	$(INSTALL) -m 0755 -d build$(prefix)/share/house/public/cgi
	$(INSTALL) -m 0644 public/* build$(prefix)/share/house/public/cgi
	$(INSTALL) -m 0644 $(SHARE)/public/house.css build$(prefix)/share/house/public
	cd build ; docker build -t housecgi .
	rm -rf build
