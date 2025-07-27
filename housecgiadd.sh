#!/bin/bash
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
# HouseCgiAdd: install one CGI application
#
# SYNOPSYS:
#
#    housecgiadd [--instance=NAME] executable public-file ...
#
CGIINSTANCE=cgi
DEFAULTBINDIR=/var/lib/house/$CGIINSTANCE-bin

if [[ "x$1" == "x--instance="* ]] ; then
   CGIINSTANCE=`cut --delimiter== --fields=2 <<< "x$1"`
   shift
fi

CGIBINDIR=/var/lib/house/$CGIINSTANCE-bin
mkdir -p $CGIBINDIR
chmod a+rx $CGIBINDIR

CGIAPP=$1
CGINAME=`basename $1`
CGIBIN=$CGIBINDIR/$CGINAME
CGIPUBLIC=/usr/local/share/house/public/$CGINAME

rm -f $CGIBIN $DEFAULTBINDIR/$CGINAME
cp $CGIAPP $CGIBIN
chmod a+rx $CGIBIN
shift

for f in $* ; do
   # Do this in the loop, so that the directory is created only when needed.
   mkdir -p $CGIPUBLIC
   chmod a+rx $CGIPUBLIC

   cp $f $CGIPUBLIC
   chmod a+r $CGIPUBLIC/`basename $f`
done

