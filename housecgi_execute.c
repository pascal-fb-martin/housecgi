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
 * housecgi_execute.c - Handle the running CGI applications.
 *
 * This module manages all CGI subprocesses.
 *
 * void housecgi_execute_initialize (int argc, const char **argv);
 *
 *    Initialize this module.
 *
 * int housecgi_execute_declare (const char *name, const char *uri,
 *                               const char *path, const char *root);
 *
 *    Register a new CGI application. This declares once the parameters
 *    that do not change from one launch to another.
 *
 *    This function returns an ID that can be used when running the CGI
 *    application.
 *
 * void housecgi_execute_launch (int id,
 *                               const char *method, const char *uri,
 *                               const char *data, int length);
 *
 *    Launch the specified CGI program. The rest of the context is retrieved
 *    from the echttp's current client context, exactly as in a standard
 *    echttp endpoint function.
 *
 * int housecgi_execute_wait (int id, int blocking);
 *
 *    wait (blocking or non blocking) for CGI output and/or termination.
 *    Returns 1 if the CGI process did exit, 0 otherwise.
 *
 * const char *housecgi_execute_output (int id);
 *
 *    Return the output of a (recently deceased) CGI. The returned pointer
 *    might be null.
 *
 *    The data returned is the content of the CGI response. Any header part
 *    provided by the CGI application has been decoded and set as HTTP
 *    attributes for this request, including the content type.
 *
 * void housecgi_execute_background (time_t now);
 *
 *    Monitor the running CGI subprocesses.
 *
 *    This function should be called periodically to detect when a
 *    subprocessed terminated.
 *
 * int housecgi_execute_status (char *buffer, int size);
 *
 *    Return the current status of this module in JSON format.
 *
 * NOTE
 *
 *    This application requires HousePortal. Otherwise, just use Apache.
 *
 * RESTRICTIONS:
 *
 *    For the time being, the CGI application is executed in synchronous mode.
 *    This means only one CGI call at one time. Elements of the design
 *    are meant for an asynchronous model, but some portions are missing:
 *    - the endpoint needs to be configured as asynchronous, then initiate
 *      a transfer to the pipe. This is supported by the echttp library.
 *    - The echttp library does not really support waiting for the CGI output
 *      yet: the data final length is not known until the pipe closes,
 *      preventing echttp from generating all the HTTP attributes. Would hate
 *      to use HTTP fragments..
 *      - A pipe is not compatible with sendfile(). Use splice() in that case?
 */

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/wait.h>

#include "echttp.h"
#include "echttp_hash.h"
#include "houseportalclient.h"
#include "houselog.h"

#include "housecgi_route.h"
#include "housecgi_execute.h"

static int Debug = 0;

#define DEBUG if (Debug) printf

typedef struct {
    char *name;
    long long signature;
    char *uri;
    char *executable;
    char *root;
    pid_t running;
    int   write;
    int   read;
    char  out[0x100000];
    int   outlen;
} CgiChild;

static CgiChild *CgiChildren = 0;

static int CgiChildrenCount = 0;
static int CgiChildrenSize = 0;

static char HostName[128] = {0};

static int housecgi_execute_search (const char *name) {

    long long signature = echttp_hash_signature (name);
    int i;
    for (i = 0; i < CgiChildrenCount; ++i) {
        if (CgiChildren[i].signature != signature) continue;
        if (strcmp (CgiChildren[i].name, name)) continue;
        return i; // Matches.
    }
    return -1;
}

static void housecgi_execute_env (int i, const char *method, const char *uri) {

    // Set the CGI environment variables:
    // AUTH_TYPE (not supported)
    // CONTENT_LENGTH (Content-Length attribute).
    // CONTENT_TYPE (Content-Type attribute)
    // GATEWAY_INTERFACE (CGI/1.1)
    // HTTP_COOKIE (Cookies attribute)
    // HTTP_HOST (host name)
    // HTTP_REFERER (Referer attribute)
    // HTTP_USER_AGENT (User-Agent attribute)
    // PATH_INFO (resource or subresource requested, based on uri)
    // PATH_TRANSLATED (full path for the PATH_INFO resource, based on root)
    // QUERY_STRING (HTTP parameters)
    // REDIRECT_STATUS (200, for now)
    // REMOTE_ADDR (not supported, for now)
    // REMOTE_HOST (not supported)
    // REQUEST_METHOD (GET, HEAD, POST, etc)
    // SCRIPT_NAME (the URI that identifies this CGI script)
    // SERVER_NAME (host name)
    // SERVER_PORT (80 for now)
    // SERVER_PROTOCOL (HTTP for now)
    // SERVER_SOFTWARE ("house/0.1" for now)
    // 
    const char *attribute;

    attribute = echttp_attribute_get ("Content-Length");
    if (attribute) setenv ("CONTENT_LENGTH", attribute, 1);
    attribute = echttp_attribute_get ("Content-Type");
    if (attribute) setenv ("CONTENT_TYPE", attribute, 1);

    setenv ("GATEWAY_INTERFACE", "CGI/1.1", 1);

    attribute = echttp_attribute_get ("Cookie");
    if (attribute) setenv ("HTTP_COOKIE", attribute, 1);
    attribute = echttp_attribute_get ("Referer");
    if (attribute) setenv ("HTTP_REFERER", attribute, 1);
    attribute = echttp_attribute_get ("User-Agent");
    if (attribute) setenv ("HTTP_USER_AGENT", attribute, 1);

    char query[1024];
    echttp_parameter_join (query, sizeof(query));
    setenv ("QUERY_STRING", query, 1);

    if (!HostName[0]) gethostname (HostName, sizeof(HostName)-1);
    setenv ("HTTP_HOST", HostName, 1);
    setenv ("SERVER_NAME", HostName, 1);

    setenv ("REDIRECT_STATUS", "200", 1); // For now..

    setenv ("REQUEST_METHOD", method, 1);

    const char *path_info = uri + strlen(CgiChildren[i].uri);
    setenv ("PATH_INFO", path_info, 1);

    char translated[1024];
    snprintf (translated, sizeof(translated),
              "%s%s", CgiChildren[i].root, path_info);
    setenv ("PATH_TRANSLATED", translated, 1);

    setenv ("SCRIPT_NAME", CgiChildren[i].uri, 1);

    setenv ("SERVER_PORT", "80", 1);
    setenv ("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv ("SERVER_SOFTWARE", "housecgi/0.1", 1); // For now
}

static pid_t housecgi_execute_fork (int i) {

    int read_pipe[2];
    int write_pipe[2];
    pid_t child;

    if (pipe (read_pipe) < 0) return -1;
    if (pipe (write_pipe) < 0) return -1;

    child = fork();
    if (child == 0) {
        // This is the child process.
        dup2 (write_pipe[0], 0);
        dup2 (read_pipe[1], 1);
        chdir (CgiChildren[i].root);
    } else {
        // This is the parent process.
        CgiChildren[i].running = child;
        CgiChildren[i].read = read_pipe[0];
        CgiChildren[i].write = write_pipe[1];
        CgiChildren[i].outlen = 0;
    }
    return child;
}

static void housecgi_execute_listen (int i, int blocking) {

    // FIXME: should declare an i/o handler with the echttp event loop.

    if (CgiChildren[i].running <= 0) return;

    if (CgiChildren[i].read > 0) {
        fd_set reads;
        struct timeval timeout;

        FD_ZERO(&reads);
        FD_SET (CgiChildren[i].read, &reads);

        timeout.tv_usec = 0;
        timeout.tv_sec = blocking ? 1 : 0;

        int result = select (CgiChildren[i].read+1, &reads, 0, 0, &timeout);
        if ((result > 0) && FD_ISSET(CgiChildren[i].read, &reads)) {
            char *buffer = CgiChildren[i].out + CgiChildren[i].outlen;
            int space = sizeof(CgiChildren[i].out) - CgiChildren[i].outlen - 1;
            int length = read (CgiChildren[i].read, buffer, space);
            if (length > 0) {
               CgiChildren[i].outlen += length;
            }
        }
    }
}

static int housecgi_execute_deceased (int i, int blocking) {

    if (CgiChildren[i].running <= 0) return 1;

    int options = blocking ? 0 : WNOHANG;
    pid_t pid = waitpid (CgiChildren[i].running, 0, options);
    if (pid == CgiChildren[i].running) {
        CgiChildren[i].running = 0;
        if (CgiChildren[i].read > 0) close (CgiChildren[i].read);
        if (CgiChildren[i].write > 0) close (CgiChildren[i].write);
        CgiChildren[i].read = CgiChildren[i].write = -1;
        return 1;
    }
    return 0;
}

void housecgi_execute_initialize (int argc, const char **argv) {
}

int housecgi_execute_declare (const char *name, const char *uri,
                              const char *path, const char *root) {

    int i = housecgi_execute_search (name);
   
    if (i < 0) {
        // We did not find this CGI program. Create a new context.
        if (CgiChildrenCount >= CgiChildrenSize) {
            CgiChildrenSize += 16;
            CgiChildren = realloc (CgiChildren,
                                   CgiChildrenSize*sizeof(CgiChild));
        }
        i = CgiChildrenCount++;
        CgiChildren[i].name = strdup(name);
        CgiChildren[i].signature = echttp_hash_signature (name);
    } else {
        // update an existing entry.
        if (CgiChildren[i].executable) free (CgiChildren[i].executable);
        if (CgiChildren[i].uri) free (CgiChildren[i].uri);
        if (CgiChildren[i].root) free (CgiChildren[i].root);
    }
    CgiChildren[i].executable = strdup (path);
    CgiChildren[i].uri = strdup (uri);
    CgiChildren[i].root = strdup (root);

    return i;
}

void housecgi_execute_launch (int id,
                              const char *method, const char *uri,
                              const char *data, int length) {

    if ((id < 0) || (id >= CgiChildrenCount)) return; // Invalid CGI?

    if (CgiChildren[id].running > 0) {
        // FIXME: queue the new request?
        while (!housecgi_execute_deceased (id, 0)) {
            housecgi_execute_listen (id, 0);
        }
    }

    pid_t child = housecgi_execute_fork (id);
    if (child < 0) return; // Failure.

    if (child == 0) {
        // This is the child process.
        housecgi_execute_env (id, method, uri);
        execlp (CgiChildren[id].executable, CgiChildren[id].name, (char *)0);
        // This should never return: failed to launch the CGI executable.
        exit(1);
    }

    if (length > 0)
        write (CgiChildren[id].write, data, length); // FIXME: blocking!!
}

int housecgi_execute_wait (int id, int blocking) {

    if ((id < 0) || (id >= CgiChildrenCount)) return 0; // Invalid CGI?

    housecgi_execute_listen (id, blocking);
    return housecgi_execute_deceased (id, blocking);
}

static char *housecgi_execute_split (char *line) {

   char *cursor = line;
   while ((*cursor > 0) && (*cursor != ':')) ++cursor;
   if (*cursor == 0) return 0;
   char *unpad = cursor - 1;
   while ((unpad > line) && (*unpad <= ' ')) *(unpad--) = 0; // No trailing space.
   *(cursor++) = 0;
   while ((*cursor > 0) && (*cursor <= ' ')) ++cursor; // Skip leading spaces.
   return cursor;
}

static const char *housecgi_execute_error (int code, const char *text) {

    static char message[1024];

    snprintf (message, sizeof(message),
              "<html><body>Sorry, your request failed: %s</body></html>", text);
    echttp_content_type_html ();
    echttp_error (code, text);
    return message;
}

const char *housecgi_execute_output (int id) {

    if ((id < 0) || (id >= CgiChildrenCount)) // Invalid CGI?
        return housecgi_execute_error (503, "No such CGI service");

    if (CgiChildren[id].running > 0) return 0; // Not complete yet.

    if (CgiChildren[id].outlen <= 0)
        return housecgi_execute_error (500, "No CGI output");

    // Extract the header attributes.
    // Accept the following EOL sequences only: CR LF, LF. (Sorry, Apple.)
    int i;
    int length = CgiChildren[id].outlen;
    char *output = CgiChildren[id].out;
    char *line = output;
    output[length] = 0; // Null terminated.
    for (i = 1; i < length; ++i) {
        output += 1;
        if (*output == '\r') *output = 0; // Exterminate!
        else if (*output == '\n') {
            // Blank line = end of header.
            if ((*line == 0) || (*line == '\n')) break;
            *output = 0;
            char *value = housecgi_execute_split (line);
            echttp_attribute_set (line, value);
            line = output + 1;
        }
    }
    return output + 1; // Skip the 2nd new line.
}

void housecgi_execute_background (time_t now) {
    int i;
    for (i = 0; i < CgiChildrenCount; ++i) {
        if (CgiChildren[i].running > 0) {
            housecgi_execute_listen (i, 0);
            housecgi_execute_deceased (i, 0);
        }
    }
}

int housecgi_execute_status (char *buffer, int size) {
    int i;
    for (i = 0; i < CgiChildrenCount; ++i) {
        if (CgiChildren[i].running > 0) {
        }
    }
}

