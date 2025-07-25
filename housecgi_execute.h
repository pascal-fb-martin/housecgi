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
 * housecgi_execute.h - Handle the running CGI applications.
 */

void housecgi_execute_initialize (int argc, const char **argv);
int housecgi_execute_declare (const char *name, const char *uri,
                              const char *path, const char *root);

void housecgi_execute_launch (int id,
                              const char *method, const char *uri,
                              const char *data, int length);
int housecgi_execute_wait (int id, int blocking);
const char *housecgi_execute_output (int id);
int housecgi_execute_max (int id);

void housecgi_execute_background (time_t now);
int housecgi_execute_status (char *buffer, int size);

