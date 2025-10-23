# HouseCGI

The House service to interface with Common Gateway Interface (CGI) applications

## Overview

This service allow the integration of a CGI compliant application within the House environment.

This service itself is not intended to have a user interface. Instead it advertises all the installed CGI applications as if these were House services. It also advertises itself (as the "cgi" service) to give access to a (minimal) maintenance and troubleshooting web UI.

> [!WARNING]
> HouseCGI is still fairly new. Its goal is to implement the complete CGI RFC, but this is still a work in progress.
>
>There is no access security. This tool is meant to be used on a local, secure, network only.

## Installation

This service depends on the House series environment:

* Install git, icoutils, openssl (libssl-dev).
* Install [echttp](https://github.com/pascal-fb-martin/echttp)
* Install [houseportal](https://github.com/pascal-fb-martin/houseportal)
* It is recommended to install [housesaga](https://github.com/pascal-fb-martin/housesaga) somewhere on the local network, preferably on a file server (logs may become large, and constant write access might not be good for SD cards).
* Clone this repository.
* make rebuild
* sudo make install

## CGI Application installation

It is possible to install or remove CGI applications while the HouseCGI service is running. A CGI application is defined by the following two resources:

* /var/lib/house/cgi-bin/<name>: this is the application's executable file
* /usr/local/share/house/public/<name>: this is where the application's publicly accessible files must be installed.

The URI prefix for the CGI application is `/<name>/cgi`.

Two helpers are provided:

* `housecgiadd` installs a new CGI application. The first parameter must be the application's executable file, the subsequent parameters are the publicly accessible files. The base name of the executable file becomes the name of the CGI application.

* `housecgiremove` uninstalls a list of CGI applications, identified by their names.

## HouseCGI and Git

This CGI support was originally intended to run cgit and git-hhtp-backend, but there are some twists as Git is picky about ownership. This makes the installation of these applications somewhat tricky. A special script `housecgigit` eases the pain, but there are still additional steps required.

To install a git-specific instance of HouseCGI, use the following command _as the owner of the git repositories_:

```
sudo housecgigit
```

This configures and starts a new service that _runs under your current account_ (to solve the ownership issue). The git-http-backend CGI application is automatically added to this instance (this is part of the standard git installation).

> [!WARNING]
> Only systemd based systems are supported at this time.

> [!NOTE]
> The default configuration for git-http-backend is to look for git repositories in `/space/git`. If your repositories location is different, create file `/etc/house/githttp` (a shell script) and set the environment variable `GIT_PROJECT_ROOT` to match your actual location.

> [!NOTE]
> If the Git repositories to share using HTTP are owned by a special account different from the user account, you can use the command `sudo housecgigit --user=NAME`.

The cgit application should then be built from source.

> [!NOTE]
> It is not recommended to install cgit using apt because Debian decided that it required Apache, which is not used here and conflicts with HousePortal on port 80. An alternative could be to install cgit using apt, and then permanently disable the Apache service.
>

Building cgit require editing a `cgit.conf` file. I found that the following configuration is working for me:

```
CGIT_SCRIPT_NAME = cgit
CGIT_SCRIPT_PATH = /var/lib/house/cgigit-bin
CGIT_DATA_PATH = /usr/local/share/house/public/cgit
```

This configuration automatically installs cgit under the proper instance of HouseCGI when running `make install`, no need to run `housecgiadd`.

The cgit installation also requires editing a `/etc/cgitrc` file. One can extract the example from file `cgitrc.5.txt` and modify the following items:

```
css=/cgit/cgit.css
js=/cgit/cgit.js
favicon=/cgit/favicon.ico
logo=/cgit/cgit.png
source-filter=/usr/local/lib/cgit/filters/syntax-highlighting.py
about-filter=/usr/local/lib/cgit/filters/about-formatting.sh
```

> [!NOTE]
> Disabling cloning is recommended (`enable-http-clone=0`): for now the old git HTTP protocol implementation in cgit does not play nice with HouseCGI, while git-http-backend works.

Do not forget to set `scan-path` to your specific repositories location, or else to list each one of your repositories. You can also set your own title (`root-title`) and subheading (`root-desc`).

You can also provide your own "about" information for the repositories index page (see `root-readme`). The simplest method is to install the `about.html` file in `/usr/local/share/house/public/cgit` and edit `/etc/gitrc` to point to it:

```
root-readme=/usr/local/share/house/public/cgit/about.html
```

Once everything has been configured and the git-http-backend service is running, repositories can be cloned using the URL below:

```
http://<your server>/githttp/cgi/<your repository>
```

When the cgit service is running, the Git web UI is accessible at the URL below:

```
http://<your server>/cgit/cgi/<your repository>
```

## Debian Packaging 

The provided Makefile supports building private Debian packages. These are _not_ official packages:

- They do not follow all Debian policies.

- They are not built using Debian standard conventions and tools.

- The packaging is not separate from the upstream sources, and there is
  no source package.

To build a Debian package, use the `debian-package` target:

```
make debian-package
```

