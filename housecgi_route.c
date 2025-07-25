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
 * housecgi_route.c - Manage the routes to all CGI applications.
 *
 * This module detect all installed CGI applications and register an URL
 * for each one.
 *
 * void housecgi_route_initialize (int argc, const char **argv);
 *
 *    Initialize this module.
 *
 * void housecgi_route_background (time_t now);
 *
 *    Search the cgi-bin directory for any executable. Each executable
 *    is then registered with an URI based on the file name.
 *
 *    This function should be called periodically to detect when an
 *    application was removed or added. It handle the HTPP routes.
 *
 * int housecgi_route_status (char *buffer, int size);
 *
 *    Return the current status of this module in JSON format.
 *
 * NOTE
 *
 *    This application requires HousePortal. Otherwise, just use Apache.
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>

#include "echttp.h"
#include "houseportalclient.h"
#include "houselog.h"

#include "housecgi_route.h"
#include "housecgi_execute.h"

static int Debug = 0;

#define DEBUG if (Debug) printf

typedef struct {
    char *name;
    char *uri;
    size_t urilength;
    char *fullpath;
    int executor;
    time_t started;
    char present;
} CgiApplication;

static CgiApplication *CgiDirectory = 0;
static int CgiDirectoryCount = 0;
static int CgiDirectorySize = 0;

static const char *CgiBinRoot = "/var/lib/house/cgi-bin";

static int CgiPollPeriod = 60;

static int housecgi_route_new (void) {

    // First try to reuse a discarded slot.
    int i;
    for (i = 0; i < CgiDirectoryCount; ++i) {
        if (!CgiDirectory[i].uri) return i;
    }
    if (CgiDirectoryCount >= CgiDirectorySize) {
        CgiDirectorySize += 8;
        CgiDirectory = realloc (CgiDirectory, sizeof(CgiApplication) * CgiDirectorySize);
    }
    return CgiDirectoryCount++;
}

static char *housecgi_route_format (const char *format, const char *name) {
    char formatted[512];
    snprintf (formatted, sizeof(formatted), format, name);
    return strdup (formatted);
}

static char *housecgi_route_uri (const char *name) {
    return housecgi_route_format ("/%s/cgi", name);
}

static char *housecgi_route_registration (const char *name) {
    return housecgi_route_format ("cgi:/%s", name);
}

void housecgi_route_initialize (int argc, const char **argv) {

    int i;
    const char *value;
    for (i = 1; i < argc; ++i) {
        if (echttp_option_match ("-cgi-poll=", argv[i], &value)) {
            CgiPollPeriod = atoi (value);
            if (CgiPollPeriod < 10) CgiPollPeriod = 10; // Self protect.
        } else if (echttp_option_match ("-cgi-bin=", argv[i], &value)) {
            CgiBinRoot = value;
        } else if (echttp_option_present ("-d", argv[i])) {
            Debug = 1;
        }
    }
    housecgi_execute_initialize (argc, argv);

    // Initial CGI applications discovery.
    housecgi_route_background (time(0));
}

static const char *housecgi_route_error (const char *uri,
                                         int code, const char *text) {
    static char message[1024];

    echttp_error (code, text);

    snprintf (message, sizeof(message),
              "<html><body>Sorry, your request failed.<br>%s: %s</body></html>",
              uri, text);
    echttp_content_type_html ();
    return message;
}

static const char *housecgi_route_handle (const char *method, const char *uri,
                                          const char *data, int length) {
    int i;
    for (i = 0; i < CgiDirectoryCount; ++i) {
        if (!CgiDirectory[i].uri) continue;
        if (strncmp (uri, CgiDirectory[i].uri, CgiDirectory[i].urilength)) continue;
        if ((uri[CgiDirectory[i].urilength] != 0) &&
            (uri[CgiDirectory[i].urilength] != '/')) continue;

        // Warning: the CGI child is executed in blocking mode.
        housecgi_execute_launch
            (CgiDirectory[i].executor, method, uri, data, length);

        while (! housecgi_execute_wait (CgiDirectory[i].executor, 1)) ;

        const char *output = housecgi_execute_output (CgiDirectory[i].executor);
        if (output) return output;
        return "";
    }
    return housecgi_route_error (uri, 503, "No such CGI service");
}

void housecgi_route_background (time_t now) {

    static char **CgiRegistration = 0;
    static int CgiRegistrationCount = 0;

    static time_t LastCall = 0;

    int firstCall = 0;
    int changed = 0;

    if (now < LastCall + CgiPollPeriod) return;
    if (!LastCall) firstCall = 1;
    LastCall = now;

    int i;
    int j;
    struct dirent **files = 0;
    char fullpath[512];
    char webroot[512];

    for (j = 0; j < CgiDirectoryCount; ++j) {
        CgiDirectory[j].present = 0;
    }

    int n = scandir (CgiBinRoot, &files, 0, 0);
    for (i = 0; i < n; i++) {
        struct dirent *ent = files[i];
        if (ent->d_name[0] == '.') continue; // Skip hidden entries.
        if (ent->d_type != DT_REG) continue;
        snprintf (fullpath, sizeof(fullpath), "%s/%s", CgiBinRoot, ent->d_name);

        struct stat filestat;
        if (stat (fullpath, &filestat)) continue; // No access.
        if (!(filestat.st_mode & S_IXOTH)) continue; // Not executable.

        char canonical[512];
        snprintf (canonical, sizeof(canonical), "%s", ent->d_name);
        char *ext = strrchr (canonical, '.');
        if (ext) *ext = 0;

        for (j = 0; j < CgiDirectoryCount; ++j) {
            if (!CgiDirectory[j].name) continue;
            if (!strcmp (canonical, CgiDirectory[j].name)) {
                CgiDirectory[j].present = 1;
                break;
            }
        }
        if (j >= CgiDirectoryCount) { // New CGI application.
            j = housecgi_route_new ();
            CgiDirectory[j].present = 1;
            CgiDirectory[j].name = strdup (canonical);
            CgiDirectory[j].fullpath = strdup (fullpath);
            CgiDirectory[j].uri = housecgi_route_uri (canonical);
            CgiDirectory[j].urilength = strlen (CgiDirectory[j].uri);
            CgiDirectory[j].started = time(0);
            echttp_route_match (CgiDirectory[j].uri, housecgi_route_handle);
            snprintf (webroot, sizeof(webroot),
                      "/usr/local/share/house/public/%s", canonical);
            CgiDirectory[j].executor =
                housecgi_execute_declare (CgiDirectory[j].name,
                                          CgiDirectory[j].uri,
                                          CgiDirectory[j].fullpath, webroot);
            if (!firstCall) {
                houselog_event ("CGI", CgiDirectory[j].name, "ACTIVATED",
                                "EXECUTABLE %s", CgiDirectory[j].fullpath);
            }
            changed = 1;
        }
        free (ent);
    }
    if (files) free (files);

    // Eliminate those entries when the application is no longer present.
    for (j = 0; j < CgiDirectoryCount; ++j) {
        if (!CgiDirectory[j].name) continue; // Already removed.
        if (CgiDirectory[j].present) continue;
        houselog_event ("CGI", CgiDirectory[j].name, "REMOVED",
                        "EXECUTABLE %s", CgiDirectory[j].fullpath);
        echttp_route_remove (CgiDirectory[j].uri);
        free (CgiDirectory[j].name);
        CgiDirectory[j].name = 0;
        free (CgiDirectory[j].uri);
        CgiDirectory[j].uri = 0;
        free (CgiDirectory[j].fullpath);
        CgiDirectory[j].fullpath = 0;
        changed = 1;
    }

    if ((!firstCall) && (!changed)) return; // Nothing more to do.

    // Re-register the new list to HousePortal and update the echttp
    // route list if necessary..
    //
    static const char *path[] = {"cgi:/cgi"};
    houseportal_declare (echttp_port(4), path, 1);

    for (j = 0; j < CgiRegistrationCount; ++j) {
        free (CgiRegistration[j]);
    }
    if (CgiRegistration) free (CgiRegistration);
    if (CgiDirectoryCount <= 0) return; // Nothing to declare.

    CgiRegistration = malloc (sizeof(char *) * CgiDirectoryCount);
    CgiRegistrationCount = 0;
    for (j = 0; j < CgiDirectoryCount; ++j) {
        if (CgiDirectory[j].present) {
            CgiRegistration[CgiRegistrationCount++] =
                housecgi_route_registration (CgiDirectory[j].name);
        }
    }
    if (CgiRegistrationCount > 0) {
        houseportal_declare_more (echttp_port(4),
                                  (const char **)CgiRegistration,
                                  CgiRegistrationCount);
    } else {
        free (CgiRegistration);
        CgiRegistration = 0;
    }
}

int housecgi_route_status (char *buffer, int size) {

    const char *sep = "[";
    int cursor = snprintf (buffer, size, "\"routes\":");
    if (cursor >= size) return 0;

    int i;
    for (i = 0; i < CgiDirectoryCount; ++i) {
        if (!CgiDirectory[i].present) continue;
        cursor += snprintf (buffer+cursor, size-cursor,
                            "%s{\"service\":\"%s\",\"uri\":\"%s\""
                                ",\"path\":\"%s\",\"start\":%lld,\"max\":%d}",
                            sep, CgiDirectory[i].name, CgiDirectory[i].uri,
                            CgiDirectory[i].fullpath,
                            (long long)CgiDirectory[i].started,
                            housecgi_execute_max(CgiDirectory[i].executor));
        if (cursor >= size) return 0;
        sep = ",";
    }

    cursor += snprintf (buffer+cursor, size-cursor, "]");
    if (cursor >= size) return 0;
    return cursor;
}

