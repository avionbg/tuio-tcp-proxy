/* 
 *  tuio_socket_proxy.c
 * 
 *  Tuio protocol proxying (forwarding from udp to tcp port)
 *
 *  Author : Goran Medakovic
 *  Email : avion.bg@gmail.com
 *
 *  Copyright (c) 2008 Goran Medakovic <avion.bg@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

/* How to compile: gcc -Wall -llo tuio_socket_proxy.c -o tuio_socket_proxy */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lo/lo.h"

int done = 0;
char fwdport[25], *port;
void error(int num, const char *m, const char *path);

int tuioFwd_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int main(int argc, char *argv[])
{
  if (argc == 5 && argv[1][0] == '-' && argv[1][1] == 'p' &&
      argv[3][0] == '-' && argv[3][1] == 't')
    {
      port = (char *) argv[2];
      sprintf(fwdport, "osc.tcp://localhost:%s", argv[4]);
    }
  else
    {
      printf
          ("Usage: %s -p from_port -t to_tcp_port\nexample %s -p 3333 -t 3000\n",
           (char *) argv[0], (char *) argv[0]);
      return 0;
    }
  printf
      ("Starting server on port %s, forwarding to tcp port: %s\nPress CTRL+C to stop\n",
       port, fwdport);
  lo_server_thread st = lo_server_thread_new(port, error);

  lo_server_thread_add_method(st, NULL, NULL, tuioFwd_handler, NULL);
  lo_server_thread_start(st);

  while (!done)
    {
#ifdef WIN32
      Sleep(1);
#else
      usleep(1000);
#endif
    }
  lo_server_thread_free(st);
  return 0;
}

void error(int num, const char *msg, const char *path)
{
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
  fflush(stdout);
}

/* Catch any incoming messages and forward them to specified tcp port. For 
 * now we handle standard touchlib messages (int/float/string) but tuio
 * protocol defines much more of them. */

int tuioFwd_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
  lo_address t = lo_address_new_from_url(fwdport);
  lo_message fwd = lo_message_new();
  int j;

  for (j = 0; j < argc; j++)
    {
      if (types[j] == 115)
        lo_message_add_string(fwd, (char *) argv[j]);
      else if (types[j] == 105)
        lo_message_add_int32(fwd, (int) argv[j]->i);
      else if (types[j] == 102)
        lo_message_add_float(fwd, (float) argv[j]->f);
      else
        printf("Warning: type of message: %d '%c' not handled\n", types[j],
               types[j]);
    }
  if (lo_send_message(t, path, fwd) == -1)
    {
      printf("OSC error %d: %s\n", lo_address_errno(t),
             lo_address_errstr(t));
    }
  lo_message_free(fwd);

  return 0;
}
