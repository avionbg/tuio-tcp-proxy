/*
 *  tuio_sdump.c
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

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#elif linux
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT 3000

extern int errno;
char localhost[] = "localhost";

int main(int argc, char *argv[])
{
    struct hostent *ptrh;
    struct protoent *ptrp;
    struct sockaddr_in sad;
    int sd, port, n;
    char *host, buf[1000];
#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
#endif
    memset((char *)&sad,0,sizeof(sad));
    sad.sin_family = AF_INET;

    if (argc > 2)
    {
        port = atoi(argv[2]);
    }
    else
    {
        port = PORT;
    }
    if (port > 0)
        sad.sin_port = htons((u_short)port);
    else
    {
        fprintf(stderr,"bad port number %s\n",argv[2]);
        exit(1);
    }
    if (argc > 1)
    {
        host = argv[1];
    }
    else
    {
        host = localhost;
    }
    ptrh = gethostbyname(host);
    if ( ((char *)ptrh) == NULL )
    {
        fprintf(stderr,"invalid host: %s\n", host);
        exit (1);
    }
    memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
    {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit (1);
    }
    while (1)
    {
        sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
        if (sd < 0)
        {
            fprintf(stderr, "socket creation failed\n");
            exit (1);
        }
        if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0)
        {
            fprintf(stderr,"connect failed\n");
            exit (1);
        }
        n = recv(sd, buf, sizeof(buf), 0);
        while (n > 0)
        {
            write(1,buf,n);
            n = recv(sd, buf, sizeof(buf), 0);
        }
        printf("\r\n");
    }
    closesocket(sd);
    exit (0);
}
