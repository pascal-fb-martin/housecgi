#!/bin/bash
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
# housecgigit: Install an additional HouseCGI service to run under
# the name of the user running "sudo".
#
# Typical use: "sudo housecgigit"
#
# If the Git user is not the current user, do: "sudo housecgigit --user=NAME"
# (replace NAME with the actual name of the Git user.)
#
# RESTRICTIONS:
#
# - This script does not allow for installing multiple HouseCGI git service.
#   The name of the systemd service is hardcoded. Support for multiple git
#   users is left as an excercise to the user.
#
# - There is no uninstall. Please stop and disable the systemd service.
#
# - cgit is not installed. It uses a custom version of git, its Debian
#   package brings Apache as a dependency (conflicting with houseportal)
#   It should be built separatly (from source). Also its configuration is
#   not trivial. See README.md on the housecgi github repository.
#
#   In a nutshell, a full installation of cgit involves 3 steps:
#   * build cgit.
#   * edit cgit configuration.
#   * run "sudo housecgiadd --instance=cgigit cgit"

prefix=/usr/local

SHARE=$prefix/share/house
PUBLICCGI=$SHARE/public/cgi
PUBLICGIT=$SHARE/public/cgigit

USERGIT=$SUDO_USER

for a in $* ; do
   case $a in
      --user=*)
         array=(${a//=/ })
         USERGIT=${array[1]}
         ;;
   esac
done

INSTALL=/usr/bin/install

# This configures the git-http-backend CGI application.
# If you want to run cgit in addition to git-http-backend, you will need to
# build and add cgit manually (using "sudo housecgiadd --instance=cgigit cgit")
# because "apt install cgit" forces install of Apache, a hog of a web server..
# Only systemd is supported here. Support for runit is left as an exercise..

$INSTALL -m 0755 -d $PUBLICGIT
sed 's/\/cgi/\/cgigit/' < $PUBLICCGI/index.html > $PUBLICGIT/index.html
sed 's/\/cgi/\/cgigit/' < $PUBLICCGI/events.html > $PUBLICGIT/events.html
chmod 644 $PUBLICGIT/*

$INSTALL -m 0755 -d /var/lib/house/cgigit-bin
$INSTALL -m 0755 -T $SHARE/cgi/githttp.sh /var/lib/house/cgigit-bin/githttp

if [ -e /lib/systemd/system/housegit.service ] ; then systemctl stop housegit ; fi

cat /lib/systemd/system/housecgi.service | sed 's/\/housecgi /\/housecgi --instance=cgigit /' | sed "s/User=house/User=$USERGIT/" | sed 's/CGI service/CGI service for Git/' > /lib/systemd/system/housegit.service
chown root:root /lib/systemd/system/housegit.service

systemctl daemon-reload
systemctl enable housegit
systemctl start housegit

