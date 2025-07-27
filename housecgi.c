/* HouseCGI - A simple home web service to support CGI applications.
 *
 * Copyright 2025, Pascal Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * NOTE: this application requires HousePortal. Otherwise, use Apache.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <math.h>

#include "echttp.h"
#include "echttp_static.h"
#include "echttp_json.h"
#include "echttp_xml.h"
#include "houseportalclient.h"

#include "housediscover.h"
#include "houselog.h"
#include "houselog_sensor.h"

#include "housecgi_route.h"

static int Debug = 0;
static char HostName[256] = {0};

#define DEBUG if (Debug) printf

static const char *housecgi_status (const char *method, const char *uri,
                                    const char *data, int length) {

    static char buffer[65537];
    int cursor;

    cursor = snprintf (buffer, sizeof(buffer),
                       "{\"host\":\"%s\",\"timestamp\":%lld,\"cgi\":{",
                       HostName);

    cursor += housecgi_route_status (buffer+cursor, sizeof(buffer)-cursor);

    snprintf (buffer+cursor, sizeof(buffer)-cursor, "}}");
    echttp_content_type_json ();
    return buffer;
}

static void housecgi_background (int fd, int mode) {

    static time_t LastCall = 0;
    time_t now = time(0);
    time_t yesterday = now - (24 * 3600);

    if (now == LastCall) return;
    LastCall = now;

    housecgi_route_background (now);

    houseportal_background (now);
    housediscover (now);
    houselog_background (now);
    houselog_sensor_background (now);
}

int main (int argc, const char **argv) {

    const char *instance = "cgi";

    // These strange statements are to make sure that fds 0 to 2 are
    // reserved, since this application might output some errors.
    // 3 descriptors are wasted if 0, 1 and 2 are already open. No big deal.
    //
    open ("/dev/null", O_RDONLY);
    dup(open ("/dev/null", O_WRONLY));

    int i;
    const char *value;
    for (i = 1; i < argc; ++i) {
        if (echttp_option_present ("-d", argv[i])) {
            Debug = 1;
        } else if (echttp_option_match ("-instance=", argv[i], &value)) {
            instance = value;
        }
    }
    DEBUG ("Starting as %s.\n", instance);

    gethostname (HostName, sizeof(HostName));

    echttp_default ("-http-service=dynamic");
    argc = echttp_open (argc, argv);

    houseportal_initialize (argc, argv);
    housediscover_initialize (argc, argv);
    houselog_initialize (instance, argc, argv);

    housecgi_route_initialize (instance, argc, argv); // Declare the CGI routes.

    static char uri[128];
    snprintf (uri, sizeof(uri), "/%s/status", instance);
    echttp_route_uri (uri, housecgi_status);
    echttp_static_route ("/", "/usr/local/share/house/public");
    echttp_background (&housecgi_background);
    echttp_loop();
}

