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
typedef struct
{
    char path[12];
    char name[8];
    void *data;
}tuio;

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

extern int errno;
char localhost[] = "localhost";

int main(int argc, char *argv[])
{
    struct hostent *ptrh;
    struct protoent *ptrp;
    struct sockaddr_in sad;
    int sd, port;
    char *host;
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
    while (1)
    {
        char buffer[1024];
        int bytes = 1;
        while (bytes >0)
        {
#ifdef WIN32
            bytes=recv(sd,buffer,sizeof(buffer), 0);
#elif linux
            bytes=read(sd,buffer,sizeof(buffer));
#endif
            if (bytes<0)
            {
                perror("read");
            }
            else

                if (bytes==0)
                {
                    close(sd);
                    printf("Socket closed\n");
                    exit(0);
                }
            tuio *msgset =  (tuio *) &buffer;

            if ( !strcmp((char *) msgset->name,"fseq"))
            {
                fseq *msgset = (fseq *) &buffer;
                printf("path: %s name: %s framenum : %d\n",msgset->path, msgset->name, msgset->framenum);
            }
            if ( !strcmp((char *) msgset->name,"set"))
            {
                set *msgset = (set *) &buffer;
                printf("path: %s name: %s id: %d, x: %f y: %f,X: %f,Y: %f m: %f\n",msgset->path ,msgset->name ,msgset->sessionID,msgset->x,msgset->y,msgset->X,msgset->Y,msgset->m);
            }
            if ( !strcmp((char *) msgset->name,"alive"))
            {
                int *blobs = (void *) &(msgset->data);
                int sizeb = blobs[0];
                int i;
                printf("path: %s name: %s numids: %d ::: ",msgset->path, msgset->name, blobs[0]);
                for (i=1;i<sizeb;i++)
                {
                printf("%d=>%d ",i,blobs[i]);
                 }
                 printf("\n");
            }

            fflush(stdout);

        }
        printf("\r\n");
    }
    closesocket(sd);
    exit (0);
}
