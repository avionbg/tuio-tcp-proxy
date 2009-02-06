/*
 * libtoxy.h
 *
 *  Created on: 2009/2/6
 */

#ifndef LIBTOXY_H_
#define LIBTOXY_H_

extern int servsock, clientsock, fwdport;
extern int done;
extern int debug;
extern char *port;
extern lo_server st;

void sigint_handler(int sig);

#if linux
#define closesocket close
#endif

#endif /* LIBTOXY_H_ */
