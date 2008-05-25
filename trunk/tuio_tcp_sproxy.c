/*
 *  tuio_socket_proxy.c
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

/* How to compile:
 * Linux:
 * gcc tuio_tcp_sproxy.c -o tuio_tcp_sproxy -Wall `pkg-config --cflags --libs liblo`
 * Windows:
 * i686-mingw32-gcc tuio_tcp_sproxy.c -o tuio_tcp_sproxy.exe \
 *  -llo -lws2_32 -lpthread -L/path/to/libs -I/path/to/includes
 */

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#elif linux
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "lo/lo.h"

#define	QLEN 1
struct	protoent *ptrp;
struct	sockaddr_in sad;
struct	sockaddr_in cad;
int	sd, sd2;
int done = 0;

void error(int num, const char *m, const char *path);
int fwd_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data);
int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data);
void sigint_handler(int sig);

#ifdef WIN32
static void
set_errno(int winsock_err)
{
    switch (winsock_err)
    {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

int mingw_setnonblocking (SOCKET fd, int nonblocking)
{
    int rc;

    unsigned long mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &mode);
    if (rc != 0)
    {
        set_errno(WSAGetLastError());
    }
    return (rc == 0 ? 0 : -1);
}
#endif

int main(int argc, char *argv[])
{
    char *port;
    int fwdport;
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
    else
    {
        printf
        ("Usage: %s -p from_port -t to_tcp_port\n\nExample:\n %s -p 3333 -t 3000\n", (char *) argv[0], (char *) argv[0]);
        exit (1);
    }
    printf ("Starting server on port %s, forwarding to tcp port: %d\nPress CTRL+C to stop\n", port    , fwdport);

#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
#endif
    char *opt;
    memset((char *)&sad,0,sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = INADDR_ANY;
    sad.sin_port = htons((u_short)fwdport);

    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
    {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,(void *)&opt, sizeof(opt));
#ifdef WIN32
    mingw_setnonblocking(sd, 1);
#elif linux
    fcntl(sd, F_SETFL, O_NONBLOCK);
#endif
    if (sd < 0)
    {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }
    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0)
    {
        fprintf(stderr,"bind failed\n");
        exit(1);
    }
    if (listen(sd, QLEN) < 0)
    {
        fprintf(stderr,"listen failed\n");
        exit(1);
    }

    lo_server_thread st = lo_server_thread_new(port, error);
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);
    lo_server_thread_add_method(st, NULL, NULL, fwd_handler, NULL);
    lo_server_thread_start(st);
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    while (!done)
    {
#ifdef WIN32
        Sleep(1);
#elif linux
        usleep(1000);
#endif
    }
    closesocket(sd2);
    closesocket(sd);
    lo_server_thread_free(st);
    exit (0);
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int fwd_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user)
{
    char buf[1000];
    int j;
    int alen;
    char tmp[100];
    strcpy (buf, path);
    for (j = 0; j < argc; j++)
    {
        if (types[j] == 115)
        {
            strcat (buf, ",");
            strcat (buf,(char *) argv[j]);
        }
        else if (types[j] == 105)
        {
            sprintf (tmp,",%d", (int) argv[j]->i);
            strcat (buf, tmp);
        }
        else if (types[j] == 102)
        {
            sprintf (tmp, ",%f", (float) argv[j]->f);
            strcat (buf, tmp);
        }
        else
            printf("Warning: type of message: %d '%c' not handled\n", types[j],
                   types[j]);
    }
    alen = sizeof(cad);
    if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) > 0)
    {
        send(sd2,buf,strlen(buf),0);
        closesocket(sd2);
    }
    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data)
{
    done = 1;
    printf("Quiting\n\n");
    fflush(stdout);
    return 0;
}

void sigint_handler(int sig)
{
    printf("Quiting!\n");
    done = 1;
}
