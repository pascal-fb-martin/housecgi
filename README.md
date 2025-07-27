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

## HouseCgi and Git

This CGI support was originally intended to run cgit and git-hhtp-backend, but there are some twists as Git is picky about ownership. This makes the installation of these applications somewhat tricky. A special make target `install-git` eases the pain.

To install a git-specific instance of HouseCgi, use the following command:
```
sudo make USERNAME=`whoami` install-git
```
This configures and starts a new service that _runs under your current account_ (to solve the ownership issue). The git-http-backend CGI application is automatically added to this instance (this is a standard part of a git installation). The cgit application must be built from source and then installed using the command:
```
sudo housecgiadd --instance=cgigit cgit
```
> [!NOTE]
> cgit is not installed using apt because Debian decided that it required Apache, which is not used here and conflicts with HousePortal on port 80.

