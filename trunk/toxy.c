/*
 *
 *  Tuio protocol proxying (forwarding from udp to server on tcp port)
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <lo/lo.h>

#include "libtoxy.h"

int main(int argc, char *argv[])
{
    if (argc == 5 && argv[1][0] == '-' && argv[1][1] == 'p' &&
            argv[3][0] == '-' && argv[3][1] == 't')
    {
        port = (char *) argv[2];
        fwdport = atoi ((char *) argv[4]);
    }
    else if (argc == 1)
    {
        port = "3333";
        fwdport = 3000;
    }
    else if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'd')
    {
        port = "3333";
        fwdport = 3000;
        debug =1;
    }
    else
    {
        printf
        ("Usage: %s -p from_port -t to_tcp_port\n\nExample:\n %s -p 3333 -t 3000\n", (char *) argv[0], (char *) argv[0]);
        exit (1);
    }
    printf ("Starting server on port %s, forwarding to tcp port: %d\nPress CTRL+C to stop\n", port , fwdport);
    int lo_fd;
    int retval;
    fd_set rfds;
    initcp();
    initlo();
    lo_fd = lo_server_get_socket_fd(st);

    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    if (lo_fd > 0) {

    do
    {
            FD_ZERO(&rfds);
//#ifndef WIN32
            //FD_SET(0, &rfds);  /* stdin */
//#endif
            FD_SET(lo_fd, &rfds);
//printf("select()\n");
            retval = select(lo_fd + 1, &rfds, NULL, NULL, NULL); /* no timeout */
//                    printf ("stdin\n");

            if (retval == -1) {

                printf("select() error\n");
                exit(1);

            } else if (retval > 0) {

                //if (FD_ISSET(0, &rfds)) {

                    //read_stdin();
                    printf ("stdin\n");

                //}
                if (FD_ISSET(lo_fd, &rfds)) {

                    lo_server_recv_noblock(st, 0);

                }
            }
    }while (!done);
}
    closesocket(clientsock);
    closesocket(servsock);
    //lo_server_thread_free(st);
    exit (0);
}
