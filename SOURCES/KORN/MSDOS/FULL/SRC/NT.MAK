#
# MS-DOS SHELL - Makefile
#
# MS-DOS SHELL - Copyright (c) 1994 Data Logic Limited.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice is duplicated in the
# source form.
#
#
#    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/nt.mak,v 1.1 1994/02/01 10:31:09 istewart Exp $
#
#    $Log: nt.mak,v $
# Revision 1.1  1994/02/01  10:31:09  istewart
# Initial revision
#
#
# This makefile builds one version of the shell:
#
#  Windows NT - 32 bit
#
# BETA is set to the release name (V2xxBeta) or RELEASE.
#
# You may need to uncomment one or more following, if the associated environment
# variable is to something else.
#
#BETA=RELEASE
#
# Include some standard NT make macros which reside in \mstools\h

!include <ntwin32.mak>

# fix a few crufts...
CC       = cl386 -nologo
CFLAGS   = -W3 -Iinclude
cvarsdll = $(cvars)
ldebug   = -debug:full
MAKE     = nmake -nologo -f $(MAKFILE)

MSTOOLS	 = f:\mstools
DEST	 = $(HOME)\bin
LDFLAGS	 = $(linkdebug) $(conlflags) $(ldebug)
LIBS	 = $(conlibs)
#LIBS	 = $(guilibsdll)
LINKER	 = $(link)
MAKEFILE = nt.mak

HDRS	 = sh.h

OBJS	 = sh1.obj sh2.obj sh3.obj sh4.obj sh5.obj sh6.obj \
	   sh7.obj sh8.obj sh9.obj sh10.obj sh11.obj sh12.obj sh13.obj

PROGRAM	 = shnt.exe

SRCS	 = sh1.c sh2.c sh3.c sh4.c sh5.c sh6.c sh7.c \
	   sh8.c sh9.c sh10.c sh11.c sh12.c sh13.c

all:		$(PROGRAM)

$(PROGRAM):	$(OBJS)
	$(LINKER) $(LDFLAGS) $(OBJS) -out:$(PROGRAM) $(LIBS)

optimized:
	$(MAKE) NODEBUG=1 all

clean:;		-rm -f *.obj *.exe

.c.obj:
	$(CC) $(CFLAGS) $(cdebug) $(cvarsdll) -c $<

.c.exe:
	$(CC) $(CFLAGS) $(cdebug) $(cvarsdll) -c $<
	$(LINKER) $(LDFLAGS) $*.obj -out:$*.exe $(LIBS)

sh1.obj:	sh1.c sh.h
sh2.obj:	sh2.c sh.h
sh3.obj:	sh3.c sh.h
sh4.obj:	sh4.c sh.h
sh5.obj:	sh5.c sh.h
sh6.obj:	$(SRCS) sh.h
	$(CC) $(CFLAGS) $(cdebug) $(cvarsdll) -c -DRELEASE=$(BETA) sh6.c

sh7.obj:	sh7.c sh.h
sh8.obj:	sh8.c sh.h
sh9.obj:	sh9.c sh.h
sh10.obj:	sh10.c sh.h
sh11.obj:	sh11.c sh.h
sh12.obj:	sh12.c sh.h
sh13.obj:	sh13.c sh.h

showkey: showkey.exe
showkey.exe:
	$(CC) $(CFLAGS) $(cdebug) $(cvarsdll) -c showkey.c
	$(LINKER) $(LDFLAGS) showkey.obj -out:showkey.exe $(LIBS)

director: director.exe
director.exe:
	$(CC) $(CFLAGS) $(cdebug) -DTEST $(cvarsdll) -c director.c
	$(LINKER) $(LDFLAGS) director.obj -out:director.exe $(LIBS)

glob: glob.exe
glob.exe: director.obj
	$(CC) $(CFLAGS) $(cdebug) -DTEST $(cvarsdll) -c glob.c
	$(LINKER) $(LDFLAGS) glob.obj director.obj -out:glob.exe $(LIBS)

stdargv: stdargv.exe
stdargv.exe: glob.obj director.obj
	$(CC) $(CFLAGS) $(cdebug) -DTEST $(cvarsdll) -c stdargv.c
	$(LINKER) $(LDFLAGS) stdargv.obj director.obj glob.obj -out:stdargv.exe $(LIBS)

glob.obj:	glob.c
director.obj:	director.c
stdargv.obj:	stdargv.c
