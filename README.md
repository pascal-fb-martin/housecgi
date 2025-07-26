# HouseCGI
The House service to interface with Common Gateway Interface (CGI) applications

## Overview

This service allow the integration of a CGI compliant application within the House environment.

This service itself is not intended to have a user interface. Instead it advertises all the installed CGI applications as if these were House services. It also advertises itself (as the "cgi" service) to give access to a (minimal) maintenance and troubleshooting web UI.

## Warnings

HouseCGI is still fairly new. Its goal is to implement the complete CGI RFC, but this is still a work in progress.

There is no access security. This tool is meant to be used on a local, secure, network only.

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

