/* 
 *  tuio_dump.c
 * 
 *  dumping forwarded messages on tcp port
 *  writen to check tuio_socket_proxy
 * 
 *  Author : Goran Medakovic
 *  Email : avion.bg@gmail.com
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lo/lo.h"

int done = 0;

void error(int num, const char *m, const char *path);

int dump_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int main()
{
  lo_server_thread st =
      lo_server_thread_new_with_proto("3000", LO_TCP, error);

  lo_server_thread_add_method(st, NULL, NULL, dump_handler, NULL);

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

int dump_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, void *data, void *user_data)
{
  int i;

  printf("path: <%s>\n", path);
  for (i = 0; i < argc; i++)
    {
      printf("arg %d '%c' ", i, types[i]);
      lo_arg_pp(types[i], argv[i]);
      printf("\n");
    }
  fflush(stdout);

  return 0;
}
