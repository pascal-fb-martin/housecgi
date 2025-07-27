# Housecgi - A simple home web service to support CGI applications.
#
# Copyright 2023, Pascal Martin
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

HAPP=housecgi
HROOT=/usr/local
SHARE=$(HROOT)/share/house

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

install-git:
	mkdir -p $(SHARE)/public/cgigit
	sed 's/\/cgi/\/cgigit/' < public/index.html > $(SHARE)/public/cgigit/index.html
	sed 's/\/cgi/\/cgigit/' < public/events.html > $(SHARE)/public/cgigit/events.html
	chmod 644 $(SHARE)/public/cgigit/*
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/cgigit
	mkdir -p /var/lib/house/cgigit-bin
	chmod 755 /var/lib/house/cgigit-bin
	cp githttp.sh /var/lib/house/cgigit-bin/githttp
	chmod 755 /var/lib/house/cgigit-bin/githttp
	if [ -e /lib/systemd/system/housegit.service ] ; then systemctl stop housegit ; fi
	sed 's/\/housecgi /\/housecgi --instance=cgigit /' < systemd.service | sed "s/User=house/User=$(USERNAME)/" | sed 's/CGI service/CGI service for Git/' > /lib/systemd/system/housegit.service
	chown root:root /lib/systemd/system/housegit.service
	systemctl daemon-reload
	systemctl enable housegit
	systemctl start housegit

uninstall-git:
	systemctl stop housegit
	systemctl disable housegit
	rm -f /lib/systemd/system/housegit.service
	rm -rf $(SHARE)/public/cgigit
	rm -rf /var/lib/house/cgigit-bin

# Application files installation --------------------------------

install-ui:
	mkdir -p $(SHARE)/public/cgi
	cp public/* $(SHARE)/public/cgi
	chmod 644 $(SHARE)/public/cgi/*
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/cgi

install-app: install-ui
	mkdir -p $(HROOT)/bin
	mkdir -p /var/lib/house
	mkdir -p /etc/house
	rm -f $(HROOT)/bin/housecgi
	cp housecgi $(HROOT)/bin
	chown root:root $(HROOT)/bin/housecgi
	chmod 755 $(HROOT)/bin/housecgi
	cp housecgiadd.sh $(HROOT)/bin/housecgiadd
	cp housecgiremove.sh $(HROOT)/bin/housecgiremove
	chown root:root $(HROOT)/bin/housecgiremove $(HROOT)/bin/housecgiadd
	chmod 755 $(HROOT)/bin/housecgiremove $(HROOT)/bin/housecgiadd
	mkdir -p /var/lib/house/cgi-bin
	chmod 755 /var/lib/house/cgi-bin
	touch /etc/default/housecgi

uninstall-app:
	rm -rf $(SHARE)/public/cgi
	rm -f $(HROOT)/bin/housecgi $(HROOT)/bin/housecgiadd $(HROOT)/bin/housecgiremove

purge-app:

purge-config:
	rm -rf /etc/default/housecgi

# System installation. ------------------------------------------

include $(SHARE)/install.mak

# Docker installation -------------------------------------------

docker: all
	rm -rf build
	mkdir -p build
	cp Dockerfile build
	mkdir -p build$(HROOT)/bin
	cp housecgi build$(HROOT)/bin
	chmod 755 build$(HROOT)/bin/housecgi
	mkdir -p build$(HROOT)/share/house/public/cgi
	cp public/* build$(HROOT)/share/house/public/cgi
	chmod 644 build$(HROOT)/share/house/public/cgi/*
	cp $(SHARE)/public/house.css build$(HROOT)/share/house/public
	chmod 644 build$(HROOT)/share/house/public/house.css
	cd build ; docker build -t housecgi .
	rm -rf build
