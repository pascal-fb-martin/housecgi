#!/bin/bash
export GIT_HTTP_EXPORT_ALL=1
export GIT_PROJECT_ROOT=/space/git
exec /usr/lib/git-core/git-http-backend

