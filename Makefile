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
	mkdir -p $(SHARE)/cgi-bin
	chmod 755 $(SHARE)/cgi-bin
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
