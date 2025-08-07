#!/bin/bash
export GIT_HTTP_EXPORT_ALL=1
export GIT_PROJECT_ROOT=/space/git
if [ -r /etc/default/house/githhtp ] ; then . /etc/default/house/githhtp ; fi
exec /usr/lib/git-core/git-http-backend

