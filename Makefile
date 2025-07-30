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

# Application build. --------------------------------------------

OBJS=housecgi.o housecgi_route.o housecgi_execute.o
LIBOJS=

all: housecgi

main: housecgi.o

clean:
	rm -f *.o *.a housecgi

rebuild: clean all

%.o: %.c
	gcc -c -g -Os -o $@ $<

housecgi: $(OBJS)
	gcc -g -Os -o housecgi $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lm -lrt

# Special git installation (Git is scyzophrenic about _who_ accesses) -----

# This target must be launched as "sudo make USERNAME=`whoami` install-git",
# preferably after the standard housecgi was installed.
# This automatically configures the git-http-backend CGI application.
# If you want to run cgit in addition to git-http-backend, you will need to
# build and add cgit manually (using "sudo housecgiadd --instance=cgigit cgit")
# because "apt install cgit" forces install of Apache, a hog of a web server..
# Only systemd is supported here. Support for runit is left as an excercise..

install-git:
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/public/cgigit
	sed 's/\/cgi/\/cgigit/' < public/index.html > $(DESTDIR)$(SHARE)/public/cgigit/index.html
	sed 's/\/cgi/\/cgigit/' < public/events.html > $(DESTDIR)$(SHARE)/public/cgigit/events.html
	chmod 644 $(DESTDIR)$(SHARE)/public/cgigit/*
	$(INSTALL) -m 0755 -d $(DESTDIR)/var/lib/house/cgigit-bin
	$(INSTALL) -m 0755 -T githttp.sh $(DESTDIR)/var/lib/house/cgigit-bin/githttp
	if [ "x$(DESTDIR)" = "x" ] ; then if [ -e /lib/systemd/system/housegit.service ] ; then systemctl stop housegit ; fi ; fi
	sed 's/\/housecgi /\/housecgi --instance=cgigit /' < systemd.service | sed "s/User=house/User=$(USERNAME)/" | sed 's/CGI service/CGI service for Git/' > $(DESTDIR)/lib/systemd/system/housegit.service
	chown root:root /lib/systemd/system/housegit.service
	if [ "x$(DESTDIR)" = "x" ] ; then systemctl daemon-reload ; systemctl enable housegit ; systemctl start housegit ; fi

uninstall-git:
	systemctl stop housegit
	systemctl disable housegit
	rm -f $(DESTDIR)/lib/systemd/system/housegit.service
	rm -rf $(DESTDIR)$(SHARE)/public/cgigit
	rm -rf $(DESTDIR)/var/lib/house/cgigit-bin

# Application files installation --------------------------------

install-ui: install-preamble
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/public/cgi
	$(INSTALL) -m 0644 public/* $(DESTDIR)$(SHARE)/public/cgi

install-app: install-ui
	$(INSTALL) -m 0755 -s housecgi $(DESTDIR)$(prefix)/bin
	$(INSTALL) -m 0755 -T housecgiadd.sh $(DESTDIR)$(prefix)/bin/housecgiadd
	$(INSTALL) -m 0755 -T housecgiremove.sh $(DESTDIR)$(prefix)/bin/housecgiremove
	$(INSTALL) -m 0755 -d $(DESTDIR)/var/lib/house/cgi-bin
	touch $(DESTDIR)/etc/default/housecgi

uninstall-app:
	rm -rf $(DESTDIR)$(SHARE)/public/cgi
	rm -f $(DESTDIR)$(prefix)/bin/housecgi
	rm -f $(DESTDIR)$(prefix)/bin/housecgiadd
	rm -f $(DESTDIR)$(prefix)/bin/housecgiremove
	rm -rf $(DESTDIR)/var/lib/house/cgi-bin

purge-app:

purge-config:
	rm -rf $(DESTDIR)/etc/default/housecgi

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
