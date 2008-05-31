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
#define SHUT_RDWR SD_BOTH
#elif linux
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> //*for TCP_NODELAY
#include <netdb.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "lo/lo.h"

#define	QLEN 5
struct protoent *ptrp;
struct sockaddr_in address;
int servsock, clientsock, fwdport;
int done = 0;
int debug = 0;
int toggle = 1;
char *port;
lo_server st;

typedef struct
{
    char path[12];
    char name[8];
    int sessionID;
    float x,y,X,Y,m,w,h;
} set;

typedef struct
{
    char path[12];
    char name[8];
    int framenum;
} fseq;

void error(int num, const char *m, const char *path);
int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data);
int fwd_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data);
void sigint_handler(int sig);
void initlo();
void initcp();

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

void initcp()
{
#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
#endif
    char *opt;
    memset((char *)&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short)fwdport);

    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
    {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }
    servsock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
#ifdef WIN32
    setsockopt(servsock,SOL_SOCKET,SO_REUSEADDR,(void *)&opt, sizeof(opt));
    mingw_setnonblocking(servsock, 1);
#elif linux
    int size = 1024;
    if (-1 == setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR | TCP_NODELAY | SO_SNDBUF,
        &size, sizeof(size)))
        perror("setsockopt");
    fcntl(servsock, F_SETFL, O_NONBLOCK);
    signal(SIGPIPE,SIG_IGN);
#endif
    if (servsock < 0)
    {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }
    if (bind(servsock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        fprintf(stderr,"bind failed\n");
        exit(1);
    }
    if (listen(servsock, QLEN) < 0)
    {
        fprintf(stderr,"listen failed\n");
        exit(1);
    }

}
void stoptcp()
{
    int err;//,status;
    if (servsock)
    {
    err = shutdown (servsock, SHUT_RDWR);
    //printf("Error: %d",err);
    fflush(stdout);
    if (err <0) perror("Error shutting down serversock\n");
    //closesocket(clientsock);
    closesocket(servsock);
    }
/*    if (clientsock)
    {
    err = shutdown (clientsock, SHUT_RDWR);
    if (err <0) perror("Error shutting down clientsock\n");
    //closesocket(clientsock);
    closesocket(clientsock);
    }*/
    if ( err == 0 )
    {
        printf ("restarting tcp\n");
        fflush(stdout);
        //lo_server_thread_free(st);
#ifdef WIN32
        Sleep(100);
#elif linux
        usleep(100000);
#endif
        initcp();
        //initlo();
    }
}

void initlo()
{
    st = lo_server_new(port, error);
    lo_server_add_method(st, "/quit", "", quit_handler, NULL);
    lo_server_add_method(st, NULL, NULL, fwd_handler, NULL);
    //lo_server_thread_start(st);
}

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
printf("select()\n");
            retval = select(lo_fd + 1, &rfds, NULL, NULL, NULL); /* no timeout */

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
void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int fwd_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user)
{
    int alen,j;
    alen = sizeof(address);

    if ( !clientsock || clientsock < 0 )
        {
        clientsock=accept(servsock, (struct sockaddr *)&address, &alen);
        /*if (clientsock <0)
            if (errno == EAGAIN)
                printf("EAGAIN\n");*/
        //perror("error pipe?? \n");//EAGAIN
        }
    if ( !strcmp((char *) argv[0],"set") )
    {
        set *msgset;
        int status;
        msgset = calloc(1, sizeof(set));
        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->sessionID = argv[1]->i;
        msgset->x = argv[2]->f;
        msgset->y = argv[3]->f;
        msgset->X = argv[4]->f;
        msgset->Y = argv[5]->f;
        msgset->m = argv[6]->f;
        if (argc > 7)
        {
            msgset->w = argv[7]->f;
            msgset->h = argv[8]->f;
        }
        if (debug) printf ("path: %s name: %s ID: %d x: %f y: %f X: %f Y: %f m: %f\n",msgset->path, msgset->name, msgset->sessionID, msgset->x, msgset->y, msgset->X, msgset->Y, msgset->m);
            status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
            if (status < 0)
            {
 /*           perror("set (send) failed");
            if (errno == EPIPE)
                printf("EPIPE\n");*/
                //if (shutdown(clientsock,SHUT_RDWR) <0)
                //closesocket(servsock);
                //closesocket(clientsock);
            closesocket(clientsock);
            clientsock = 0;
            }
        free (msgset);
    }
    else if ( !strcmp((char *) argv[0],"fseq") && argv[1]->i != -1 )
    {
        fseq *msgset;
        int status;
        msgset = calloc (1, sizeof(fseq));
        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->framenum = argv[1]->i;
        if (debug) printf("path: %s name: %s fremseq: %d\n",msgset->path, msgset->name, msgset->framenum);
        alen = sizeof(address);
            status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
            if (status < 0)
            {
            /*if (status == EPIPE)
                printf("EPIPE\n");
            perror("fseq (send) failed");*/
                //if (shutdown(clientsock,SHUT_RDWR) <0)
                //closesocket(servsock);
                //closesocket(clientsock);
            closesocket(clientsock);
            clientsock = 0;
            }
        free(msgset);
    }
    else if ( !strcmp((char *) argv[0],"alive") )
    {
        struct _alive {
        char path[12];
        char name[8];
        int blobs[argc];
        };

        struct _alive *msgset;
        int status;
        msgset = calloc (1, sizeof(struct _alive));

        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->blobs[0] = argc;
        if (debug) printf("path: %s name: %s ",msgset->path, msgset->name);
        for (j=1;j<argc;j++)
        {
            msgset->blobs[j] = argv[j]->i;
            if (debug) printf("Id[%d] - %d ",j-1,msgset->blobs[j]);
        }
        if (debug) printf("\n");
            status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
            if (status < 0)
            {
            /*if (errno == EPIPE)
                printf("EPIPE\n");*/
            //if (errno == ECONNRESET)
                //printf("ECONNRESET\n");
            //printf("errno %d\n",errno);
                //if (shutdown(clientsock,SHUT_RDWR) <0)
                //closesocket(servsock);
            closesocket(clientsock);
            clientsock = 0;
            /*perror("alive (send) failed");*/
                //stoptcp();
            }
        free(msgset);
    }
    fflush(stdout);
    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data)
{
    if ( !strcmp((char *) path ,"/quit"))
    {
    done = 1;
    //if (debug)
    printf("Quiting\n\n");
    fflush(stdout);
    return 0;
    }
    else return toggle;
}

void sigint_handler(int sig)
{
    printf("Quiting! %d\n",done);
    //done = 1;
}
