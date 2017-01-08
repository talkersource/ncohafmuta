#ifndef AUTHUSER_H
#define AUTHUSER_H

extern unsigned short auth_tcpport;
extern int            auth_rtimeout;

extern char *auth_xline(register char *user, register int fd, register unsigned long *in);
extern int auth_fd(register int fd, register unsigned long *in, register unsigned short *local, register unsigned short *remote);
extern int auth_fd2(register int fd, register unsigned long *inlocal, register unsigned long *inremote, register unsigned short *local, register unsigned short *remote);
extern int auth_tcpsock(register unsigned long inlocal, register unsigned long inremote);
extern char *auth_tcpuser(register unsigned in, register unsigned short local, register unsigned short remote);
extern char *auth_tcpuser2(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote);
extern char *auth_tcpuser3(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote, register int ctimeout);
extern char *auth_tcpuser4(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote, register int ctimeout, register int rtimeout);
extern char *auth_sockuser(register int s, register unsigned short local, register unsigned short remote);
extern char *auth_sockuser2(register int s, register unsigned short local, register unsigned short remote, int rtimeout);

#endif