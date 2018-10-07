From: davy@ecn.purdue.edu (Dave Curry)
Newsgroups: comp.sources.unix
Subject: v26i116: getethers - find IP addrs for all local ether addrs (V1.4), Part01/01
Date: 10 Apr 1993 00:48:22 -0700
Approved: paul@vix.com
Message-ID: <1q5u46$i49@gw.home.vix.com>

Submitted-By: davy@ecn.purdue.edu (Dave Curry)
Posting-Number: Volume 26, Issue 116
Archive-Name: getethers/part01

(This is a new version of getethers, which originally appeared in Volume 25)

							May, 1992

This is GETETHERS Version 1.4.

New functionality since the 1.0 release in 11/91:

	- Thanks to Ric Anderson (ric@cs.arizona.edu), you can now write
	  Network General Sniffer data files as well as Excelan Lanalyzer
	  files.  The file format is selected with the -e and -s options
	  in conjunction with -w.

	- Thanks to Peter Shipley (shipley@tfs.com), when the -v option is
	  given, vendor names for the discovered ethernet addresses are
	  now printed as well as the previously printed data.

	- Thanks to Peter again, GETETHERS can now be compiled and run on
	  pretty much any 4.3BSD system.  It will *not* obtain the ethernet
	  address of the local host unless you're running on a Sun (4.3BSD
	  does not provide a way to do this).

	- GETETHERS will no longer exit if it bumps into an interface not
	  supported by the Network Interface Tap (e.g., Sun X.25 interfaces).

If you don't know what GETETHERS is:

GETETHERS runs through all the addresses on an ethernet (a.b.c.1 - a.b.c.254)
and pings each address, and then determines the ethernet address for that
host.  It produces a list, either in ASCII or in the binary format for an
Excelan Lanalyzer, of hostname/ethernet address pairs for all hosts on that
network.

This program has been tested on Sun workstations under SunOS 4.1.1, and on
DEC Vaxes under 4.3BSD.  It should be relatively easy to port to any system
with BSD-style networking.  Mostly you need to work on if.c a bit, and maybe
on arp.c.

If you make modifications or fixes, please send them to me for incorporation
into future versions.

	Dave Curry
	Purdue University
	Engineering Computer Network
	davy@ecn.purdue.edu

#! /bin/sh
# This is a shell archive.  Remove anything before this line, then unpack
# it by saving it into a file and typing "sh file".  To overwrite existing
# files, type "sh file -c".  You can also feed this as standard input via
# unshar, or by typing "sh <file", e.g..  If this archive is complete, you
# will see the following message at the end:
#		"End of archive 1 (of 1)."
# Contents:  MANIFEST Makefile README arp.c defs.h excelan.c
#   getethers.8l if.c main.c ping.c sniffer.c vendors.c
# Wrapped by vixie@gw.home.vix.com on Sat Apr 10 00:46:20 1993
PATH=/bin:/usr/bin:/usr/ucb ; export PATH
if test -f 'MANIFEST' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'MANIFEST'\"
else
echo shar: Extracting \"'MANIFEST'\" \(486 characters\)
sed "s/^X//" >'MANIFEST' <<'END_OF_FILE'
X   File Name		Archive #	Description
X-----------------------------------------------------------
X MANIFEST                   1	This shipping list
X Makefile                   1	
X README                     1	
X arp.c                      1	
X defs.h                     1	
X excelan.c                  1	
X getethers.8l               1	
X if.c                       1	
X main.c                     1	
X ping.c                     1	
X sniffer.c                  1	
X vendors.c                  1	
END_OF_FILE
if test 486 -ne `wc -c <'MANIFEST'`; then
    echo shar: \"'MANIFEST'\" unpacked with wrong size!
fi
# end of 'MANIFEST'
fi
if test -f 'Makefile' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'Makefile'\"
else
echo shar: Extracting \"'Makefile'\" \(952 characters\)
sed "s/^X//" >'Makefile' <<'END_OF_FILE'
X#
X# $Header: /usr/src/ecn/getethers/RCS/Makefile,v 1.4 92/05/08 14:15:36 davy Exp $
X#
X# Makefile for getethers.
X#
X# David A. Curry
X# Purdue University
X# Engineering Computer Network
X# davy@ecn.purdue.edu
X# November, 1991
X#
X#BINDIR=	/usr/ecn/etc
BINDIR=	/usr/local/etc
X#MANDIR=	/usr/man/man8
MANDIR=	/usr/local/man/man8
X
X#
X# Add -DNEED_STRDUP if you don't have the strdup() library routine.
X# Add -DNEED_ENTOA  if you don'y have the ether_ntoa() library routine.
X#
DEFS=
CFLAGS=	-O $(DEFS)
X
OBJS=	arp.o excelan.o if.o main.o ping.o sniffer.o vendors.o
X
getethers: $(OBJS)
X	$(CC) -o getethers $(OBJS)
X
install: getethers
X	install -c -s -m 4750 -o root -g wheel getethers $(BINDIR)/getethers
X	install -c -m 644 getethers.8l $(MANDIR)/getethers.8l
X
clean:
X	rm -f a.out core getethers *.o \#*
X
arp.o:		arp.c defs.h
excelan.o:	excelan.c defs.h
main.o:		main.c defs.h
if.o:		if.c defs.h
ping.o:		ping.c
sniffer.o:	sniffer.c defs.h
vendors.o:	vendors.c defs.h
END_OF_FILE
if test 952 -ne `wc -c <'Makefile'`; then
    echo shar: \"'Makefile'\" unpacked with wrong size!
fi
# end of 'Makefile'
fi
if test -f 'README' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'README'\"
else
echo shar: Extracting \"'README'\" \(1667 characters\)
sed "s/^X//" >'README' <<'END_OF_FILE'
X							May, 1992
X
This is GETETHERS Version 1.4.
X
New functionality since the 1.0 release in 11/91:
X
X	- Thanks to Ric Anderson (ric@cs.arizona.edu), you can now write
X	  Network General Sniffer data files as well as Excelan Lanalyzer
X	  files.  The file format is selected with the -e and -s options
X	  in conjunction with -w.
X
X	- Thanks to Peter Shipley (shipley@tfs.com), when the -v option is
X	  given, vendor names for the discovered ethernet addresses are
X	  now printed as well as the previously printed data.
X
X	- Thanks to Peter again, GETETHERS can now be compiled and run on
X	  pretty much any 4.3BSD system.  It will *not* obtain the ethernet
X	  address of the local host unless you're running on a Sun (4.3BSD
X	  does not provide a way to do this).
X
X	- GETETHERS will no longer exit if it bumps into an interface not
X	  supported by the Network Interface Tap (e.g., Sun X.25 interfaces).
X
If you don't know what GETETHERS is:
X
GETETHERS runs through all the addresses on an ethernet (a.b.c.1 - a.b.c.254)
and pings each address, and then determines the ethernet address for that
host.  It produces a list, either in ASCII or in the binary format for an
XExcelan Lanalyzer, of hostname/ethernet address pairs for all hosts on that
network.
X
This program has been tested on Sun workstations under SunOS 4.1.1, and on
DEC Vaxes under 4.3BSD.  It should be relatively easy to port to any system
with BSD-style networking.  Mostly you need to work on if.c a bit, and maybe
on arp.c.
X
If you make modifications or fixes, please send them to me for incorporation
into future versions.
X
Dave Curry
Purdue University
XEngineering Computer Network
davy@ecn.purdue.edu
END_OF_FILE
if test 1667 -ne `wc -c <'README'`; then
    echo shar: \"'README'\" unpacked with wrong size!
fi
# end of 'README'
fi
if test -f 'arp.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'arp.c'\"
else
echo shar: Extracting \"'arp.c'\" \(2182 characters\)
sed "s/^X//" >'arp.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/arp.c,v 1.3 92/05/08 14:15:55 davy Exp $";
X#endif
X
X/*
X * arp.c - routines for digging up arp table entries.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	arp.c,v $
X * Revision 1.3  92/05/08  14:15:55  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.2  92/05/08  13:06:31  davy
X * Added vendor name printing from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.1  91/11/27  10:56:13  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <netinet/in.h>
X#include <sys/ioctl.h>
X#include <net/if.h>
X#ifdef sun
X#include <net/if_arp.h>
X#endif
X#include <netinet/if_ether.h>
X#include <string.h>
X#include <errno.h>
X#include <stdio.h>
X#include "defs.h"
X
extern int errno;
X
X/*
X * get_arp - get the arp table entry for the internet address in addr, and
X *	     return the ethernet address as a character string.
X */
char *
get_arp(addr, vname)
struct in_addr addr;
char **vname;
X{
X	int s;
X	struct arpreq ar;
X	struct sockaddr_in *sin;
X	char *ether_ntoa(), *vendor_name();
X
X	/*
X	 * Clear the structure.
X	 */
X	bzero((char *) &ar, sizeof(struct arpreq));
X
X	/*
X	 * We want internet family only.
X	 */
X	ar.arp_pa.sa_family = AF_INET;
X	sin = (struct sockaddr_in *) &ar.arp_pa;
X
X	/*
X	 * Copy in the internet address.
X	 */
X	sin->sin_family = AF_INET;
X	bcopy((char *) &addr, (char *) &sin->sin_addr, sizeof(struct in_addr));
X
X	/*
X	 * Get a socket.
X	 */
X	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
X		error("socket");
X		return(strdup("(unknown)"));
X	}
X
X	/*
X	 * Get the arp table entry.
X	 */
X	if (ioctl(s, SIOCGARP, (char *) &ar) < 0) {
X		/*
X		 * Nothing in the table.
X		 */
X		if (errno == ENXIO) {
X			close(s);
X			return(strdup("(no entry)"));
X		}
X
X		error("ioctl: SIOCGARP");
X		close(s);
X
X		return("(unknown)");
X	}
X
X	close(s);
X
X	/*
X	 * Is the entry complete?
X	 */
X	if (ar.arp_flags & ATF_COM) {
X		*vname = vendor_name((struct ether_addr *) ar.arp_ha.sa_data);
X		return(strdup(ether_ntoa((struct ether_addr *) ar.arp_ha.sa_data)));
X	}
X
X	return(strdup("(incomplete)"));
X}
END_OF_FILE
if test 2182 -ne `wc -c <'arp.c'`; then
    echo shar: \"'arp.c'\" unpacked with wrong size!
fi
# end of 'arp.c'
fi
if test -f 'defs.h' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'defs.h'\"
else
echo shar: Extracting \"'defs.h'\" \(1660 characters\)
sed "s/^X//" >'defs.h' <<'END_OF_FILE'
X/*
X * $Header: /usr/src/ecn/getethers/RCS/defs.h,v 1.5 92/05/08 14:18:38 davy Exp $
X *
X * Definitions for getethers.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	defs.h,v $
X * Revision 1.5  92/05/08  14:18:38  davy
X * Fix from David Ferbrache (ferbrache@ajaz.rsre.mod.uk) for passing over
X * non-Ethernet links.
X * 
X * Revision 1.4  92/05/08  14:15:57  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.3  92/05/08  13:06:33  davy
X * Added vendor name printing from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.2  92/05/08  10:54:43  davy
X * Added changes to dump Sniffer format files from Ric Anderson,
X * ric@cs.arizona.edu.
X * 
X * Revision 1.1  91/11/27  10:56:28  davy
X * Initial revision
X * 
X */
X#define VERSION		1.4
X
X#define MAXHOST		256		/* max number of hosts to check	*/
X#define MINADDR		1		/* minimum host number		*/
X#define MAXADDR		254		/* maximum host number		*/
X#define MAXPING		3		/* max number of pings to send	*/
X#define PACKWAIT	1		/* min time to wait for packet	*/
X#define MAXPACKET	4096		/* max packet size for ping	*/
X#define FILESIZE	2584		/* size of lanalyzer file	*/
X
X/*
X * Analyzer file output types.
X */
X#define EXCELAN		1
X#define SNIFFER		2
X
X/*
X * Record for a host.
X */
typedef struct {
X	char	*hl_name;		/* host name			*/
X	char	*hl_inet;		/* internet address		*/
X	char	*hl_ether;		/* ethernet address		*/
X	char	*hl_vname;		/* vendor name			*/
X} HostInfo;
X
X#ifdef NEED_ENTOA
struct ether_addr {
X        u_char  ether_addr_octet[6];
X};
X
char	*ether_ntoa();
X#endif
X
X#ifdef NEED_STRDUP
char	*strdup();
X#endif
END_OF_FILE
if test 1660 -ne `wc -c <'defs.h'`; then
    echo shar: \"'defs.h'\" unpacked with wrong size!
fi
# end of 'defs.h'
fi
if test -f 'excelan.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'excelan.c'\"
else
echo shar: Extracting \"'excelan.c'\" \(1748 characters\)
sed "s/^X//" >'excelan.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/excelan.c,v 1.1 91/11/27 10:56:30 davy Exp $";
X#endif
X
X/*
X * excelan.c - routines to write lanalyzer files.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	excelan.c,v $
X * Revision 1.1  91/11/27  10:56:30  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <net/if.h>
X#include <netinet/in.h>
X#include <netinet/if_ether.h>
X#include <stdio.h>
X#include "defs.h"
X
int nbytes;				/* number of bytes written	*/
X
X/*
X * excelan_header - put out the mysterious crap at the top of the file.
X */
excelan_header(fp)
XFILE *fp;
X{
X	int i;
X
X	fwrite(" \20L\0\01\01", 1, 6, fp);
X	fwrite("Excelan host name file", 1, 22, fp);
X
X	for (i = 0; i < 52; i++)
X		fputc('\0', fp);
X
X	fwrite("!\20\304\t", 1, 4, fp);
X
X	nbytes = 84;
X}
X
X/*
X * excelan_entry - write out an ethernet address/hostname pair.
X */
excelan_entry(h, fp)
HostInfo *h;
XFILE *fp;
X{
X	int i;
X	register char *p;
X	struct ether_addr *ea, *ether_aton();
X
X	/*
X	 * Magic.
X	 */
X	fputc('\31', fp);
X
X	/*
X	 * Write the ethernet address.
X	 */
X	ea = ether_aton(h->hl_ether);
X	fwrite(ea, 1, 6, fp);
X
X	/*
X	 * Put the hostname.
X	 */
X	for (i=0, p=h->hl_name; (i < 18) && (*p != '.'); i++, p++)
X		fputc(*p, fp);
X
X	/*
X	 * Pad with nulls.
X	 */
X	for (; i < 18; i++)
X		fputc('\0', fp);
X
X	nbytes += 25;
X}
X
X/*
X * excelan_footer - pad the file to FILESIZE bytes.
X */
excelan_footer(fp)
XFILE *fp;
X{
X	/*
X	 * Put out the broadcast address.
X	 */
X	fputc('\31', fp);
X	fwrite("\377\377\377\377\377\377", 1, 6, fp);
X	fwrite("BROADCAST\0", 1, 10, fp);
X
X	nbytes += 17;
X
X	/*
X	 * Pad the file.
X	 */
X	for (; nbytes < FILESIZE; nbytes++)
X		fputc('\0', fp);
X}
END_OF_FILE
if test 1748 -ne `wc -c <'excelan.c'`; then
    echo shar: \"'excelan.c'\" unpacked with wrong size!
fi
# end of 'excelan.c'
fi
if test -f 'getethers.8l' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'getethers.8l'\"
else
echo shar: Extracting \"'getethers.8l'\" \(2569 characters\)
sed "s/^X//" >'getethers.8l' <<'END_OF_FILE'
X.TH GETETHERS 8L "27 November 1991" "ECN"
X.SH NAME
getethers \- get ethernet address/hostname information
X.SH SYNOPSIS
X.B getethers
X[
X.B \-e
X] [
X.B \-s
X] [
X.B \-v
X] [
X.B \-w
X]
X.I net
X[
X.IR net ...
X]
X.SH DESCRIPTION
X.PP
X.B getethers
probes the specified network(s),
whose addresses are given in Internet ``dot'' notation,
and produces a list of host name and ethernet address pairs for each host
on the network that responds to ICMP ECHO_REQUEST packets.
The program assumes a subnet mask of 255.255.255.0 (0xffffff00),
and does not probe host number 0 or host number 255,
which are used as the broadcast address.
X.PP
If the
X.B \-v
option is specified,
a list is produced on the standard output that contains the host name,
internet address,
ethernet address,
and vendor name of the ethernet board for each host that responded to
ICMP ECHO_REQUEST packets.
X.PP
If the
X.B \-w
option is specified,
along with either the
X.B \-e
or
X.B \-s
options,
a file will be written for each network that can be used with the Excelan
Lanalyzer (\fB\-e\fP)
or Network General Sniffer (\fB\-s\fP)
products as a name-to-address translation file.
The name of the file will be
X.IR XXXnet.nam ,
where
X.I XXX
is the last byte of the network address.
X.PP
X.B getethers
works by first probing the system's ethernet interfaces,
looking for the interface that is connected to the specified network.
If the system is not connected to that network,
X.B getethers
prints an error message and goes on to the next network.
After discovering the proper interface,
X.B getethers
sends up to three ICMP ECHO_REQUEST packets to each host number on that
network from 1 to 254.
If the host responds,
X.B getethers
then searches the local host's arp table for the remote host's ethernet
address.
If the remote host does not respond to the ECHO_REQUEST packets within
three seconds,
it is assumed to be down or non-existent,
and is skipped.
X.SH SEE ALSO
X.BR arp (4P),
X.BR arp (8C),
X.BR etherfind (8C),
X.BR ethers (3N),
X.BR icmp (4P),
X.BR inet (3N),
X.BR nit (4P),
X.BR ping (8C),
X.BR rarpd (8C)
X.SH BUGS
X.PP
The assumption of a 255.255.255.0 subnet mask,
and the assumption of all-zero or all-ones as a broadcast address,
are probably not good ideas and should be determined at run-time,
but it works in our environment.
X.PP
The Lanalyzer file,
due to its fixed size,
cannot handle more than 100 entries.
If a network has more than 100 hosts connected (and up),
the file will not be usable (or at least the stuff after 100 entries won't
be).
X.SH AUTHOR
David A. Curry, Purdue University Engineering Computer Network
END_OF_FILE
if test 2569 -ne `wc -c <'getethers.8l'`; then
    echo shar: \"'getethers.8l'\" unpacked with wrong size!
fi
# end of 'getethers.8l'
fi
if test -f 'if.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'if.c'\"
else
echo shar: Extracting \"'if.c'\" \(3917 characters\)
sed "s/^X//" >'if.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/if.c,v 1.4 92/05/08 14:19:29 davy Exp $";
X#endif
X
X/*
X * if.c - routines to check a system's ethernet interfaces.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	if.c,v $
X * Revision 1.4  92/05/08  14:19:29  davy
X * Fix from David Ferbrache (ferbrache@ajaz.rsre.mod.uk) for passing over
X * non-Ethernet links.
X * 
X * Revision 1.3  92/05/08  14:15:58  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.2  92/05/08  13:06:36  davy
X * Added vendor name printing from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.1  91/11/27  10:56:31  davy
X * Initial revision
X * 
X */
X#ifdef sun
X
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <netinet/in.h>
X#include <arpa/inet.h>
X#include <sys/ioctl.h>
X#include <sys/time.h>
X#include <sys/file.h>
X#include <net/if.h>
X#include <net/nit_if.h>
X#include <netinet/if_ether.h>
X#include <string.h>
X#include <stdio.h>
X#include "defs.h"
X
X/*
X * check_if - check our ethernet interfaces and find the one that's attached
X *	      to the network in addr.
X */
check_if(addr, hosts)
struct in_addr addr;
HostInfo *hosts;
X{
X	int n, s, lna;
X	char buf[BUFSIZ];
X	struct ifreq ifr;
X	struct ifconf ifc;
X	char *vendor_name();
X	struct sockaddr *sad;
X	struct sockaddr_in *sin;
X	register struct ifreq *ifrp;
X
X	/*
X	 * Need a socket...
X	 */
X	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
X		error("socket");
X		return(-1);
X	}
X
X	ifc.ifc_buf = buf;
X	ifc.ifc_len = sizeof(buf);
X
X	/*
X	 * Get the list of configured interfaces.
X	 */
X	if (ioctl(s, SIOCGIFCONF, (char *) &ifc) < 0) {
X		error("ioctl: SIOCGIFCONF");
X		close(s);
X
X		return(-1);
X	}
X
X	close(s);
X
X	/*
X	 * For each interface...
X	 */
X	ifrp = ifc.ifc_req;
X	for (n = ifc.ifc_len/sizeof(struct ifreq); n > 0; n--, ifrp++) {
X		bcopy((char *) ifrp, (char *) &ifr, sizeof(struct ifreq));
X
X		/*
X		 * Need a new socket.
X		 */
X		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
X			error("socket");
X			return(-1);
X		}
X
X		/*
X		 * Get the address of this interface.
X		 */
X		if (ioctl(s, SIOCGIFADDR, (char *) &ifr) < 0) {
X			error("ioctl: SIOCGIFADDR");
X			close(s);
X			
X			continue;
X		}
X
X		close(s);
X
X		sin = (struct sockaddr_in *) &ifr.ifr_addr;
X
X		/*
X		 * If it's not the one we need, bag it.
X		 */
X		if ((sin->sin_addr.s_addr & addr.s_addr) != addr.s_addr)
X			continue;
X
X		/*
X		 * Save the internet address, and keep the host byte.
X		 * We are assuming a netmask of 255.255.255.0 here.
X		 */
X		lna = inet_lnaof(sin->sin_addr) & 0xff;
X		hosts[lna].hl_inet = strdup(inet_ntoa(sin->sin_addr));
X
X		/*
X		 * Now we need the NIT, to get the ethernet address.
X		 */
X		if ((s = open("/dev/nit", O_RDONLY)) < 0) {
X			error("open: /dev/nit");
X			return(-1);
X		}
X
X		/*
X		 * Bind the nit to this interface.
X		 */
X		if (ioctl(s, NIOCBIND, (char *) ifrp) < 0) {
X			error("ioctl: NIOCBIND");
X			close(s);
X
X			return(-1);
X		}
X
X		/*
X		 * Get the address.
X		 */
X		if (ioctl(s, SIOCGIFADDR, (char *) ifrp) < 0) {
X			error("ioctl: SIOCGIFADDR");
X			close(s);
X
X			continue;
X		}
X
X		close(s);
X
X		/*
X		 * Save the ethernet address.
X		 */
X		sad = (struct sockaddr *) &ifrp->ifr_addr;
X		hosts[lna].hl_ether = strdup(ether_ntoa((struct ether_addr *)
X							sad->sa_data));
X		hosts[lna].hl_vname = vendor_name((struct ether_addr *)
X						  sad->sa_data);
X
X		return(lna);
X	}
X
X	return(-1);
X}
X
X#else
X
X#include <sys/param.h>
X#include <netinet/in.h>
X#include <netdb.h>
X#include "defs.h"
X
check_if(addr, hosts)
struct in_addr addr;
HostInfo *hosts;
X{
X	int lna;
X	char hn[256];
X	struct hostent *hp;
X	extern char *strdup();
X
X	(void) gethostname(hn, sizeof(hn));
X
X	hp = gethostbyname(hn);
X
X	lna = inet_lnaof(hp->h_addr) & 0xff;
X	hosts[lna].hl_inet = strdup(inet_ntoa(hp->h_addr));
X	hosts[lna].hl_name = strdup(hp->h_name);
X	hosts[lna].hl_ether = "???";
X	hosts[lna].hl_vname = "???";
X	return(0);
X}
X
X#endif
END_OF_FILE
if test 3917 -ne `wc -c <'if.c'`; then
    echo shar: \"'if.c'\" unpacked with wrong size!
fi
# end of 'if.c'
fi
if test -f 'main.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'main.c'\"
else
echo shar: Extracting \"'main.c'\" \(6625 characters\)
sed "s/^X//" >'main.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/main.c,v 1.4 92/05/08 14:15:59 davy Exp $";
X#endif
X/*
X * getethers - get hostname/ethernet address information for all hosts on
X *	       an ethernet.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	main.c,v $
X * Revision 1.4  92/05/08  14:15:59  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.3  92/05/08  13:06:38  davy
X * Added vendor name printing from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.2  92/05/08  09:27:48  davy
X * Added changes to dump Sniffer format files from Ric Anderson,
X * ric@cs.arizona.edu.
X * 
X * Revision 1.1  91/11/27  10:56:32  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <netinet/in.h>
X#include <arpa/inet.h>
X#include <string.h>
X#include <netdb.h>
X#include <stdio.h>
X#include "defs.h"
X
char	*pname;
X
main(argc, argv)
char **argv;
int argc;
X{
X	char *p;
X	FILE *fp;
X	char *vname;
X	u_long network;
X	struct hostent *hp;
X	struct in_addr addr;
X	HostInfo hosts[MAXHOST];
X	char hname[64], fname[BUFSIZ];
X	char *get_arp(), *strip_domain();
X	int analyzer_type, lna, verbose, writefile;
X
X	pname = *argv;
X	analyzer_type = verbose = writefile = 0;
X
X	if (argc < 2)
X		usage();
X
X	/*
X	 * Get our hostname.
X	 */
X	if (gethostname(hname, sizeof(hname)) < 0) {
X		error("gethostname");
X		exit(1);
X	}
X
X	/*
X	 * Process arguments.
X	 */
X	while (--argc) {
X		/*
X		 * Option.
X		 */
X		if (**++argv == '-') {
X			switch (*++*argv) {
X			case 'e':			/* excelan	*/
X				analyzer_type = EXCELAN;
X				break;
X			case 's':			/* sniffer	*/
X				analyzer_type = SNIFFER;
X				break;
X			case 'v':			/* verbose	*/
X				verbose = 1;
X				break;
X			case 'w':			/* write files	*/
X				writefile = 1;
X				break;
X			default:
X				usage();
X				break;
X			}
X
X			continue;
X		}
X
X		/*
X		 * Do *something*.
X		 */
X		if (!verbose && !writefile)
X			verbose = 1;
X
X		/*
X		 * Just so they know.
X		 */
X		if (writefile && !analyzer_type) {
X			fprintf(stderr, "%s: -w with no format; excelan format assumed.\n",
X				pname);
X			analyzer_type = EXCELAN;
X		}
X
X		/*
X		 * Convert the given network address to an internet
X		 * address.
X		 */
X		network = inet_network(*argv);
X		addr = inet_makeaddr(network, 0);
X		bzero(hosts, sizeof(hosts));
X
X		/*
X		 * Find the ethernet interface that's on that network.
X		 */
X		if ((lna = check_if(addr, hosts)) < 0) {
X			fprintf(stderr, "%s: this host is not on the %s net.\n",
X				pname, inet_ntoa(addr));
X			continue;
X		}
X
X		/*
X		 * Save our hostname.  check_if() filled in our
X		 * internet address and ethernet address.
X		 */
X		hosts[lna].hl_name = strip_domain(hname);
X
X		if (verbose)
X			printf("%s:\n    ", *argv);
X
X		/*
X		 * For each host...
X		 */
X		for (lna = MINADDR; lna <= MAXADDR; lna++) {
X			if ((verbose == 1) && ((lna % 16) == 0)) {
X				printf("%d...", lna);
X				fflush(stdout);
X			}
X
X			/*
X			 * Skip our entry; we did it already.
X			 */
X			if (hosts[lna].hl_name != NULL)
X				continue;
X
X			/*
X			 * Build the internet address.
X			 */
X			addr = inet_makeaddr(network, lna);
X
X			/*
X			 * Ping it, and if it's up...
X			 */
X			if (ping(addr, lna)) {
X				/*
X				 * Get the hostname.
X				 */
X				hp = gethostbyaddr(&addr.s_addr,
X						   sizeof(addr.s_addr),
X						   AF_INET);
X
X				/*
X				 * Save the hostname.
X				 */
X				if (hp != NULL)
X					hosts[lna].hl_name =
X						strip_domain(hp->h_name);
X				else
X					hosts[lna].hl_name = strdup("???");
X
X				/*
X				 * Save the internet address and get the
X				 * ethernet address from the arp table.
X				 */
X				hosts[lna].hl_inet = strdup(inet_ntoa(addr));
X				hosts[lna].hl_ether = get_arp(addr, &vname);
X				hosts[lna].hl_vname = vname;
X			}
X		}
X
X		if (verbose)
X			putchar('\n');
X
X		/*
X		 * If we need to write files, create the file for this
X		 * network.
X		 */
X		if (writefile) {
X			p = strrchr(*argv, '.') + 1;
X			sprintf(fname, "%snet.nam", p);
X
X			if ((fp = fopen(fname, "w")) == NULL) {
X				error(fname);
X				exit(1);
X			}
X
X			switch (analyzer_type) {
X			case EXCELAN:
X				excelan_header(fp);
X				break;
X			case SNIFFER:
X				sniffer_header(fp);
X				break;
X			}
X		}
X			
X		if (verbose) {
X			printf("    Hostname       Internet Addr   ");
X			printf("  Ethernet Addr    Vendor Name\n");
X			printf("--------------------------");
X			printf("--------------------------");
X			printf("--------------------------\n");
X		}
X
X		/*
X		 * Write or print each entry.
X		 */
X		for (lna = MINADDR; lna <= MAXADDR; lna++) {
X			if (hosts[lna].hl_name == NULL)
X				continue;
X
X			if (writefile) {
X				switch (analyzer_type) {
X				case EXCELAN:
X					excelan_entry(&hosts[lna], fp);
X					break;
X				case SNIFFER:
X					sniffer_entry(&hosts[lna], fp);
X					break;
X				}
X			}
X				
X			if (verbose) {
X				printf("%-16.16s  %-15.15s  %-17.17s  %s\n",
X				       hosts[lna].hl_name, hosts[lna].hl_inet,
X				       hosts[lna].hl_ether,
X				       hosts[lna].hl_vname);
X			}
X
X			free(hosts[lna].hl_name);
X			free(hosts[lna].hl_inet);
X			free(hosts[lna].hl_ether);
X		}
X
X		/*
X		 * Write a footer and close the file.
X		 */
X		if (writefile) {
X			switch (analyzer_type) {
X			case EXCELAN:
X				excelan_footer(fp);
X				break;
X			case SNIFFER:
X				sniffer_footer(fp);
X				break;
X			}
X
X			fclose(fp);
X		}
X	}
X
X	exit(0);
X}
X
X/*
X * strip_domain - strip the domain name
X */
char *
strip_domain(name)
char *name;
X{
X	char *index();
X	register char *p;
X
X	if ((p = index(name, '.')) != NULL)
X		*p = '\0';
X
X	return(strdup(name));
X}
X
X#ifdef NEED_ENTOA
X
X#define ETHER_INDEX(i, j)   (unsigned int)(i->ether_addr_octet[(j)])
X
char *
ether_ntoa(ea)
struct ether_addr *ea;
X{
X	static char s[20];
X
X	s[0] = 0;
X	(void) sprintf(s, "%x:%x:%x:%x:%x:%x",
X		       ETHER_INDEX(ea,0), ETHER_INDEX(ea,1), ETHER_INDEX(ea,2),
X		       ETHER_INDEX(ea,3), ETHER_INDEX(ea,4), ETHER_INDEX(ea,5));
X	return (s);
X}
X
struct ether_addr *
ether_aton(s)
char *s;
X{
X	int i;
X	unsigned int t[6];
X	static struct ether_addr ep;
X
X	bzero(&ep, sizeof (struct ether_addr));
X
X	i = sscanf(s, " %x:%x:%x:%x:%x:%x",
X		   &t[0], &t[1], &t[2], &t[3], &t[4], &t[5]);
X
X	if (i != 6)
X		return ((struct ether_addr *)NULL);
X
X	for (i = 0; i < 6; i++)
X		ep.ether_addr_octet[i] = t[i];
X
X	return (&ep);
X}
X#endif
X
X#ifdef NEED_STRDUP
char *
strdup(s)
char *s;
X{
X	char *p;
X	char *malloc();
X
X	if ((p = malloc(strlen(s)+1)) == NULL) {
X		fprintf(stderr, "%s: out of memory.\n", pname);
X		exit(1);
X	}
X
X	strcpy(p, s);
X	return(p);
X}
X#endif
X
X/*
X * error - perror with program name.
X */
error(s)
char *s;
X{
X	fprintf(stderr, "%s: ", pname);
X	perror(s);
X}
X
usage()
X{
X	fprintf(stderr, "Usage: %s [-v] [-w] network [network...]\n", pname);
X	exit(1);
X}
END_OF_FILE
if test 6625 -ne `wc -c <'main.c'`; then
    echo shar: \"'main.c'\" unpacked with wrong size!
fi
# end of 'main.c'
fi
if test -f 'ping.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'ping.c'\"
else
echo shar: Extracting \"'ping.c'\" \(4244 characters\)
sed "s/^X//" >'ping.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/ping.c,v 1.2 92/05/08 14:16:01 davy Exp $";
X#endif
X
X/*
X * ping.c - routines for pinging a host.
X *
X * David A. Curry
X * Purdue University
X * Engineering Computer Network
X * davy@ecn.purdue.edu
X * November, 1991
X *
X * $Log:	ping.c,v $
X * Revision 1.2  92/05/08  14:16:01  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.1  91/11/27  10:56:33  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <sys/file.h>
X#include <netinet/in_systm.h>
X#include <netinet/in.h>
X#include <netinet/ip.h>
X#include <netinet/ip_icmp.h>
X#include <signal.h>
X#include <setjmp.h>
X#include <netdb.h>
X#include <errno.h>
X#include <stdio.h>
X#include "defs.h"
X
static jmp_buf	env;
static int	ident;
static int	datalen = 64 - 8;
static u_char	inpacket[MAXPACKET], outpacket[MAXPACKET];
X
extern int	errno;
X
X/*
X * ping - send ICMP ECHO REQUEST packets to the host at addr until it either
X *	  responds or we decide to bag it.  Most of this code was stolen and
X *	  simplified from Mike Muuss' ping program.
X */
ping(addr, id)
struct in_addr addr;
int id;
X{
X	int ringring();
X	register int i, n, s;
X	int cc, hlen, fromlen;
X	register u_char *datap;
X	register struct ip *ip;
X	register struct icmp *icp;
X	struct sockaddr_in from, to;
X	static struct protoent *proto = NULL;
X
X	bzero((char *) &to, sizeof(struct in_addr));
X
X	/*
X	 * Construct destination address.
X	 */
X	to.sin_family = AF_INET;
X	bcopy((char *) &addr, (char *) &to.sin_addr, sizeof(struct in_addr));
X
X	/*
X	 * ICMP ID number.
X	 */
X	ident = (getpid() + id) & 0xFFFF;
X
X	/*
X	 * Look up protocol number.
X	 */
X	if (proto == NULL) {
X		if ((proto = getprotobyname("icmp")) == NULL) {
X			error("icmp: unknown protocol");
X			return(0);
X		}
X	}
X
X	/*
X	 * Need a raw socket.
X	 */
X	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
X		error("socket");
X		return(0);
X	}
X
X	icp = (struct icmp *) outpacket;
X
X	/*
X	 * Send up to MAXPING packets.
X	 */
X	for (i=0; i < MAXPING; i++) {
X		/*
X		 * Construct ICMP header.
X		 */
X		icp->icmp_type = ICMP_ECHO;
X		icp->icmp_code = 0;
X		icp->icmp_cksum = 0;
X		icp->icmp_seq = i;
X		icp->icmp_id = ident;
X
X		/*
X		 * Stick some junk in the packet.
X		 */
X		cc = datalen + 8;
X		datap = &outpacket[8];
X
X		for (n = 0; n < datalen; n++)
X			*datap++ = n;
X
X		/*
X		 * Compute the IP checksum.
X		 */
X		icp->icmp_cksum = in_cksum(icp, cc);
X
X		/*
X		 * Send the packet...
X		 */
X		n = sendto(s, outpacket, cc, 0, &to, sizeof(struct sockaddr));
X
X		if ((n < 0) || (n != cc)) {
X			if (n < 0)
X				error("sendto");
X			else
X				error("sendto truncated");
X
X			close(s);
X			return(0);
X		}
X
X		/*
X		 * We'll wait for PACKWAIT seconds for a response.
X		 */
X		signal(SIGALRM, ringring);
X		fromlen = sizeof(struct sockaddr_in);
X
X		alarm(PACKWAIT);
X
X		/*
X		 * Bag it... send the next packet.
X		 */
X		if (setjmp(env))
X			continue;
X
X		/*
X		 * Get the packet.
X		 */
X		cc = recvfrom(s, inpacket, MAXPACKET, 0, &from, &fromlen);
X		alarm(0);
X
X		if (cc < 0) {
X			if (errno == EINTR)
X				continue;
X
X			error("recvfrom");
X			continue;
X		}
X
X		/*
X		 * Make sure it's a reply to ours.
X		 */
X		ip = (struct ip *) inpacket;
X		hlen = ip->ip_hl << 2;
X
X		if (cc < (hlen + ICMP_MINLEN))
X			continue;
X
X		cc -= hlen;
X		icp = (struct icmp *) (&inpacket[hlen]);
X
X		if (icp->icmp_type != ICMP_ECHOREPLY)
X			continue;
X
X		if (icp->icmp_id != ident)
X			continue;
X
X		/*
X		 * Yay!  The host is up.
X		 */
X		close(s);
X		return(1);
X	}
X
X	/*
X	 * Boo!  The host is down.
X	 */
X	close(s);
X	return(0);
X}
X
X/*
X * in_cksum - compute the IP checksum.
X */
in_cksum(addr, len)
u_short *addr;
int len;
X{
X	register u_short *w = addr;
X	register int nleft = len;
X	register u_short answer;
X	register int sum = 0;
X
X	/*
X	 * Use a 32-bit accumulator (sum) and add sequential 16-bit
X	 * words to it, then fold back all the carry bits from the
X	 * top 16 bits into the lower 16 bits.
X	 */
X	while (nleft > 1) {
X		sum += *w++;
X		nleft -= 2;
X	}
X
X	/*
X	 * Pick up odd byte if necessary.
X	 */
X	if (nleft == 1)
X		sum += *(u_char *) w;
X
X	/*
X	 * Add back the carry bits.
X	 */
X	sum = (sum >> 16) + (sum & 0xFFFF);
X	sum += (sum >> 16);
X
X	/*
X	 * Truncate to 16 bits.
X	 */
X	answer = ~sum;
X
X	return(answer);
X}
X
ringring()
X{
X	longjmp(env, 1);
X}
END_OF_FILE
if test 4244 -ne `wc -c <'ping.c'`; then
    echo shar: \"'ping.c'\" unpacked with wrong size!
fi
# end of 'ping.c'
fi
if test -f 'sniffer.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'sniffer.c'\"
else
echo shar: Extracting \"'sniffer.c'\" \(2787 characters\)
sed "s/^X//" >'sniffer.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/sniffer.c,v 1.1 92/05/08 09:27:49 davy Exp $";
X#endif
X/*
X * sniffer.c - routines to write Network General Sniffer files.
X *
X * Ric Anderson
X * University of Arizona
X * ric@cs.arizona.edu
X *
X * $Log:	sniffer.c,v $
X * Revision 1.1  92/05/08  09:27:49  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <net/if.h>
X#include <netinet/in.h>
X#include <netinet/if_ether.h>
X#include <stdio.h>
X#include <string.h>
X#include "defs.h"
X
X#define PADDING_SIZE 18
X
X/*
X * sniffer_header - put out the constant stuff at top of file.
X */
sniffer_header(fp)
XFILE *fp;
X{
X    int i;
X
X    fprintf(fp,"station \"Broadcast\"     = addrtype \"DLC\" FFFFFFFFFFFF\n");
X    fprintf(fp,"station \"Broadcast\"     = addrtype \"XNS\" FFFFFFFFFFFF\n");
X    fprintf(fp,"station \"MOP Download\"  = addrtype \"DLC\" AB0000010000\n");
X    fprintf(fp,"station \"DEC Rem Cons\"  = addrtype \"DLC\" AB0000020000\n");
X    fprintf(fp,"station \"DEC Endnode\"   = addrtype \"DLC\" AB0000040000\n");
X    fprintf(fp,"station \"DEC Routers\"   = addrtype \"DLC\" AB0000030000\n");
X    fprintf(fp,"station \"LTM listnrs\"   = addrtype \"DLC\" 09002B000003\n");
X    fprintf(fp,"station \"LAT units\"     = addrtype \"DLC\" 09002B00000F\n");
X    fprintf(fp,"station \"NetBIOS\"       = addrtype \"DLC\" 030000000001\n");
X}
X
X/*
X * sniffer_entry - write out an ethernet address/hostname pair.
X */
sniffer_entry(h, fp)
HostInfo *h;
XFILE *fp;
X{
X    int i;
X    extern char *format_hn(), *format_enet();
X
X    fprintf(fp,"station %s = addrtype\"DLC\" %s\n",
X      format_hn(h->hl_name),format_enet(h->hl_ether));
X}
X
X/*
X * sniffer_footer - dummy routine.
X */
sniffer_footer(fp)
XFILE *fp;
X{
X	return; /* no trailer for sniffer */
X}
X
X/*
X * format_enet - format ethernet number for printing.
X *
X *	Entry	ether = string of the form aa:bb:cc:dd:ee:ff:gg
X *			(leading zeros optional).
X *
X *	Exit	returns a string of the form aabbccddeeff
X *			with leading zeros added to each octet
X *			as needed.
X */
char *
format_enet(ether)
char *ether;
X{
X    int n1, n2, n3, n4, n5, n6;
X    static char buf[16];
X
X    sscanf(ether,"%x:%x:%x:%x:%x:%x:",&n1,&n2,&n3,&n4,&n5,&n6);
X    sprintf(buf,"%02x%02x%02x%02x%02x%02x",n1,n2,n3,n4,n5,n6);
X    return(buf);
X}
X
X/*
X * format_hn - format hostname for printing.
X */
char *
format_hn(host)
char *host;
X{
X    char *p;
X    int i, len;
X    static char buf[128];
X
X    memset(buf,'\0',sizeof(buf));
X    (void) strcpy(buf,"\"");	/* start with a " */
X    p = strchr(host,'.');
X    if(p != NULL)
X	len = p - &host[0];
X    else
X	len = strlen(host);
X    (void) strncat(buf,host,len);
X    (void) strcat(buf,"\"");	/* and end with a " */
X    for(i = strlen(buf); i < PADDING_SIZE; i++)
X	buf[i] = ' ';
X    return(buf);
X}
END_OF_FILE
if test 2787 -ne `wc -c <'sniffer.c'`; then
    echo shar: \"'sniffer.c'\" unpacked with wrong size!
fi
# end of 'sniffer.c'
fi
if test -f 'vendors.c' -a "${1}" != "-c" ; then 
  echo shar: Will not clobber existing file \"'vendors.c'\"
else
echo shar: Extracting \"'vendors.c'\" \(8842 characters\)
sed "s/^X//" >'vendors.c' <<'END_OF_FILE'
X#ifndef lint
static char *RCSid = "$Header: /usr/src/ecn/getethers/RCS/vendors.c,v 1.2 92/05/08 14:16:03 davy Exp $";
X#endif
X/*
X * vendors.c - print ethernet vendor codes
X *
X * Peter Shipley
X * shipley@tfs.com
X *
X * More vendors added, code rearranged by D. Curry.
X *
X * $Log:	vendors.c,v $
X * Revision 1.2  92/05/08  14:16:03  davy
X * Made portable to 4.3BSD, from Peter Shipley (shipley@tfs.com).
X * 
X * Revision 1.1  92/05/08  13:06:39  davy
X * Initial revision
X * 
X */
X#include <sys/param.h>
X#include <sys/socket.h>
X#include <netinet/in.h>
X#include <net/if.h>
X#ifdef sun
X#include <net/if_arp.h>
X#endif
X#include <netinet/if_ether.h>
X#include <stdio.h>
X#include "defs.h"
X
X#define VENDOR_NUMS_MASK	0xFFFFFF00
X#define VEN_MAX		(sizeof(Vendor_nums) / sizeof(struct _vendor_nums))
X
static struct _vendor_nums  {
X    unsigned long addr;
X    char *name;
X} Vendor_nums[] = { 
X    0x00000200, "BBN (internal)",
X    0x00000C00, "Cisco Systems",
X    0x00000E00, "Fujitsu",
X    0x00000F00, "NeXT",
X    0x00001000, "Hughes LAN Systems (Sytek)",
X    0x00001100, "Tektronix",
X    0x00001500, "Datapoint Corporation",
X    0x00001800, "Webster (?)",
X    0x00001B00, "Novell",
X    0x00001D00, "Cabletron",
X    0x00002000, "Data Industrier AB (DIAB)",
X    0x00002100, "SC&C",
X    0x00002200, "Visual Technology",
X    0x00002900, "IMC",
X    0x00002A00, "TRW",
X    0x00003C00, "Auspex",
X    0x00003D00, "AT&T",
X    0x00004400, "Castelle",
X    0x00004600, "ISC-Bunker Ramo (Olivetti)",
X    0x00004900, "Apricot, Ltd.",
X    0x00004B00, "APT AppleTalk WAN Router",
X    0x00004F00, "Logicraft (386-Ware PC Emulator)",
X    0x00005200, "ODS",
X    0x00005500, "AT&T",
X    0x00005A00, "Schneider&Koch/Syskonnect",
X    0x00005D00, "RDE",
X    0x00005E00, "US Dept. of Defense (IANA)",
X    0x00006200, "Honeywell",
X    0x00006500, "Network General",
X    0x00006900, "Silicon Graphics (?)",
X    0x00006B00, "MIPS",
X    0x00006E00, "Artisoft, Inc.",
X    0x00007700, "MIPS (?), Interphase (?)",
X    0x00007900, "Net Ware (?)",
X    0x00007A00, "Ardent",
X    0x00007B00, "Research Machines",
X    0x00007D00, "Harris (3M) (old)",
X    0x00007F00, "Linotronic",
X    0x00008000, "Imagen (?) Harris (3M) (new)?",
X    0x00008100, "Synoptics",
X    0x00008400, "Aquila (?), ADI Systems, Inc. (?)",
X    0x00008600, "Gateway (?), Megahertz Corp. (?)",
X    0x00008900, "Cayman Systems (Gatorbox)",
X    0x00008E00, "Jupiter (?), Solbourne (?)",
X    0x00009300, "Proteon",
X    0x00009400, "Asante MAC",
X    0x00009500, "Sony/Tektronix",
X    0x00009700, "Epoch",
X    0x00009800, "Cross Com",
X    0x00009F00, "Ameristar Technology",
X    0x0000A000, "Sanyo Electronics",
X    0x0000A200, "Wellfleet",
X    0x0000A300, "Network Application Technology",
X    0x0000A400, "Acorn",
X    0x0000A500, "Compatible Systems Corporation",
X    0x0000A600, "Network General (internal)",
X    0x0000A700, "Network Computing Devices (NCD) (X terminal)",
X    0x0000A800, "Stratus Computer, Inc.",
X    0x0000A900, "Network Systems Corp. (NSC)",
X    0x0000AA00, "Xerox",
X    0x0000AC00, "Apollo",
X    0x0000AF00, "Nuclear Data",
X    0x0000B000, "RAD Network Devices (RND)",
X    0x0000B300, "CIMLinc",
X    0x0000B500, "Datability",
X    0x0000B700, "Dove Fastnet",
X    0x0000BB00, "??? (AppleTalk?)",
X    0x0000BC00, "Allen-Bradley",
X    0x0000C000, "Standard Microsystems Corp. (SMC) (Western Digital)",
X    0x0000C600, "HP Intelligent Networks Operation (Eon Systems)",
X    0x0000C800, "Altos",
X    0x0000C900, "Emulex (terminal server)",
X    0x0000D000, "Develcon Electronics, Ltd.",
X    0x0000D100, "Adaptec, Inc. (Nodem)",
X    0x0000D700, "Dartmouth College (NED router)",
X    0x0000D800, "3Com? Novell? (IBM PS/2)",
X    0x0000DD00, "Gould",
X    0x0000DE00, "Unigraph",
X    0x0000E200, "Acer Counterpoint",
X    0x0000E800, "Accton Technology Corporation",
X    0x0000EE00, "Network Designers Limited (?)",
X    0x0000EF00, "Alantec",
X    0x0000F000, "Samsung",
X    0x0000F300, "Gandalf",
X    0x0000F400, "Allied Telesis, Inc.",
X    0x0000F600, "Applied Microsystems Corp. (AMC)",
X    0x0000FD00, "High Level Hardware (Orion, UK)",
X    0x00010200, "BBN (internal)",
X    0x00014300, "IEEE 802",
X    0x00016300, "National Datacomm Corporation",
X    0x00016800, "Wandel & Goltermann",
X    0x0001C800, "Thomas Conrad Corp.",
X    0x00170000, "Kabel",
X    0x00402B00, "TriGem",
X    0x0040C500, "Micom Communications Corp.",
X    0x0040C800, "Milan Technology Corp.",
X    0x00608C00, "3Com (1990 onwards)",
X    0x00800F00, "Standard Microsystems Corp. (SMC)",
X    0x00801000, "Commodore",
X    0x00801700, "PFU",
X    0x00801900, "Dayna COmmunications (Etherprint)",
X    0x00801B00, "Kodiak Technology",
X    0x00802100, "Newbridge Networks Corporation",
X    0x00802900, "Microdyne Corporation",
X    0x00802D00, "Xylogics (Annex server)",
X    0x00802E00, "Plexcom, Inc.",
X    0x00803400, "SMT-Goupil",
X    0x00803500, "Technology Works",
X    0x00805100, "ADC Fibermux",
X    0x00805200, "Network Professor",
X    0x00805C00, "Agilis (?)",
X    0x00807C00, "FiberCom",
X    0x00808700, "Okidata",
X    0x00808C00, "Frontier Software Development",
X    0x00809600, "Human Designed Systems (HDS) (X terminal)",
X    0x0080A100, "Microtest",
X    0x0080A300, "Lantronix",
X    0x0080B200, "Network Equipment Technologies",
X    0x0080C700, "Xircom, Inc.",
X    0x0080C800, "D-Link, Solectek Pocket Adapters",
X    0x0080D000, "Computer Products International",
X    0x0080D300, "Shiva (Fastpath)",
X    0x0080D400, "Chase Limited",
X    0x0080D800, "Network Perihperals",
X    0x0080F100, "Opus",
X    0x00AA0000, "Intel",
X    0x00B0D000, "Computer Products International",
X    0x00DD0000, "Ungermann-Bass (IBM RT)",
X    0x00DD0100, "Ungermann-Bass",
X    0x00EFE500, "3Com (IBM Microchannel card)",
X    0x02040600, "BBN (internal)",
X    0x02070100, "MICOM/Interlan (UNIBUS/QBUS/Apollo/Cisco)",
X    0x02606000, "3Com",
X    0x02608600, "Satelcom MegaPac (UK)",
X    0x02608C00, "3Com (IBM-PC/Imagen/Valid/Cisco/Macintosh)",
X    0x02CF1F00, "CMC (Masscomp/Silicon Graphics)",
X    0x02E6D300, "Bus-Tech, Inc. (IBM mainframes)",
X    0x08000100, "Computer Vision",
X    0x08000200, "3Com (Bridge)",
X    0x08000300, "Advanced Computer Communications (ACC)",
X    0x08000500, "Symbolics (LISP machine)",
X    0x08000700, "Apple",
X    0x08000800, "BBN",
X    0x08000900, "Hewlett-Packard",
X    0x08000A00, "Nestar Systems",
X    0x08000B00, "Unisys",
X    0x08000D00, "International Computers, Ltd. (ICL)",
X    0x08000E00, "NCR/AT&T",
X    0x08000F00, "Standard Microsystems Corporation (SMC)",
X    0x08001000, "AT&T",
X    0x08001100, "Tektronix",
X    0x08001400, "Excelan (BBN Butterfly/Masscomp/Silicon Graphics)",
X    0x08001700, "National Semiconductor (NSC)",
X    0x08001A00, "Data General",
X    0x08001B00, "Data General",
X    0x08001E00, "Apollo",
X    0x08001F00, "Sharp",
X    0x08002000, "Sun",
X    0x08002200, "NBI",
X    0x08002300, "Matsushita Denso",
X    0x08002500, "CDC",
X    0x08002600, "Norsk Data",
X    0x08002700, "PCS Computer Systems GmbH",
X    0x08002800, "TI (Explorer)",
X    0x08002B00, "Digital Equipment Corp.",
X    0x08002E00, "Metaphor",
X    0x08002F00, "Prime Computer (50-series LHC300)",
X    0x08003600, "Intergraph",
X    0x08003700, "Fujitsu-Xerox",
X    0x08003800, "Bull",
X    0x08003900, "Spider Systems",
X    0x08003B00, "Torus Systems",
X    0x08003E00, "Motorola (VME bus processor module)",
X    0x08004100, "Digital Comm. Association (DCA)",
X    0x08004400, "DAVID Systems, Inc. (DSI)",
X    0x08004500, "Xylogics (?)",
X    0x08004600, "Sony",
X    0x08004700, "Sequent",
X    0x08004900, "Univation",
X    0x08004C00, "Encore",
X    0x08004E00, "BICC",
X    0x08005100, "Experdata",
X    0x08005600, "Stanford University",
X    0x08005700, "Evans & Sutherland (?)",
X    0x08005800, "??? (DECsystem-20)",
X    0x08005A00, "IBM",
X    0x08005D00, "Gould",
X    0x08006700, "Comdesign",
X    0x08006800, "Ridge",
X    0x08006900, "Silicon Graphics",
X    0x08006A00, "ATTst (?)",
X    0x08006E00, "Excelan",
X    0x08007000, "Mitsubishi",
X    0x08007400, "Casio",
X    0x08007500, "Danish Data Elektronik A/S (DDE)",
X    0x08007700, "TSL (Retix)",
X    0x08007900, "Silicon Graphics",
X    0x08007C00, "Vitalink TransLAN III",
X    0x08008000, "XIOS",
X    0x08008100, "Crosfield Electronics",
X    0x08008300, "Seiko Denshi",
X    0x08008600, "Imagen/QMS",
X    0x08008700, "Xyplex",
X    0x08008900, "Kinetics",
X    0x08008B00, "Pyramid",
X    0x08008D00, "XyVision",
X    0x08008E00, "Tandem",
X    0x08009000, "Retix",
X};
X
char *
vendor_name(ea)
struct ether_addr *ea;
X{
X	int ether_num;
X	int i;
X
X	ether_num = 0;
X	ether_num =
X		(ea->ether_addr_octet[0] << 24 ) |
X		(ea->ether_addr_octet[1] << 16 ) |
X		(ea->ether_addr_octet[2] << 8 );
X
X	/* I should install a binary search */
X	for (i=0; i < VEN_MAX; i++) {
X		if ((ether_num & VENDOR_NUMS_MASK) ==
X		    (Vendor_nums[i].addr & VENDOR_NUMS_MASK)) {
X			return(Vendor_nums[i].name);
X		}
X	}
X
X	return("???");
X}
END_OF_FILE
if test 8842 -ne `wc -c <'vendors.c'`; then
    echo shar: \"'vendors.c'\" unpacked with wrong size!
fi
# end of 'vendors.c'
fi
echo shar: End of archive 1 \(of 1\).
cp /dev/null ark1isdone
MISSING=""
for I in 1 ; do
    if test ! -f ark${I}isdone ; then
	MISSING="${MISSING} ${I}"
    fi
done
if test "${MISSING}" = "" ; then
    echo You have the archive.
    rm -f ark[1-9]isdone
else
    echo You still need to unpack the following archives:
    echo "        " ${MISSING}
fi
##  End of shell archive.
exit 0
