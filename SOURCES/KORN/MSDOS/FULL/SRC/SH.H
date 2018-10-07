/*
 * MS-DOS SHELL - Header File
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Charles Forsyth
 *
 * This code is based on (in part) the shell program written by Charles
 * Forsyth and the subsequence modifications made by Simon J. Gerraty (for
 * his Public Domain Korn Shell) and is subject to the following copyright
 * restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh.h,v 2.16 1994/08/25 20:58:47 istewart Exp $
 *
 *    $Log: sh.h,v $
 *	Revision 2.16  1994/08/25  20:58:47  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.15  1994/02/23  09:23:38  istewart
 *	Beta 233 updates
 *
 *	Revision 2.14  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.13  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.12  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.11  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.10  1993/07/02  10:25:53  istewart
 *	224 Beta fixes
 *
 *	Revision 2.9  1993/06/14  11:00:54  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.8  1993/06/02  09:58:12  istewart
 *	Shell 223 Beta Release - see Notes file
 *
 *	Revision 2.7  1993/02/16  16:04:22  istewart
 *	Beta 2.22 Release
 *
 *	Revision 2.6  1993/01/26  18:35:09  istewart
 *	Release 2.2 beta 0
 *
 *	Revision 2.5  1992/12/14  10:54:56  istewart
 *	BETA 215 Fixes and 2.1 Release
 *
 *	Revision 2.4  1992/11/06  10:03:44  istewart
 *	214 Beta test updates
 *
 *	Revision 2.3  1992/09/03  18:54:45  istewart
 *	Beta 213 Updates
 *
 *	Revision 2.2  1992/07/16  14:33:34  istewart
 *	Beta 212 Baseline
 *
 *	Revision 2.1  1992/07/14  08:58:59  istewart
 *	211 Beta updates
 *
 *	Revision 2.0  1992/04/13  17:39:40  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 */

/*
 * Operating System Definitions
 */

#define	OS_DOS		1			/* MSDOS		*/
#define	OS_OS2		2			/* OS/2 		*/
#define	OS_NT		3			/* Windows NT		*/
#define	OS_UNIX		4			/* A UNIX system	*/

#define	OS_16		1			/* 16-bit O/S		*/
#define	OS_32		2			/* 32-bit O/S		*/

#if defined (__OS2__)
#  define OS_TYPE	OS_OS2
#  define OS_SIZE	OS_32
#elif defined (__32BIT__) && defined (__EMX__)
#  if defined (EMX_DOS)
#    define OS_TYPE	OS_DOS
#  else
#    define OS_TYPE	OS_OS2
#  endif
#  define OS_SIZE	OS_32
#elif defined (OS2)
#  define OS_TYPE	OS_OS2
#  define OS_SIZE	OS_16
#elif defined (WIN32)
#  define OS_TYPE	OS_NT
#  define OS_SIZE	OS_32
#elif defined (__386__)
#  define OS_TYPE	OS_DOS
#  define OS_SIZE	OS_32
#elif defined (unix)
#  define OS_TYPE	OS_UNIX
#  define OS_SIZE	OS_32
#else
#  define OS_TYPE	OS_DOS
#  define OS_SIZE	OS_16
#  define OS_SWAPPING	1
#endif

#if (OS_SIZE == OS_32)
#  define F_LOCAL
#else
#  define F_LOCAL	near
#endif

/*
 * Get the system include files
 */

#if (OS_TYPE == OS_OS2)

/*
 * OS/2 Stuff.  Define the following so we get the right defs.
 */

#  define INCL_DOSDEVICES
#  define INCL_DOSERRORS
#  define INCL_DOSEXCEPTIONS
#  define INCL_DOSFILEMGR
#  define INCL_DOSMEMMGR
#  define INCL_DOSMISC
#  define INCL_DOSMODULEMGR
#  define INCL_DOSPROCESS
#  define INCL_DOSQUEUES
#  define INCL_DOSSEMAPHORES
#  define INCL_DOSSESMGR
#  define INCL_DOSSIGNALS
#  define INCL_KBD
#  define INCL_VIO
#  define INCL_WINSWITCHLIST

#  include <os2.h>
#  include <bseerr.h>

#  if (OS_SIZE == OS_32)
#    include <bsedev.h>

/* Some compilers miss the 1.x to 2.x conversion macros */

#    ifndef DosCwait
#      define DosCwait			DosWaitChild
#    endif

#    ifndef DosQCurDisk
#      define DosQCurDisk		DosQueryCurrentDisk
#    endif

#    ifndef DosQFSAttach
#      define DosQFSAttach		DosQueryFSAttach
#    endif

#    ifndef DosQFSInfo
#      define DosQFSInfo		DosQueryFSInfo
#    endif

#    ifndef DosQHandType
#      define DosQHandType		DosQueryHType
#    endif

#    ifndef DosQVerify
#      define DosQVerify		DosQueryVerify
#    endif

#    ifndef DosSelectDisk
#      define DosSelectDisk		DosSetDefaultDisk
#    endif

#    ifndef DosSetFHandState
#      define DosSetFHandState		DosSetFHState
#    endif

#    ifndef DosQFileInfo
#      define DosQFileInfo		DosQueryFileInfo
#    endif

#  endif

#elif (OS_TYPE == OS_NT)

/*
 * Windows NT Stuff.  Define the following so we don't get tons of extra stuff
 * when we include windows.h 
 */

#  define NOGDICAPMASKS     
#  define NOVIRTUALKEYCODES 
#  define NOWINMESSAGES     
#  define NOWINSTYLES       
#  define NOSYSMETRICS      
#  define NOMENUS           
#  define NOICONS           
#  define NOKEYSTATES       
#  define NOSYSCOMMANDS     
#  define NORASTEROPS       
#  define NOSHOWWINDOW      
#  define OEMRESOURCE       
#  define NOATOM            
#  define NOCLIPBOARD       
#  define NOCOLOR           
#  define NOCTLMGR          
#  define NODRAWTEXT        
#  define NOGDI             
/* #  define NOKERNEL         */
/* #  define NOUSER           */
#  define NONLS             
#  define NOMB              
#  define NOMEMMGR          
#  define NOMETAFILE        
#  define NOMINMAX          
#  define NOMSG             
#  define NOOPENFILE        
#  define NOSCROLL          
#  define NOSERVICE         
#  define NOSOUND           
#  define NOTEXTMETRIC      
#  define NOWH              
#  define NOWINOFFSETS      
#  define NOCOMM            
#  define NOKANJI           
#  define NOHELP            
#  define NOPROFILER        
#  define NODEFERWINDOWPOS  

/*
 * We're not using Microsoft's "extensions" to C for Structured Exception
 * Handling (SEH).
 */

#  undef try
#  undef except
#  undef finally
#  undef leave

#  include <windows.h>

#elif (OS_TYPE == OS_DOS)

/* Finally the DOS stuff */

#  include <dos.h>
#  if defined (__32BIT__) && defined (__EMX__)

#  else
#    include <bios.h>
#  endif

/* Add bool definitions and others for UNIX */

#elif (OS_TYPE == OS_UNIX) 
typedef pid_t 		PID;
#endif

#if defined (__EMX__) && (OS_TYPE == OS_OS2)
typedef PID 		pid_t;
#endif

#ifndef _BOOL_T_DEFINED
typedef unsigned char	bool;
#  define _BOOL_T_DEFINED
#endif

#ifndef TRUE
#  define TRUE	((bool)1)
#endif

#ifndef FALSE
#  define FALSE	((bool)0)
#endif


/*
 * Define number of signals
 */

#ifndef NSIG
#  define NSIG	10
#endif

/*
 * Flags to DosFlagProcess are missing on IBM C Set/2
 */

#ifndef PFLG_A
#  define PFLG_A	0	/* Process Flag A			*/
#endif

#ifndef PFLG_B
#  define PFLG_B	1	/* Process Flag B			*/
#endif

#ifndef PFLG_C
#  define PFLG_C	2	/* Process Flag C			*/
#endif

/*
 * DosFlagProcess codes
 */

#ifndef FLGP_SUBTREE
#  define FLGP_SUBTREE	0	/* All sub-tree processes		*/
#endif

#ifndef FLGP_PID
#  define FLGP_PID	1	/* only the process			*/
#endif

/*
 * Spawn Types
 */

#ifndef P_WAIT
#  define P_WAIT	  0
#endif

#ifndef P_NOWAIT
#  define P_NOWAIT	1
#endif

#ifndef P_OVERLAY
#  define P_OVERLAY	2
#endif

#ifndef OLD_P_OVERLAY
#  define OLD_P_OVERLAY	2
#endif

#ifndef P_NOWAITO
#  define P_NOWAITO	3
#endif

#ifndef P_DETACH
#  define P_DETACH	4
#endif

/* Wait values */

#ifndef WAIT_CHILD
#  define WAIT_CHILD		0
#endif

#ifndef WAIT_GRANDCHILD
#  define WAIT_GRANDCHILD	1
#endif
/*
 * Missing errno values
 */

#ifndef EIO
#  define EIO		105	/* I/O error				*/
#endif

#ifndef E2BIG
#  define E2BIG		107	/* Arg list too long			*/
#endif

#ifndef ENOTDIR
#  define ENOTDIR	120	/* Not a directory			*/
#endif

/*
 * Missing stat definitions
 */

#ifndef S_IFMT
#  define	S_IFMT	0xf000	/* type of file				*/
#endif

#ifndef S_IFBLK
#  define	S_IFBLK	0x6000	/* block special			*/
#endif

#ifndef S_ISDIR
#  define S_ISDIR(m)	((((m) & S_IFMT) == S_IFDIR))
#endif

#ifndef S_ISCHR
#  define S_ISCHR(m)	((((m) & S_IFMT) == S_IFCHR))
#endif

#ifndef S_ISREG
#  define S_ISREG(m)	((((m) & S_IFMT) == S_IFREG))
#endif

#ifndef S_ISBLK
#  define S_ISBLK(m)	((((m) & S_IFMT) == S_IFBLK))
#endif

/* GCC has these missing */

#ifndef min
#  define min(a,b)     		(((a) < (b)) ? (a) : (b))
#endif

#ifndef O_TEXT
#  define O_TEXT		0
#endif

#ifndef O_BINARY
#  define O_BINARY		0
#endif

#ifndef O_NOINHERIT
#  define O_NOINHERIT		0x8000
#endif

#ifndef CLOCKS_PER_SEC
#  define CLOCKS_PER_SEC	1000
#endif

#ifndef max
#  define max(a,b)     (((a) > (b)) ? (a) : (b))
#endif

/*
 * File Attributes
 */

#if defined (__TURBOC__)
#  define OS_FILE_DIRECTORY		FA_DIREC
#  define OS_FILE_HIDDEN		FA_HIDDEN
#  define OS_FILE_SYSTEM 		FA_SYSTEM
#  define OS_FILE_NORMAL		0
#  define OS_FILE_READONLY		FA_RDONLY
#  define OS_FILE_ARCHIVED		FA_ARCH

/* TurboC puts this in io.h, which we don't include */

extern int  _Cdecl			_chmod (const char *, int, ...);
#  define OS_GetFileAttributes(a,b)	(*b = _chmod ((a), 0))

#elif (OS_TYPE == OS_DOS)

#  define OS_FILE_DIRECTORY		_A_SUBDIR
#  define OS_FILE_HIDDEN		_A_HIDDEN
#  define OS_FILE_SYSTEM 		_A_SYSTEM
#  define OS_FILE_NORMAL		_A_NORMAL
#  define OS_FILE_READONLY		_A_RDONLY
#  define OS_FILE_ARCHIVED		_A_ARCH

#  ifndef _A_NORMAL
#    define _A_NORMAL	0x00		/* No attributes		*/
#  endif
#  ifndef _A_RDONLY
#    define _A_RDONLY	0x01		/* Read-only			*/
#  endif
#  ifndef _A_HIDDEN
#    define _A_HIDDEN	0x02		/* Hidden			*/
#  endif
#  ifndef _A_SYSTEM
#    define _A_SYSTEM	0x04		/* System			*/
#  endif
#  ifndef _A_VOLID
#    define _A_VOLID	0x08		/* Volume label			*/
#  endif
#  ifndef _A_SUBDIR
#    define _A_SUBDIR	0x10		/* Directory			*/
#  endif
#  ifndef _A_ARCH
#    define _A_ARCH	0x20		/* Archive			*/
#  endif
#  ifndef _A_DEVICE
#    define _A_DEVICE	0x40		/* Device			*/
#  endif

#  define OS_GetFileAttributes(a,b)	_dos_getfileattr ((a), (b))

#elif (OS_TYPE == OS_OS2)

#  define OS_FILE_DIRECTORY		FILE_DIRECTORY
#  define OS_FILE_HIDDEN		FILE_HIDDEN
#  define OS_FILE_SYSTEM		FILE_SYSTEM
#  define OS_FILE_NORMAL		FILE_NORMAL
#  define OS_FILE_READONLY		FILE_READONLY
#  define OS_FILE_ARCHIVED		FILE_ARCHIVED

#  if (OS_SIZE == OS_16)
#    define OS_GetFileAttributes(a,b)	DosQFileMode ((a), (b), 0L)
#  else
#    define OS_GetFileAttributes(a,b)	DosQFileMode ((a), (b))
#  endif

#elif (OS_TYPE == OS_NT)

#  define OS_FILE_DIRECTORY		FILE_ATTRIBUTE_DIRECTORY
#  define OS_FILE_HIDDEN		FILE_ATTRIBUTE_HIDDEN
#  define OS_FILE_SYSTEM		FILE_ATTRIBUTE_SYSTEM
#  define OS_FILE_NORMAL		0
#  define OS_FILE_READONLY		FILE_ATTRIBUTE_READONLY
#  define OS_FILE_ARCHIVED		FILE_ATTRIBUTE_ARCHIVE

#  define OS_GetFileAttributes(a,b)	DosQFileMode ((a), (b))
#endif

#define OS_FILE_ATTRIBUTES	(OS_FILE_DIRECTORY | OS_FILE_HIDDEN | \
				 OS_FILE_SYSTEM    | OS_FILE_NORMAL | \
				 OS_FILE_READONLY  | OS_FILE_ARCHIVED)

/*
 * Get major version number
 */

#if (OS_TYPE == OS_OS2)
#  define OS_VERS_N		_osmajor / 10
#else
#  define OS_VERS_N		_osmajor
#endif

extern int	BaseOS;		/* Underlying OS			*/

#define BASE_OS_DOS	0	/* DOS					*/
#define BASE_OS_WIN	1	/* Windows				*/
#define BASE_OS_OS2	2	/* OS2					*/
#define BASE_OS_NT	3	/* Windows NT				*/
#define BASE_OS_UNIX	4	/* UNIX					*/

/*
 * Watcom does something funny in 386 mode with registers and int86
 */

#if (OS_TYPE == OS_DOS)
#  if defined (__WATCOMC__) && defined(__386__) && !defined(__WINDOWS_386__)
#    define REG_AX				eax
#    define REG_BX				ebx
#    define REG_CX				ecx
#    define REG_DX				edx
#    define REG_SI				esi
#    define REG_DI				edi
#    define REG_CFLAGS				cflag
#    define SystemInterrupt(a,b,c)		int386 (a, b, c)
#    define SystemExtendedInterrupt(a,b,c,d)	int386x (a, b, c, d)
#    define DosInterrupt(a,b)			intdos (a, b)
#    define DosExtendedInterrupt(a,b,c)		intdosx (a, b, c)
#  elif defined (__EMX__)
#    define REG_AX				ax
#    define REG_BX				bx
#    define REG_CX				cx
#    define REG_DX				dx
#    define REG_SI				si
#    define REG_DI				di
#    define REG_CFLAGS				flags
#    define SystemInterrupt(a,b,c)		_int86 (a, b, c)
#    define SystemExtendedInterrupt(a,b,c,d)	not available
#    define DosInterrupt(a,b)			_int86 (0x21, a, b)
#    define DosExtendedInterrupt(a,b,c)		intdosx (a, b, c)
#  else
#    define REG_AX				ax
#    define REG_BX				bx
#    define REG_CX				cx
#    define REG_DX				dx
#    define REG_SI				si
#    define REG_DI				di
#    define REG_CFLAGS				cflag
#    define SystemInterrupt(a,b,c)		int86 (a, b, c)
#    define SystemExtendedInterrupt(a,b,c,d)	int86x (a, b, c, d)
#    define DosInterrupt(a,b)			intdos (a, b)
#    define DosExtendedInterrupt(a,b,c)		intdosx (a, b, c)
#  endif
#endif

/*
 * Missing OS/2 1.x definitions.  Map OS/2 2.x definitions as appropriate
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_16)
#  define SSF_RELATED_INDEPENDENT	0	/* New session is an	*/
					/* independent			*/
					/* session (not related)	*/
#  define SSF_RELATED_CHILD	1	/* New session is a child	*/
					/* session (related)		*/
#  define SSF_FGBG_FORE		0	/* Start session in foreground	*/
#  define SSF_FGBG_BACK		1	/* Start session in background	*/
#  define SSF_TRACEOPT_NONE	0	/* No trace			*/
#  define SSF_TRACEOPT_TRACE	1	/* Trace with no notification	*/
					/* of descendants		*/
#  define SSF_TRACEOPT_TRACEALL	2	/* Trace all descendant sessions */
#  define SSF_INHERTOPT_SHELL	0	/* Inherit the Shell's environment.*/
#  define SSF_INHERTOPT_PARENT	1	/* Inherit the environment of the*/
					/* program issuing the		*/
					/* DosStartSession call.	*/
#  define SSF_TYPE_DEFAULT	0	/* Use the PgmHandle data, or	*/
					/* allow the Shell to establish	*/
					/* the session type.		*/
#  define SSF_TYPE_FULLSCREEN	1	/* Start the program in a	*/
					/* full-screen session.		*/
#  define SSF_TYPE_WINDOWABLEVIO	2	/* Start the program	*/
					/* in a	windowed session for	*/
					/* programs using the Base Video*/
					/* Subsystem			*/
#  define SSF_TYPE_PM		3	/* Start the program in a	*/
					/* windowed session for programs*/
					/* using the Presentation Manager*/
					/* services			*/
#  define SSF_TYPE_VDM		4	/* Start the program in a	*/
					/* full-screen DOS session.	*/
#  define SSF_TYPE_WINDOWEDVDM	7	/* Start the program in a	*/
					/* windowed DOS session.	*/
#  define SSF_CONTROL_VISIBLE	0x0000	/* Visible			*/
#  define SSF_CONTROL_INVISIBLE	0x0001	/* Invisible			*/
#  define SSF_CONTROL_MAXIMIZE	0x0002	/* Maximize			*/
#  define SSF_CONTROL_MINIMIZE	0x0004	/* Minimize			*/
#  define SSF_CONTROL_NOAUTOCLOSE	0x0008	/* No Auto Close	*/
#  define SSF_CONTROL_SETPOS	0x8000	/* Use specified size and position */
#endif

/*
 * Define common OS2 APIs declaractions
 */

#if (OS_TYPE == OS_OS2)
#  if (OS_SIZE == OS_16)
#    define OSCALL_RET		USHORT
#    define OSCALL_PARAM	USHORT
#  else
#    define OSCALL_RET		APIRET
#    define OSCALL_PARAM	ULONG
#  endif
#elif (OS_TYPE == OS_DOS)
#  define OSCALL_RET		unsigned int
#  define OSCALL_PARAM		unsigned int
#elif (OS_TYPE == OS_NT)
#  define OSCALL_RET		DWORD
#  define OSCALL_PARAM		DWORD
#elif (OS_TYPE == OS_UNIX)
#  define OSCALL_RET		int
#  define OSCALL_PARAM		int
#endif

/*
 * Borland TurboC has this in a file we don't include
 */

#ifdef __TURBOC__
#  define getpid()	(_psp)
#endif

/*
 * UNIX doesn't have these functions
 */

#if (OS_TYPE == OS_UNIX)
extern char			*strlwr (char *);
extern char			*strupr (char *);
extern int			stricmp (char *, char *);
#  define fputchar(c)		putchar (c)
#endif

/*
 * Ignore case compare.  UNIX cares about case!
 */

#if (OS_TYPE == OS_UNIX)
#  define NOCASE_COMPARE(a,b)	strcmp (a,b)
#else
#  define NOCASE_COMPARE(a,b)	stricmp (a,b)
#endif

/*
 * Executable Head Information structures
 *
 * Variable signatures
 */

#define SIG_DOS		0x5a4d		/* Dos Magic number		*/
#define SIG_OS2_16	0x454e		/* OS/2 16 Magic number		*/
#define SIG_OS2_16LE	0x454c		/* OS/2 16 Magic number		*/
#define SIG_OS2_32	0x584c		/* OS/2 32 Magic number		*/
#define SIG_NT		0x00004550	/* NT Magic number		*/

/*
 * Standard DOS header
 */

#pragma pack (1)

struct ExecDosHeader {
    unsigned short	e_magic;	/* Magic number			*/
    unsigned short	e_cblp;		/* Bytes on last page of file	*/
    unsigned short	e_cp;		/* Pages in file		*/
    unsigned short	e_crlc;		/* Relocations			*/
    unsigned short	e_cparhdr;	/* Size of header in paragraphs */
    unsigned short	e_minalloc;	/* Minimum extra paras needed	*/
    unsigned short	e_maxalloc;	/* Maximum extra paras needed	*/
    unsigned short	e_ss;		/* Initial (relative) SS value	*/
    unsigned short	e_sp;		/* Initial SP value		*/
    unsigned short	e_csum;		/* Checksum			*/
    unsigned short	e_ip;		/* Initial IP value		*/
    unsigned short	e_cs;		/* Initial (relative) CS value	*/
    unsigned short	e_lfarlc;	/* File addr. of reloc. table	*/
    unsigned short	e_ovno;		/* Overlay number		*/
    unsigned short	e_res[4];	/* Reserved words		*/
    unsigned short	e_oemid;	/* OEM identifier		*/
    unsigned short	e_oeminfo;	/* OEM information		*/
    unsigned short	e_res2[10];	/* Reserved words		*/
    long		e_lfanew;	/* File addr of new exe header	*/
};

/*
 * OS/2 and Windows 16 Bit header
 */

struct ExecOS2_16Header {
    unsigned short	ne_magic;	/* Magic number			*/
    unsigned char	ne_ver;		/* Version number		*/
    unsigned char	ne_rev;		/* Revision number		*/
    unsigned short	ne_enttab;	/* Offset of Entry Table	*/
    unsigned short	ne_cbenttab;	/* Size of Entry Table		*/
    long		ne_crc;		/* Checksum of whole file	*/
    unsigned short	ne_flags;	/* Flag word			*/
    unsigned short	ne_autodata;	/* Automatic data segment number*/
    unsigned short	ne_heap;	/* Initial heap allocation	*/
    unsigned short	ne_stack;	/* Initial stack allocation	*/
    long		ne_csip;	/* Initial CS:IP setting	*/
    long		ne_sssp;	/* Initial SS:SP setting	*/
    unsigned short	ne_cseg;	/* Count of file segments	*/
    unsigned short	ne_cmod;	/* Entries in Module Ref. Table	*/
    unsigned short	ne_cbnrestab;	/* Size non-resident name table	*/
    unsigned short	ne_segtab;	/* Off. Segment Table		*/
    unsigned short	ne_rsrctab;	/* Off. Resource Table		*/
    unsigned short	ne_restab;	/* Off. resident name table	*/
    unsigned short	ne_modtab;	/* Off. Module Reference Table	*/
    unsigned short	ne_imptab;	/* Off. Imported Names Table	*/
    long		ne_nrestab;	/* Off. Non-resident Names Table*/
    unsigned short	ne_cmovent;	/* Count of movable entries	*/
    unsigned short	ne_align;	/* Segment alignment shift count*/
    unsigned short	ne_cres;	/* Count of resource segments	*/
    unsigned char	ne_exetyp;	/* Target Operating system	*/
    unsigned char	ne_flagsothers;	/* Other .EXE flags		*/
    unsigned short	ne_pretthunks;	/* offset to return thunks	*/
    unsigned short	ne_psegrefbytes;/* offset to segment ref. bytes	*/
    unsigned short	ne_swaparea;	/* Minimum code swap area size	*/
    unsigned short	ne_expver;	/* Expected Windows version number */
};

/*
 * Selected ne_flags values
 */

#define OS2_16_NOTP		0x8000	/* Not a process		*/
#define OS2_16_IERR		0x2000	/* Errors in image		*/
#define OS2_16_BOUND		0x0800	/* Bound Family/API		*/
#define OS2_16_APPTYP		0x0700	/* Application type mask	*/
#define OS2_16_NOTWINCOMPAT	0x0100	/* Not compatible with P.M. Windowing */
#define OS2_16_WINCOMPAT	0x0200	/* Compatible with P.M. Windowing */
#define OS2_16_WINAPI		0x0300	/* Uses P.M. Windowing API	*/

/*
 * ne_exetyp values
 */

#define OS2_16_UNKNOWN		0	/* Unknown (any "new-format" OS) */
#define OS2_16_OS2		1	/* OS/2 (default)		*/
#define OS2_16_WINDOWS		2	/* Windows			*/
#define OS2_16_DOS4		3	/* DOS 4.x			*/
#define OS2_16_DEV386		4	/* Windows 386			*/

/*
 * OS2 32bit header
 */

struct ExecOS2_32header {
    unsigned short	e32_magic;	/* Magic number OS2__MAGIC	*/
    unsigned char	e32_border;	/* Byte ordering for the .EXE	*/
    unsigned char	e32_worder;	/* Word ordering for the .EXE	*/
    unsigned long	e32_level;	/* EXE format level for now = 0	*/
    unsigned short	e32_cpu;	/* CPU type			*/
    unsigned short	e32_os;		/* OS type			*/
    unsigned long	e32_ver;	/* Module version		*/
    unsigned long	e32_mflags;	/* Module flags			*/
    unsigned long	e32_mpages;	/* Module # pages		*/
    unsigned long	e32_startobj;	/* Object # for IP		*/
    unsigned long	e32_eip;	/* Extended IP			*/
    unsigned long	e32_stackobj;	/* Object # for SS		*/
    unsigned long	e32_esp;	/* Extended SS			*/
    unsigned long	e32_pagesize;	/* .EXE page size		*/
    unsigned long	e32_pageshift;	/* Page alignment shift in .EXE	*/
    unsigned long	e32_fixupsize;	/* Fixup section size		*/
    unsigned long	e32_fixupsum;	/* Fixup section checksum	*/
    unsigned long	e32_ldrsize;	/* Loader section size		*/
    unsigned long	e32_ldrsum;	/* Loader section checksum	*/
    unsigned long	e32_objtab;	/* Object table off.		*/
    unsigned long	e32_objcnt;	/* Number of objects in module	*/
    unsigned long	e32_objmap;	/* Object page map off.		*/
    unsigned long	e32_itermap;	/* Object iterated data map off. */
    unsigned long	e32_rsrctab;	/* Off. Resource Table		*/
    unsigned long	e32_rsrccnt;	/* Number of resource entries	*/
    unsigned long	e32_restab;	/* Off. resident name table	*/
    unsigned long	e32_enttab;	/* Off. Entry Table		*/
    unsigned long	e32_dirtab;	/* Off. Module Directive Table	*/
    unsigned long	e32_dircnt;	/* Number of module directives	*/
    unsigned long	e32_fpagetab;	/* Off. Fixup Page Table	*/
    unsigned long	e32_frectab;	/* Off. Fixup Record Table	*/
    unsigned long	e32_impmod;	/* Off. Import Module Name Table */
    unsigned long	e32_impmodcnt;	/* # entries in IM Name Table	*/
    unsigned long	e32_impproc;	/* Off. IProcedure Name Table	*/
    unsigned long	e32_pagesum;	/* Off. Per-Page Checksum Table	*/
    unsigned long	e32_datapage;	/* Off. Enumerated Data Pages	*/
    unsigned long	e32_preload;	/* Number of preload pages	*/
    unsigned long	e32_nrestab;	/* Off. Non-resident Names Table */
    unsigned long	e32_cbnrestab;	/* Size of Non-resident Name Table */
    unsigned long	e32_nressum;	/* Non-resident Name Table Checksum */
    unsigned long	e32_autodata;	/* Object # for automatic data object */
    unsigned long	e32_debuginfo;	/* Off. the debugging information */
    unsigned long	e32_debuglen;	/* Length of debugging info.	*/
    unsigned long	e32_instpreload;/* # instance pages in preload	*/
    					/* section of .EXE file		*/
    unsigned long	e32_instdemand;	/* # instance pages in demand	*/
    					/* load section of .EXE file	*/
    unsigned long	e32_heapsize;	/* Size of heap - for 16-bit apps */
/* Padding ignored */
};

/*
 * Format of e32_mflags
 */

#define OS2_NOTP	0x00008000L	/* Library Module - used as NENOTP */
#define OS2_NOLOAD	0x00002000L	/* Module not Loadable		*/
#define OS2_PMAPI	0x00000300L	/* Uses PM Windowing API	*/
#define OS2_PMW		0x00000200L	/* Compatible with PM Windowing */
#define OS2_NOPMW	0x00000100L	/* Incompatible with PM Windowing */
#define OS2_APPMASK	0x00000700L	/* Application Type Mask	*/
#define OS2_PROTDLL	0x00010000L	/* Protected memory library module */
#define OS2_DEVICE	0x00020000L	/* Device driver		*/
#define OS2_MODEXE	0x00000000L	/* .EXE module			*/
#define OS2_MODDLL	0x00008000L	/* .DLL module			*/
#define OS2_MODPROTDLL	0x00018000L	/* Protected memory library module */
#define OS2_MODPDEV	0x00020000L	/* Physical device driver	*/
#define OS2_MODVDEV	0x00028000L	/* Virtual device driver	*/
#define OS2_MODMASK	0x00038000L	/* Module type mask		*/

/*
 * NT Exec Header
 */

struct ExecNTHeader {
    unsigned long		Signature;

    struct NTFileHeader {
	unsigned short		Machine;
	unsigned short		NumberOfSections;
	unsigned long		TimeDateStamp;
	unsigned long		PointerToSymbolTable;
	unsigned long		NumberOfSymbols;
	unsigned short		SizeOfOptionalHeader;
	unsigned short		Characteristics;
    }				FileHeader;

    struct NTOptionalHeader {
	unsigned short		Magic;
	unsigned char		MajorLinkerVersion;
	unsigned char		MinorLinkerVersion;
	unsigned long		SizeOfCode;
	unsigned long		SizeOfInitializedData;
	unsigned long		SizeOfUninitializedData;
	unsigned long		AddressOfEntryPoint;
	unsigned long		BaseOfCode;
	unsigned long		BaseOfData;
	unsigned long		ImageBase;
	unsigned long		SectionAlignment;
	unsigned long		FileAlignment;
	unsigned short		MajorOperatingSystemVersion;
	unsigned short		MinorOperatingSystemVersion;
	unsigned short		MajorImageVersion;
	unsigned short		MinorImageVersion;
	unsigned short		MajorSubsystemVersion;
	unsigned short		MinorSubsystemVersion;
	unsigned long		Reserved1;
	unsigned long		SizeOfImage;
	unsigned long		SizeOfHeaders;
	unsigned long		CheckSum;
	unsigned short		Subsystem;
	unsigned short		DllCharacteristics;
	unsigned long		SizeOfStackReserve;
	unsigned long		SizeOfStackCommit;
	unsigned long		SizeOfHeapReserve;
	unsigned long		SizeOfHeapCommit;
	unsigned long		LoaderFlags;
	unsigned long		NumberOfRvaAndSizes;
/* All entries after this left out */
    }				OptionalHeader;
};

#pragma pack ()

/*
 * NT header definitions
 */

#define NT_STD_HEADER		28	/* Size of Standard header	*/
#define NT_OPTIONAL_HEADER	224	/* Size of Optional header	*/

#define NT_FILE_DLL		0x2000		/* File is a DLL.	*/
#define NT_FILE_MACHINE_I386	0x14c		/* Intel 386.		*/

/* Subsystem Values */

#define NT_SS_NATIVE		1	/* doesn't require a subsystem.	*/
#define NT_SS_WINDOWS_GUI	2	/* Windows GUI subsystem.	*/
#define NT_SS_WINDOWS_CUI	3	/* Windows character subsystem.	*/
#define NT_SS_OS2_CUI		5	/* OS/2 character subsystem.	*/
#define NT_SS_POSIX_CUI		7	/* Posix character subsystem.	*/

/*
 * Find the application type
 */

extern unsigned long		QueryApplicationType (const char *);

/* Result from above */

#define EXETYPE_ERROR		0x00000f	/* Error on program	*/
#define EXETYPE_DOS		0x0000f0	/* Dos program		*/
#define EXETYPE_OS2		0x000f00	/* OS/2 16 program	*/
#define EXETYPE_OS2_TYPE	0x000700	/* OS/2 Type program	*/
#define EXETYPE_OS2_32		0x000800	/* OS/2 32 program	*/
#define EXETYPE_NT		0x00f000	/* Win NT program	*/
#define EXETYPE_UNIX		0x0f0000	/* UNIX program		*/

#define EXETYPE_UNKNOWN		0x000001	/* Not known		*/
#define EXETYPE_BAD_IMAGE	0x000002	/* Bad image 	 	*/
#define EXETYPE_NOT_EXE		0x000003	/* Not exe - OS/2	*/
#define EXETYPE_BAD_FILE	0x000004	/* File not found	*/

#define EXETYPE_DOS_CUI		0x000010	/* Dos non windows	*/
#define EXETYPE_DOS_GUI		0x000020	/* Windows		*/
#define EXETYPE_DOS_32		0x000030	/* Watcom 32 bit	*/
#define EXETYPE_DOS_BOUND	0x000040	/* OS/2 Bound 		*/

#define EXETYPE_OS2_CUI		0x000100	/* Not windows compat	*/
#define EXETYPE_OS2_CGUI	0x000200	/* Windows compatible	*/
#define EXETYPE_OS2_GUI		0x000300	/* Uses PM		*/

#define EXETYPE_NT_NATIVE	0x001000	/* NT native		*/
#define EXETYPE_NT_WINDOWS_GUI	0x002000	/* NT Windows GUI ss	*/
#define EXETYPE_NT_WINDOWS_CUI	0x003000	/* NT Windows CUI ss	*/
#define EXETYPE_NT_OS2		0x004000	/* NT OS2 subsystem	*/
#define EXETYPE_NT_POSIX	0x005000	/* NT POSIX subsystem	*/

#define EXETYPE_UNIX_NATIVE	0x010000	/* UNIX program		*/

/*
 * Shell Definitions
 */

#define PATCHLEVEL		4
#define	LINE_MAX		1000	/* Command line length		*/
#define HISTORY_MAX		100	/* History array length		*/
					/* Space for full file name	*/
#define FFNAME_MAX		(PATH_MAX + NAME_MAX + 4)
#define DOS_CMD_LINE_MAX	127	/* Max command line length	*/
#define ARRAY_SIZE(a)		((sizeof (a)) / sizeof (a[0]))

#if (OS_TYPE == OS_DOS)
#  define CMD_LINE_MAX		DOS_CMD_LINE_MAX
#elif (OS_TYPE == OS_OS2)
#  define CMD_LINE_MAX		32000	/* Max command line length	*/
#elif (OS_TYPE == OS_NT)
#  define CMD_LINE_MAX		1024	/* Max command line length	*/
#endif


#define SSAVE_IO_SIZE		4	/* Save IO array malloc inc	*/
#define LEN_DEVICE_NAME_HEADER	5	/* /dev/ string length		*/

extern int	MaxNumberofFDs;		/* Max no of file descriptors	*/
#define	NUFILE			10	/* # of user-accessible files	*/
#define	FDBASE			10	/* First file usable by Shell	*/

/*
 * Some characters
 */

#define CHAR_UNIX_DIRECTORY	'/'
#define CHAR_NEW_LINE		'\n'
#define CHAR_SINGLE_QUOTE	'\''
#define CHAR_DOUBLE_QUOTE	'"'
#define CHAR_BACKQUOTE		'`'
#define CHAR_RETURN		'\r'
#define CHAR_SPACE		' '
#define	CHAR_NOT		'^'
#define	CHAR_TAB		'\t'
#define	CHAR_BACKSPACE		'\b'
#define CHAR_ESCAPE		0x1b
#define	CHAR_XOR		'^'
#define	CHAR_BEGIN_LINE		'^'
#define CHAR_OPEN_PARATHENSIS	'('
#define CHAR_CLOSE_PARATHENSIS	')'
#define CHAR_OPEN_BRACES	'{'
#define CHAR_CLOSE_BRACES	'}'
#define CHAR_OPEN_BRACKETS	'['
#define CHAR_CLOSE_BRACKETS	']'
#define CHAR_TILDE		'~'
#define CHAR_PIPE		'|'
#define CHAR_HISTORY		'!'
#define CHAR_COMMENT		'#'
#define CHAR_MATCH_START	'#'
#define CHAR_VARIABLE		'$'
#define CHAR_END_LINE		'$'
#define CHAR_FORMAT		'%'
#define CHAR_JOBID		'%'
#define CHAR_MATCH_END		'%'
#define CHAR_ASYNC		'&'
#define CHAR_MATCH_ALL		'*'
#define CHAR_PLUS		'+'
#define CHAR_HYPHEN		'-'
#define CHAR_MATCH_RANGE	'-'
#define CHAR_CLOSE_FD		'-'
#define CHAR_SWITCH		'-'
#define CHAR_PERIOD		'.'
#define CHAR_COLON		':'
#define CHAR_DRIVE		':'
#define CHAR_SEPARATOR		';'

#if (OS_TYPE == OS_UNIX)
#  define CHAR_PATH_SEPARATOR	':'
#else
#  define CHAR_PATH_SEPARATOR	';'
#endif

#define CHAR_INPUT		'<'
#define CHAR_ASSIGN		'='
#define CHAR_OUTPUT		'>'
#define CHAR_MATCH_ANY		'?'
#define CHAR_INDIRECT		'@'
#define CHAR_META		'\\'
#define CHAR_DOS_PATH		'\\'
#define CHAR_MAGIC		0x80

/*
 * fast character classes
 */

#define	C_ALPHA		0x001		/* a-z_A-Z			*/
#define	C_DIGIT		0x002		/* 0-9				*/
#define	C_LEX1		0x004		/* \0 \t\n|&;<>()		*/
#define	C_VAR1		0x008		/* *@#!$-?			*/
#define	C_SUBOP		0x010		/* "=-+?#%"			*/
#define	C_IFS		0x020		/* $IFS				*/
#define	C_WILD		0x040		/* Wildcards			*/
#define	C_SEMICOLON	0x080		/* Semi-colon (;)		*/

extern void		InitialiseCharacterTypes (void);
extern void		SetCharacterTypes (char *, int);
extern unsigned char	CharTypes [UCHAR_MAX + 1];

/*
 * Check for variable characters
 */

#define	IS_VariableFC(c)	(CharTypes[(c)] & C_ALPHA)
#define	IS_VariableSC(c)	(CharTypes[(c)] & (C_ALPHA | C_DIGIT))

#define	IS_Numeric(c)		(CharTypes[(c)] & C_DIGIT)
#define	IS_AlphaNumeric(c)	(CharTypes[(c)] & (C_ALPHA | C_DIGIT))
#define	IS_IFS(c)		(CharTypes[(c)] & C_IFS)
#define	IS_Lexical(c)		(CharTypes[(c)] & C_LEX1)
#define	IS_VarNumeric(c)	(CharTypes[(c)] & (C_DIGIT | C_VAR1))
#define	IS_VarOp(c)		(CharTypes[(c)] & C_SUBOP)
#define	IS_WildCard(c)		(CharTypes[(c)] & C_WILD)

extern char			*SkipToWhiteSpace (char *);

/*
 * File open modes
 */
				/* Open in create mode			*/
#define O_CMASK		(O_WRONLY | O_CREAT | O_TRUNC | O_BINARY)
				/* Open in create mode for a pipe	*/
#define O_PMASK		(O_RDWR | O_CREAT | O_TRUNC | O_TEXT)
				/* Open in create mode for swap file	*/
#define O_SMASK		(O_RDWR | O_CREAT | O_TRUNC | O_BINARY)
#define O_SaMASK	(O_RDWR | O_BINARY)
#define O_RMASK		(O_RDONLY | O_NOINHERIT | O_TEXT)

/*
 * Path format conversion
 */

#define PATH_TO_UNIX(x)	ConvertPathToFormat ((x), CHAR_DOS_PATH,	\
						  CHAR_UNIX_DIRECTORY)
#define PATH_TO_DOS(x)	ConvertPathToFormat ((x), CHAR_UNIX_DIRECTORY,	\
						  CHAR_DOS_PATH)

#if (OS_TYPE == OS_UNIX)
#  define PATH_TO_UPPER_CASE(a)	
#  define PATH_TO_LOWER_CASE(a)
#  define IsHPFSFileSystem(a)	TRUE
#elif (OS_TYPE != OS_DOS)
#  define PATH_TO_UPPER_CASE(a)	{if (!IsHPFSFileSystem (a)) strupr (a); }
#  define PATH_TO_LOWER_CASE(a)	{if (!IsHPFSFileSystem (a)) strlwr (a); }
#else
#  define PATH_TO_UPPER_CASE(a)	{ strupr (a); }
#  define PATH_TO_LOWER_CASE(a)	{ strlwr (a); }
#  define IsHPFSFileSystem(a)	FALSE
#endif

/*
 * Drive support
 */

#if (OS_TYPE == OS_UNIX)
#  define IsDriveCharacter(a)		FALSE
#else
#  define IsDriveCharacter(a)		C2bool ((a) == CHAR_DRIVE)
#endif

#define IsPathCharacter(a)		C2bool ((a) == CHAR_UNIX_DIRECTORY)
#define FindLastPathCharacter(a)	strrchr (a, CHAR_UNIX_DIRECTORY)
#define FindPathCharacter(a)		strchr (a, CHAR_UNIX_DIRECTORY)

/*
 * shell components
 */

#define	NOBLOCK		((C_Op *)NULL)
#define	NOWORD		((char *)NULL)
#define	NOWORDS		((char **)NULL)
#define	NOPIPE		(-1)

/*
 * Ignore Variables flags
 */

#define DISABLE_MAILCHECK	0x0001
#define DISABLE_OPTARG		0x0002
#define DISABLE_OPTIND		0x0004
#define DISABLE_SECONDS		0x0008
#define DISABLE_RANDOM		0x0010
#define DISABLE_LASTWORD	0x0020
#define DISABLE_LINECOUNT	0x0040
#define DISABLE_WINTITLE	0x0080

extern int	DisabledVariables;

/*
 * File Descriptor Types and Macros
 */

#define DESCRIPTOR_UNKNOWN	0x0000		/* Error - not known	*/
#define DESCRIPTOR_PIPE		0x0001		/* Pipe			*/
#define DESCRIPTOR_FILE		0x0002		/* File			*/
#define DESCRIPTOR_DEVICE	0x0004		/* Device - tty		*/
#define DESCRIPTOR_CONSOLE	0x0008		/* Console device	*/

#define IS_Pipe(a)		(GetDescriptorType (a) & DESCRIPTOR_PIPE)
#define IS_File(a)		(GetDescriptorType (a) & DESCRIPTOR_FILE)
#define IS_TTY(a)		(GetDescriptorType (a) & \
				    (DESCRIPTOR_DEVICE | DESCRIPTOR_CONSOLE))
#define IS_Console(a)		(GetDescriptorType (a) & DESCRIPTOR_CONSOLE)

/*
 * Result from FindLocationOfExecutable
 */

#define EXTENSION_NOT_FOUND	0	/* Cannot find file		*/
#define EXTENSION_EXECUTABLE	1	/* OS/2 or DOS .exe or .com	*/
#define EXTENSION_BATCH		2	/* OS/2 or DOS .cmd or .bat	*/
#define EXTENSION_SHELL_SCRIPT	3	/* Shell script			*/
#define EXTENSION_OTHER		4	/* Other			*/

/*
 * XString - Expandable strings
 *
 * XString functions:
 *
 * XFree   - Release the string
 * XStart  - Get start of string
 * XClose  - close a string
 * XCreate - Create a string
 * XCheck  - Check for overflow
 */

typedef struct XString {
    unsigned char	*SStart;	/* End of string		*/
    unsigned char	*SEnd;		/* Beginning of string		*/
    size_t		SLength;	/* length			*/
} XString;

#define	XFree(xs)		ReleaseMemoryCell ((void*)(xs).SStart)
#define	XStart(xs)		((xs).SStart)
#define	XCurrentOffset(xs, xp)	(xp - (xs).SStart)
#define	XResetOffset(xs, n)	((xs).SStart + (n))

extern void	 		XCheck (XString *, unsigned char **);
extern char			*XClose (XString *, unsigned char *);
extern char			*XCreate (XString *, size_t);

/*
 * Description of a command or an operation on commands.
 * Might eventually use a union.
 */

typedef struct op {
    int			type;		/* operation type, see below	*/
    char		**args;		/* arguments to a command	*/
    char		**vars;		/* variable assignments		*/
    struct ioword	**ioact;	/* IO actions (eg, < > >>)	*/
    struct op		*left;
    struct op		*right;
    char		*str;		/* identifier for case and for	*/
} C_Op;

/*
 * C_Op.type values
 */

#define	TEOF		0
#define	TCOM		1	/* command				*/
#define	TPAREN		2	/* (c-list)				*/
#define	TPIPE		3	/* a | b				*/
#define	TLIST		4	/* a [&;] b				*/
#define	TOR		5	/* ||					*/
#define	TAND		6	/* &&					*/
#define	TFOR		7	/* FOR					*/
#define	TCOPROCESS	8	/* coprocess				*/
#define	TCASE		9	/* CASE					*/
#define	TIF		10	/* IF					*/
#define	TWHILE		11	/* WHILE				*/
#define	TUNTIL		12	/* UNTIL				*/
#define	TELIF		13	/* ELSE IF				*/
#define	TPAT		14	/* pattern in case			*/
#define	TBRACE		15	/* {c-list}				*/
#define	TASYNC		16	/* c &					*/
#define	TFUNC		17	/* c () {c-list}			*/
#define	TSELECT		18	/* SELECT				*/
#define	TTIME		19	/* time pipeline			*/

/*
 * Prefix codes for words in command tree
 */

#define	WORD_EOS	0	/* end of string			*/
#define	WORD_CHAR	1	/* unquoted character			*/
#define	WORD_QCHAR	2	/* quoted character			*/
#define	WORD_QTCHAR	3	/* temporary quoted character		*/
#define	WORD_COMSUB	4	/* $() substitution (0 terminated)	*/
#define	WORD_OQUOTE	5	/* opening '				*/
#define	WORD_CQUOTE	6	/* closing '				*/
#define	WORD_ODQUOTE	7	/* opening " 				*/
#define	WORD_CDQUOTE	8	/* closing "				*/
#define	WORD_OSUBST	9	/* opening ${ substitution		*/
#define	WORD_CSUBST	10	/* closing } of above			*/
#define	WORD_OMATHS	11	/* opening $(()) substitution (0 term)	*/
#define WORD_OARRAY	12	/* opening ${name[ of array		*/
#define WORD_CARRAY	13	/* closing ] for above			*/

/*
 * Syntax and Lexical Analysis
 *
 * Lexical tokens
 */

#define PARSE_WORD		256
#define PARSE_LOGICAL_AND	257	/* && */
#define PARSE_LOGICAL_OR	258	/* || */
#define PARSE_BREAK		259
#define PARSE_IF		260
#define PARSE_THEN		261
#define PARSE_ELSE		262
#define PARSE_ELIF		263
#define PARSE_FI		264
#define PARSE_CASE		265
#define PARSE_ESAC		266
#define PARSE_FOR		267
#define PARSE_WHILE		268
#define PARSE_UNTIL		269
#define PARSE_DO		270
#define PARSE_DONE		271
#define PARSE_IN		272
#define PARSE_SELECT		273
#define PARSE_FUNCTION		274
#define	PARSE_TIME		275
#define	PARSE_REDIR		276		/* >, <, etc */
#define	PARSE_MPAREN		277		/* () */
#define	PARSE_MDPAREN		278		/* (( )) */
#define	PARSE_TEST		279		/* [[ ]] */
#define	PARSE_COPROCESS		280		/* |& */
#define YYERRCODE		300

/*
 * Lexical token value
 */

typedef union {
    int			i;			/* Integer		*/
    char		*cp;			/* String		*/
    char		**wp;			/* List			*/
    struct op		*o;			/* Command tree		*/
    struct ioword	*iop;			/* IO action		*/
} YYSTYPE;

/*
 * flags to ScanNextToken
 */

#define	ALLOW_CONTINUATION	0x0001	/* skip new lines to complete	*/
					/* command			*/
#define	ONEWORD			0x0002	/* single word for substitute()	*/
#define	ALLOW_ALIAS		0x0004	/* recognize alias		*/
#define	ALLOW_KEYWORD		0x0008	/* recognize keywords		*/
#define	MATHS_EXPRESSION	0x0010	/* get expression inside (( ))	*/
#define	TEST_EXPRESSION		0x0020	/* get expression inside [[ ]]	*/


#define	IDENT			64	/* Max size of an Identifier	*/
extern char			CurrentLexIdentifier [IDENT+1];

/*
 * Input descriptor for yylex
 */

typedef struct source {
    char		*str;		/* input pointe			*/
    int			type;		/* input type			*/
    union {
	char			**strv;		/* string []		*/
	FILE			*file;		/* file			*/
	struct AliasList	*Calias;	/* alias		*/
    }			u;

    int			line;		/* line number			*/
    char		*file;		/* input file name		*/
    bool		echo;		/* echo input to shlout		*/
    struct source	*next;		/* stacked source		*/
} Source;

/* Source.type values */

#define	SEOF		0	/* input EOF				*/
#define	STTY		1	/* terminal input			*/
#define	SFILE		2	/* file input				*/
#define	SWSTR		3	/* string without \n			*/
#define	SSTRING		4	/* string				*/
#define	SWORDS		5	/* string[]				*/
#define	SALIAS		6	/* alias expansion			*/
#define	SWORDSEP	8	/* string[] seperator			*/

extern Source		*pushs (int);	 	/* push Source		*/
extern C_Op		*compile (Source *s);	/* compile tree		*/


extern Source		*source;	/* yyparse/yylex source		*/
extern YYSTYPE		yylval;		/* result from yylex		*/
extern int		yynerrs;

/* Built in Command list */

struct	builtin {
    char	*command;
    int		(*fn)(int, char **);
    int		mode;
};

extern int	doexec (C_Op *);	/* Exec function a cheat	*/

/*
 * Valid values of mode
 */

#define BLT_ALWAYS	0x0001	/* Always use builtin version		*/
#define BLT_CURRENT	0x0002	/* Currently use builtin version	*/
#define BLT_NOGLOB	0x0004	/* No globbing for this internal	*/
#define BLT_CENVIRON	0x0008	/* Don't create a new environment	*/
#define BLT_NOWORDS	0x0010	/* Don't split words for this internal	*/
#define BLT_SKIPGLOB	(BLT_CURRENT | BLT_NOGLOB)
#define BLT_SKIPENVIR	(BLT_CURRENT | BLT_CENVIRON)

/*
 * actions determining the environment of a process
 */

#define	EXEC_WITHOUT_FORK	0x0001	/* execute without forking	*/
#define	EXEC_FUNCTION		0x0002	/* execute a function		*/

#if (OS_TYPE != OS_DOS)
#  define EXEC_SPAWN_NOWAIT	0x0004	/* execute a non-wait		*/
#  define EXEC_SPAWN_DETACH	0x0008	/* execute a detach		*/
#  define EXEC_SPAWN_IGNOREWAIT	0x0010	/* Pipe processing		*/
#endif

#define EXEC_PIPE_IN		0x0020	/* On pipe			*/
#define EXEC_PIPE_SUBS		0x0040	/* Second command		*/
#define EXEC_WINDOWS		0x0080	/* Start Windows app		*/

/* MSDOS Memory Control Block chain structure */

#ifdef OS_SWAPPING
#  pragma pack (1)
struct MCB_list	{
    char		MCB_type;	/* M or Z			*/
    unsigned int	MCB_pid;	/* Process ID			*/
    unsigned int	MCB_len;	/* MCB length			*/
};
#  pragma pack ()

#  define MCB_CON	'M'		/* More MCB's			*/
#  define MCB_END	'Z'		/* Last MCB's			*/
#endif

/* Externs for Swapper assembler function */

#ifdef OS_SWAPPING
extern char far		cmd_line[];	/* Command line			*/
extern char far		path_line[];	/* Process path			*/
extern unsigned int far	SW_intr;	/* interrupt pending		*/
#else
extern unsigned int 	SW_intr;	/* interrupt pending		*/
extern bool		IgnoreInterrupts;/* Ignore interrupts flag	*/
#endif

extern int		LastNumberBase;	/* Last base entered		*/

#ifdef OS_SWAPPING
extern unsigned int far	SW_Blocks;	/* Number of blocks to read	*/
extern unsigned int far	SW_SBlocks;	/* Short Number of blocks to	*/
					/* read				*/
extern unsigned int far	SW_MinESpace;	/* Minimum environment space	*/
extern int far		SW_fp;		/* File or EMS Handler		*/
extern int far		SW_Pwrite;	/* Partial write to disk?	*/
extern unsigned long far	SW_EMstart;/* Start addr of extend mem	*/
extern unsigned int far	SW_Mode;	/* Type of swapping to do	*/
					/* 1 - disk			*/
					/* 2 - Extended	memory		*/
					/* 3 - EMS Driver		*/
					/* 4 - XMS Driver		*/
extern unsigned int far	SW_EMSFrame;	/* EMS Frame segment		*/

extern unsigned int far	etext;		/* End of text segment		*/
extern int		Swap_Mode;	/* Swapping mode		*/

/* If you change these values, change sh7, swap_device as well */

#  define SWAP_OFF	0x0000		/* No swapping			*/
#  define SWAP_DISK	0x0001		/* Disk only			*/
#  define SWAP_EXTEND	0x0002		/* Extended memory		*/
#  define SWAP_EXPAND	0x0004		/* Expanded memory		*/
#endif

/*
 * Convert to bool
 */

#define C2bool(c)	(bool)((c) ? TRUE : FALSE)

/*
 * flags to control evaluation of words
 */

#define	EXPAND_SPLITIFS	0x01	/* Perform blank interpretation		*/
#define	EXPAND_GLOBBING	0x02	/* Do globbing on name			*/
#define EXPAND_PATTERN	0x04	/* quote *?[				*/
#define	EXPAND_TILDE	0x10	/* expand ~ 				*/
#define	EXPAND_CONVERT	0x20	/* Convert - and / to DOS format	*/
#define	EXPAND_NOALTS	0x40	/* No alternations			*/

/*
 * Hard error handler
 */

#if (OS_TYPE == OS_OS2)
#  if (OS_SIZE == OS_32)
#    define DISABLE_HARD_ERRORS		DosError (FERR_DISABLEHARDERR)
#    define ENABLE_HARD_ERRORS		DosError (FERR_ENABLEHARDERR)
#  else
#    define DISABLE_HARD_ERRORS		DosError (HARDERROR_DISABLE)
#    define ENABLE_HARD_ERRORS		DosError (HARDERROR_ENABLE)
#  endif

#elif (OS_TYPE == OS_NT)
#  define DISABLE_HARD_ERRORS		SetErrorMode (SEM_FAILCRITICALERRORS | \
						      SEM_NOOPENFILEERRORBOX)
#  define ENABLE_HARD_ERRORS		SetErrorMode (0)

#elif (OS_TYPE == OS_DOS)
#  if (OS_SIZE == OS_32)
extern bool	IgnoreHardErrors;
#    define DISABLE_HARD_ERRORS		IgnoreHardErrors = TRUE;
#    define ENABLE_HARD_ERRORS		IgnoreHardErrors = FALSE;
#  else
#    define DISABLE_HARD_ERRORS
#    define ENABLE_HARD_ERRORS
#  endif

#elif (OS_TYPE == OS_UNIX)
#  define DISABLE_HARD_ERRORS
#  define ENABLE_HARD_ERRORS
#endif

/*
 * General variables
 */

extern char		**ParameterArray;/* $<numeric> values		*/
extern int		ParameterCount;	/* $<numeric> count		*/
extern int		ExitStatus;
extern bool		ExpansionErrorDetected;
extern bool		InteractiveFlag;/* interactive			*/
extern bool		ProcessingEXECCommand;
extern int		AllowMultipleLines;	/* Allow continuation	*/
extern int		Current_Event;	/* Current history event	*/
extern bool		ChangeInitLoad;	/* Change load .ini point.	*/

/*
 * Break/Continue (in for and while), Return and Exit handler
 */

typedef struct brkcon {
    jmp_buf		CurrentReturnPoint;
    struct brkcon	*NextExitLevel;
} Break_C;
				/* Values returned by longjmp		*/
#define BC_LOAD		0	/* Load condition			*/
#define BC_BREAK	1	/* Break condition			*/
#define BC_CONTINUE	2	/* Continue condition			*/

extern Break_C	*Break_List;	/* Break list for FOR/WHILE		*/
extern Break_C	*Return_List;	/* Return list for RETURN		*/
extern Break_C	*SShell_List;	/* SubShell list for EXIT		*/
extern bool	RestrictedShellFlag;	/* Read only shell		*/
extern bool	HistoryEnabled;

/*
 * Word List structure
 */

typedef struct wdblock {
    short	w_bsize;
    short	w_nword;
    char	*w_words[1];
} Word_B;

/*
 * Save Standard Input/Output/Error structure
 */

typedef struct save_io {
    int		depth;			/* Execute recursive depth	*/
    int		fp[STDERR_FILENO + 1];	/* File handlers		*/
} Save_IO;

extern Save_IO	*SSave_IO;		/* Save IO array		*/
extern int	NSave_IO_E;		/* Number of entries		*/
extern int	MSave_IO_E;		/* Max Number of entries	*/

/*
 * Function tree processing
 */

typedef struct FunctionList {
    C_Op		*tree;		/* The tree itself		*/
    bool		Traced;		/* Traced flag			*/
} FunctionList;

extern void		*FunctionTree;	/* Function Tree root		*/
extern FunctionList	*CurrentFunction;

/*
 * Alias processing
 */

typedef struct AliasList {
    char		*name;		/* The alias name		*/
    char		*value;		/* The alias			*/
    int			AFlags;		/* Alias flags			*/
} AliasList;

extern void		*AliasTree;	/* Alias Tree root		*/

#define ALIAS_TRACKED		0x0001	/* Tracked alias		*/
#define ALIAS_EXPANDING		0x0002	/* Alias being expanded		*/
#define MAX_RECURSIVEALIASES	20	/* Max depth of recursive alias	*/

/*
 * Job Processing
 */

#if (OS_TYPE != OS_DOS)

#  if (OS_TYPE == OS_NT)
typedef DWORD		PID;		/* Set up PID definition for NT */
#  endif

typedef struct JobList {
    int			Number;		/* Current number		*/
    PID			pid;		/* Process ID 			*/
    unsigned short	SessionId;	/* Session ID			*/
    char		*Command;	/* Program			*/
} JobList;

extern void		*JobTree;	/* Job Tree root		*/
extern bool		ExitWithJobsActive;	/* Exit flag		*/
extern int		CurrentJob;		/* No current		*/
extern int		PreviousJob;		/* Previous Job		*/

/*
 * Session Info
 */

extern char		*SessionEndQName;	/* Queue name		*/

/*
 * Special flag for EMX parameters
 */

extern bool		EMXStyleParameters;
#endif

/*
 * redirection
 */

typedef struct ioword {
    short	io_unit;	/* unit affected			*/
    short	io_flag;	/* action (below)			*/
    char	*io_name;	/* file name				*/
} IO_Actions;

#define	IOTYPE		0x000f	/* type: bits 0:3			*/
#define	IOREAD		0x0001	/* <					*/
#define	IOWRITE		0x0002	/* >					*/
#define	IORDWR		0x0003	/* <>					*/
#define	IOHERE		0x0004	/* << (here file)			*/
#define	IOCAT		0x0005	/* >>					*/
#define	IODUP		0x0006	/* >&digit				*/
#define	IOCLOSE		0x0007	/* >&-					*/
#define	IOEVAL 		0x0010	/* Expand in <<				*/
#define	IOSKIP		0x0020	/* <<- (here file			*/
#define	IOCLOBBER	0x0040	/* >| overwrite noclobber		*/
#define IOFUNCTION	0x0080	/* The Here document name is inside a	*/
				/* function - don't delete it		*/

/*
 * parsing & execution environment
 *
 * For some reason, MSC (and IBM C Set/2), don't like taking addresses
 * of jmp_buf's, so this macro set removes the errors
 */

#if defined (__TURBOC__) || defined (__WATCOMC__)
#  define ErrorPoint		jmp_buf *
#  define SetErrorPoint(a)	setjmp (*(e.ErrorReturnPoint = &(a)))
#  define ExitErrorPoint(a)	longjmp (*(e.ErrorReturnPoint), a)
#else
#  define ErrorPoint		int *
#  define SetErrorPoint(a)	setjmp (e.ErrorReturnPoint = a)
#  define ExitErrorPoint(a)	longjmp (e.ErrorReturnPoint, a)
#endif

#define TERMINATE_POINT_SET	0	/* Error point set		*/
#define TERMINATE_COMMAND	1	/* Exit Error point for command	*/
#define TERMINATE_SHELL		2	/* Exit Error point for shell	*/


/* The environment structure itself */

typedef struct env {
    ErrorPoint		ErrorReturnPoint;
    unsigned long	IOMap;		/* File Descriptors open in 	*/
					/* this environment		*/
    char		*line;		/* Current input line		*/
					/* Previous environment		*/
    struct env		*PreviousEnvironment;
    Word_B		*OpenStreams;
} ShellFileEnvironment;

extern ShellFileEnvironment	e;

/*
 * Switches/flags
 */

#define FL_TEST(x)	(flags & (1L << ((x) - 'a')))
#define FL_SET(x)	flags |= (1L << ((x) - 'a'))
#define FL_CLEAR(x)	flags &= (~(1L << ((x) - 'a')))

extern long	flags;

/*
 * Switch values
 */

#define FLAG_ALL_EXPORT		'a'	/* Set all env vars to exported	*/
#define FLAG_EXECUTE_STRING	'c'	/* Command from string		*/
#define FLAG_EXIT_ON_ERROR	'e'	/* Quit on error		*/
#define FLAG_DISABLE_GLOB	'f'	/* Disable file name expansion	*/
#define FLAG_TRACK_ALL		'h'	/* Track all aliases		*/
#define FLAG_INTERACTIVE	'i'	/* Interactive shell		*/
#define FLAG_ALL_KEYWORDS	'k'	/* Look for name=value everywhere */
#define FLAG_SEPARATE_GROUP	'm'	/* Separate process group	*/
#define FLAG_NO_EXECUTE		'n'	/* No execution			*/
#define FLAG_READONLY_SHELL	'r'	/* Read only shell		*/
#define FLAG_POSITION		's'	/* Read from standard input	*/
#define FLAG_ONE_COMMAND	't'	/* exit after exec'ing 1 cmd	*/
#define FLAG_UNSET_ERROR	'u' 	/* Abort if env var not set	*/
#define FLAG_ECHO_INPUT		'v'	/* Echo as read			*/
#define FLAG_WARNING		'w'	/* No Warning messages		*/
#define FLAG_PRINT_EXECUTE	'x'	/* Trace			*/

/*
 * Global flags set by set -o which do not have single letter equivalents
 */

extern unsigned int		ShellGlobalFlags;

#define FLAGS_NONE		0x0000
#define FLAGS_IGNOREEOF		0x0001	/* Ignore EOF			*/
#define FLAGS_MARKDIRECTORY	0x0002	/* Mark directories with /	*/
#define FLAGS_NOCLOBER		0x0004	/* No delete on existing files	*/
#define FLAGS_FUNCTION		0x0008	/* Special value used in	*/
					/* CreateGlobalVariableList.	*/
					/* Indicates a function caused  */
					/* CGVL to be called. Not used	*/
					/* otherwise			*/
#define FLAGS_REALPIPES		0x0010	/* Use Real pipes under OS/2	*/
#define FLAGS_ALTERNATION	0x0020	/* Allow alternations		*/

#define FLAGS_EDITORS		0x01c0	/* Any of Emacs, Gmacs, or Vi	*/
#define FLAGS_NOCASE		0x0200	/* Ignore case			*/
#define FLAGS_MSDOS_FORMAT	0x0400	/* MSDOS format environment	*/
#define FLAGS_VERIFY_SWITCH	0x0800	/* Change verify status		*/

#if (OS_TYPE == OS_DOS)
#define FLAGS_BREAK_SWITCH	0x1000	/* Change break status		*/
#define FLAGS_SET_OS2		0x2000	/* Set OS to OS2		*/
#define FLAGS_SET_NT		0x4000	/* Set OS to NT			*/
#endif

#define FLAGS_VI		0x0040	/* Vi mode			*/
#define FLAGS_EMACS		0x0080	/* Emacs mode			*/
#define FLAGS_GMACS		0x0100	/* Gmacs mode			*/

extern char	null[];		/* null value for variable		*/
extern int	InterruptTrapPending;	/* trap pending			*/
extern int	Execute_stack_depth;	/* execute function recursion	*/
					/* depth			*/

/*
 * Mode values for new GeneralPatternMatch
 */

#define GM_ALL		0		/* Match full string		*/
#define GM_SHORTEST	1		/* Shortest prefix/suffix	*/
#define GM_LONGEST	2		/* Longest prefix/suffix	*/

/*
 * Variable list
 */

typedef struct var {
    char		*name;		/* Name				*/
    char		*value;		/* Value			*/
    int			index;		/* Array index			*/
    unsigned long	nvalue;		/* Numeric value		*/
    unsigned int	base;		/* Numeric base			*/
    unsigned int	width;		/* Field width			*/
    unsigned int	status;		/* Type, see below		*/
} VariableList;

#define	STATUS_READONLY		0x0001	/* variable is read-only	*/
#define	STATUS_EXPORT		0x0002	/* variable is to be exported	*/
#define STATUS_CANNOT_UNSET	0x0008	/* PATH Value - no unset	*/
#define STATUS_CONVERT_MSDOS	0x0010	/* Convert to MSDOS format	*/
#define STATUS_LEFT_JUSTIFY	0x0020	/* Left Justify			*/
#define STATUS_RIGHT_JUSTIFY	0x0040	/* Right Justify		*/
#define STATUS_ZERO_FILL	0x0080	/* Zero fill			*/
#define STATUS_LOWER_CASE	0x0100	/* Convert to lower case	*/
#define STATUS_UPPER_CASE	0x0200	/* Convert to upper case	*/
#define STATUS_INTEGER		0x0400	/* Contains integer value	*/
#define STATUS_TAGGED		0x0800	/* User tagged			*/
#define STATUS_LOCAL		0x1000	/* Local variable in function	*/
#define STATUS_GLOBAL		0x2000	/* Global variable		*/
#define STATUS_NOEXISTANT	0x8000	/* Does not exist		*/

extern void		*VariableTree;		/* Variable dictionary	*/
extern VariableList	*CurrentDirectory;	/* Current directory	*/
extern char	PS1[];			/* Prompt 1			*/
extern char	PS2[];			/* Prompt 2			*/
extern char	PS3[];			/* Prompt 3			*/
extern char	PS4[];			/* Prompt 4			*/
extern char	IFS[];			/* Interfield separators	*/
extern char	*LastUserPrompt;	/* Last prompt output		*/
extern char	*LastUserPrompt1;	/* Alternate prompt output	*/
extern char	PathLiteral[];		/* PATH Variable		*/
extern char	CDPathLiteral[];	/* CDPATH Variable		*/
extern char	CurrentDirLiteral[];	/* Current Directory		*/
extern char	ParentDirLiteral[];	/* Parent Directory		*/
extern char	PathExtsLiteral[];	/* PATHEXTS Variable		*/
extern char	HomeVariableName[];	/* Home Variable		*/
extern char	ShellVariableName[];	/* Shell Variable		*/
extern char	*ParameterCountVariable;/* Parameter Count Variable (#)	*/
extern char	*ShellOptionsVariable;	/* Shell Options Variable (-)	*/
extern char	StatusVariable[];	/* Status variable (?)		*/
extern char	*ComspecVariable;	/* COMSPEC string		*/
extern char	SecondsVariable[];	/* Seconds string		*/
extern char	RandomVariable[];	/* Random string		*/
extern char	LineCountVariable[];	/* LINENO string		*/
extern char	*RootDirectory;		/* Root directory		*/


#if (OS_TYPE != OS_DOS)
extern char	WinTitleVariable[];	/* WINTITLE string		*/
#endif

extern char	*OldPWDVariable;	/* OLDPWD string		*/
extern char	*PWDVariable;		/* PWD string			*/
extern char	*ENVVariable;		/* ENV string			*/
extern char	BATExtension[];		/* .bat string			*/
extern char	SHELLExtension[];	/* .sh string			*/
extern char	KSHELLExtension[];	/* .ksh string			*/
extern char	EXEExtension[];		/* .exe string			*/
extern char	COMExtension[];		/* .com string			*/
extern char	HistoryFileVariable[];	/* HISTFILE string		*/
extern char	HistorySizeVariable[];	/* HISTSIZE string		*/
extern bool	UseConsoleBuffer;	/* Flag from dofc to		*/
					/* GetConsoleInput		*/
extern char	*NotFound;		/* Not found message		*/
extern char	*BasicErrorMessage;	/* Basic error message		*/
extern char	*DirectorySeparator;	/* Directory separator		*/
extern char	*DeviceNameHeader;	/* /dev/			*/
extern char	LastWordVariable[];	/* Last word of command variable*/
extern char	OptArgVariable[];	/* OPTARG			*/
extern char	OptIndVariable[];	/* OPTIND			*/
extern char	MailCheckVariable[];	/* MAILCHECK			*/
extern char	FCEditVariable[];	/* FCEDIT			*/
extern char	EditorVariable[];	/* EDITOR			*/
extern char	VisualVariable[];	/* VISUAL			*/
extern char	Trap_DEBUG[];		/* DEBUG trap variable		*/
extern char	Trap_ERR[];		/* ERR trap variable		*/
extern char	ConsoleLineBuffer[];	/* Console line buffer		*/
extern char	LIT_dos[];		/* dos Literal			*/
extern char	*LIT_NewLine;		/* NewLine Literal		*/
extern char	*LIT_BadID;		/* Bad Identifier literal	*/
extern char	*LIT_OSname;		/* OS name			*/
extern char	LIT_export[];		/* export literal		*/
extern char	LIT_history[];		/* history literal		*/
extern char	LIT_REPLY[];		/* Reply literal		*/
extern char	LIT_exit[];		/* Exit literal			*/
extern char	LIT_exec[];		/* Exec literal			*/
extern char	LIT_done[];		/* done literal			*/
extern char	LIT_LINES[];		/* LINES literal		*/
extern char	LIT_COLUMNS[];		/* COLUMNS literal		*/
extern char	*LIT_2Strings;		/* 2 String concat		*/
extern char	*LIT_3Strings;		/* 3 string concat		*/
extern char	*ListVarFormat;		/* List variable format		*/
extern char	*Outofmemory1;		/* Out of memory string		*/
extern char	*LIT_Emsg;		/* Error message format		*/
extern char	*LIT_SyntaxError;	/* Syntax error			*/
extern char 	*LIT_BadArray;		/* Bad Array value		*/
extern char	*LIT_ArrayRange;	/* subscript out of range	*/
extern char	*LIT_BNumber;		/* [%d]				*/
extern char	*LIT_Invalidfunction;	/* Invalid function name	*/
extern char	*LIT_AllowTTY;		/* Allow Psuedo TTYs		*/
extern char	*LIT_IsReadonly;	/* is readonly			*/
extern char	LIT_Test[];		/* Test function		*/
extern char	*sOpenReadMode;		/* Open file in read mode	*/
extern char	*sOpenWriteMode;	/* Open file in write mode	*/
extern char	*sOpenAppendMode;	/* Open file in append mode	*/
extern char	*sOpenWriteBinaryMode;	/* Open file in append mode	*/
extern int	MaximumColumns;		/* Max columns			*/
extern int	MaximumLines;		/* Max Lines			*/
extern int	StartCursorPosition;	/* Start cursor position	*/

#if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_NT)
extern void	SetWindowName (char *);	/* Set the Window Name		*/
#else
#  define SetWindowName(a)
#endif

/*
 * Note the the following global structure is used to pass session
 * information around.  If the value of the Environment field is
 * (char *)NULL, then the parent environment is used.  If the value is
 * (char *)1, then the environment is built from the current exported
 * environment.  Otherwise the value of Environment is used as the
 * environment
 */

#if (OS_TYPE == OS_OS2)
extern STARTDATA *SessionControlBlock;		/* Start a session info	*/
extern STARTDATA PM_SessionControlBlock;	/* PM session defaults	*/
extern STARTDATA DOS_SessionControlBlock;	/* DOS session defaults	*/
#endif

/*
 * SubShell Save Structure
 */

typedef struct subshell {
    int			depth;		/* Sub_Shell Depth		*/
    unsigned int	GFlags;		/* Global flags			*/
    long		Eflags;		/* single letter flags		*/
    void		*OldVariableTree;	/* Header start		*/
} S_SubShell;

extern S_SubShell	*SubShells;	/* Save Vars array		*/
extern int		NSubShells;	/* Number of entries		*/
extern int		MSubShells;	/* Max Number of entries	*/


/*
 * Extract field from a line
 */

typedef struct Fields {
    FILE	*FP;			/* File handler			*/
    char	*Line;			/* Line buffer			*/
    int		LineLength;		/* Line Length			*/
    Word_B	*Fields;	/* ptr to the start of fields	*/
} LineFields;

extern int	ExtractFieldsFromLine (LineFields *);
extern Word_B	*SplitString (char *, Word_B *);

/*
 * Type of processing required by executable program.
 */

typedef struct ExecutableMode {
    char		*Name;
    unsigned int	Flags;
    unsigned char	FieldSep;
} ExeMode;

extern void		CheckProgramMode (char *, ExeMode *);
extern ExeMode		ExecProcessingMode; /* Current executable mode	*/

/* Flags set a bit to indicate program mode */

#define EP_NONE		0x0000		/* Use PSP command line		*/
#define EP_DOSMODE	0x0001		/* Use DOS mode extended line	*/
#define EP_UNIXMODE	0x0002		/* Use UNIX mode extended line	*/
#define EP_NOEXPAND	0x0004		/* Use -f for this command	*/
#define EP_ENVIRON	0x0008		/* Use environ for variable	*/
#define EP_NOSWAP	0x0010		/* Do not swap for this command	*/
#define EP_EXPORT	0x0040		/* Use -m for this command	*/
#define EP_CONVERT	0x0080		/* Use conversion		*/
#define EP_NOWORDS	0x0100		/* Do word expansion		*/
#define EP_NOQUOTE	0x0200		/* No quote protection		*/
#define EP_IGNTYPE	0x0400		/* Ignore exe type (DOS only)	*/
#define EP_PSEUDOTTY	0x0800		/* Child shells allow psuedo tty*/
#define EP_QUOTEWILD	0x1000		/* Quote wildcards		*/

/*
 * storage allocation
 */

extern int	MemoryAreaLevel;	/* current allocation area */

/* Functions */

extern void	main (int, char **);

extern void	ShellErrorMessage (char *, ...);
extern void	PrintErrorMessage (char *, ...);
extern int	PrintWarningMessage (char *, ...);
extern void	CompilingError (void);

extern void	ExitTheShell (bool);
extern void	FinalExitCleanUp (int);

extern void	TerminateCurrentEnvironment (int);
extern void	CreateNewEnvironment (void);
extern void	QuitCurrentEnvironment (void);

extern void	InterruptSignalled (int);
extern void	TerminateSignalled (int);
extern void	RunTrapCommand (int);

extern void	SetShellSwitches (void);
extern Word_B	*AddWordToBlock (char *, Word_B *);
extern char	**GetWordList (Word_B *);
extern int	WordBlockSize (Word_B *);
extern char	*IntegerToString (int);
extern char	*GenerateTemporaryFileName (void);
extern int	(*IsCommandBuiltIn (char *, int *))(int, char **);
extern char	*BuildNextFullPathName (char *, char *, char *);
extern int	LookUpSymbol (char *);
extern int	GetNumericValue (char *);
extern bool	ConvertNumericValue (char *, long *, int);
extern char	*BuildFileName (char *);
extern int	CreateGlobalVariableList (unsigned int);
extern void	DeleteGlobalVariableList (void);
extern Word_B	*AddParameter (char *, Word_B *, char *);
extern char	*BuildOS2String (char **, char);
extern char	*GenerateFullExecutablePath (char *);

#if (OS_TYPE != OS_DOS)
extern char	*GetOSSystemErrorMessage (OSCALL_RET);
#endif

extern int	RestoreStandardIO (int, bool);
extern void	RestoreCurrentDirectory (char *);
extern bool	GotoDirectory (char *, unsigned int);
extern void	RestoreEnvironment (int, int);
extern bool	CheckForRestrictedShell (char *);
extern void	OutputUserPrompt (char *);
extern void	DisplayLineWithControl (char *);
extern void	GetCurrentDirectoryPath (void);
extern int	OpenForExecution (char *, char **, int *);
extern int	ProcessOutputMetaCharacters (char **);
extern char	*ConvertPathToFormat (char *, char, char);
extern int	CheckForScriptFile (char *, char **, int *);
extern void	PrintVersionNumber (FILE *);

/*
 * Smaller version of fputs & fputc
 */

extern int	feputs (char *);
extern int	foputs (char *);
extern int	feputc (int);

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32)
extern int	PrintTimes (void);
#endif

#if (OS_TYPE == OS_NT)
extern int	PrintTimes (void);
#endif

#ifdef OS_SWAPPING
extern void	ClearSwapFile (void);
#else
#  define ClearSwapFile()
#endif

extern void	ClearExtendedLineFile (void);

extern bool	GeneralPatternMatch (char *, unsigned char *, bool, char **,
				     int);
extern bool	SuffixPatternMatch (char *, char *, char **, int);
extern int	CountNumberArguments (char **);
extern int	SortCompare (const void *, const void *);

extern long	EvaluateMathsExpression (char *);
extern bool	ValidMathsExpression (char *, long *);

/*
 * Compiling and executing parse tree
 */

extern C_Op	*BuildParseTree (Source *);
extern int	ExecuteParseTree (C_Op *, int, int, int);
extern int	ScanNextToken (int);

#ifdef OS_SWAPPING
extern int	SA_spawn (char **);
#endif

extern int	ExecuteACommand (char **, int);
extern int	RunACommand (Source *, char **);
extern int	FindLocationOfExecutable (char *, char *);

extern int	ReMapIOHandler (int);

extern char	**ExpandWordList (char **, int, ExeMode *);
extern char	*ExpandAString (char *, int);
extern char	*substitute (char *, int);
extern char	*ExpandOneStringFirstComponent (char *, int);
extern char	**BuildCompletionList (char *, size_t, int *, bool);
extern size_t	GetCommonPartOfFileList (char **);

extern bool	ChangeInitialisationValue (char *, int);
extern void	Configure_Keys (void);

/*
 * Keyboard Input
 */

extern bool		RingWarningBell (void);
#if (OS_TYPE == OS_DOS)
extern unsigned char	Poll_Keyboard (void);
#else
#  define Poll_Keyboard()
#endif

extern void		PositionCursorInColumnZero (void);
extern int		GetConsoleInput (void);
extern int		GetDescriptorType (int);
extern bool		IsDirectory (char *);
extern int		GetEOFKey (void);
extern int		LookUpKeyBoardFunction (unsigned char, unsigned char);
extern int		ReadCursorPosition (void);
extern void		SetCursorPosition (int);
extern bool		ClearScreen (void);
extern void		PrintAList (int, char **);
extern void		GetScreenParameters (void);
extern void		SetCursorShape (bool);
extern void		EMACS_Initialisation (void);

/* read the keyboard */

extern unsigned char	ReadKeyBoard (unsigned char *);

#define KT_FUNCTION	0			/* Functionkey		*/
#define KT_ALTFUNCTION	0xff			/* ALT key pressed	*/
#define KT_RESIZE	0xfe			/* Window re-sized	*/

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
extern int		EditorInput (void);
#endif

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
extern int		BindKeyStroke (char *, char *, bool);
extern unsigned char	GetFunctionKeyMap (int, unsigned char *);
#endif

/*
 * Keyboard functions
 */

#define KF_SCANBACKWARD	0x00		/* Scan backwards in history	*/
#define KF_SCANFOREWARD	0x01		/* Scan forewards in history	*/
#define KF_PREVIOUS	0x02		/* Previous command		*/
#define KF_NEXT		0x03		/* Next command			*/
#define KF_LEFT		0x04		/* Left one character		*/
#define KF_RIGHT	0x05		/* Right one character		*/
#define KF_WORDRIGHT	0x06		/* Right one word		*/
#define KF_WORDLEFT	0x07		/* Left one word		*/
#define KF_START	0x08		/* Move to start of line	*/
#define KF_CLEAR	0x09		/* Clear input line		*/
#define KF_FLUSH	0x0a		/* Flush to end of line		*/
#define KF_END		0x0b		/* End of line			*/
#define KF_INSERT	0x0c		/* Insert mode switch		*/
#define KF_DELETERIGHT	0x0d		/* Delete right character	*/
#define KF_DELETELEFT	0x0e		/* Delete left character	*/
#define KF_COMPLETE	0x0f		/* Complete file name		*/
#define KF_DIRECTORY	0x10		/* Complete directory function	*/
#define KF_CLEARSCREEN	0x11		/* Clear screen			*/
#define KF_JOBS		0x12		/* Print Job tree		*/
#define KF_TRANSPOSE	0x13		/* Transpose characters		*/
#define KF_QUOTE	0x14		/* Quote character		*/
#define KF_END_FKEYS	0x15		/* End of function keys		*/

/*
 * Other init file codes
 */

#define KF_RINGBELL	0x15		/* Ring bell			*/
#define KF_HALFHEIGTH	0x16		/* Half height cursor		*/
#define KF_INSERTMODE	0x17		/* Overstrike or Insert		*/
#define KF_INSERTCURSOR	0x18		/* Enable insert cursor		*/
#define KF_ROOTDRIVE	0x19		/* Root drive			*/
#define KF_EOFKEY	0x1a		/* EOF value			*/

/*
 * Variable Name functions
 */

extern void		UnSetVariable (char *, int, bool);
extern void		SetVariableStatus (char *, int);
extern void		SetVariableArrayStatus (char *, int, int);
extern void		ClearVariableStatus (char *, int);
extern char		*GetVariableAsString (char *, bool);
extern char		*GetVariableArrayAsString (char *, int, bool);
extern long		GetVariableAsNumeric (char *);
extern long		GetVariableArrayAsNumeric (char *, int);
extern int		CountVariableArraySize (char *);
extern void		SetVariableFromString (char *, char *);
extern void		SetVariableArrayFromString (char *, int, char *);
extern void		SetVariableFromNumeric (char *, long);
extern void		SetVariableArrayFromNumeric (char *, int, long);
extern void		HandleSECONDandRANDOM (void);
extern bool		AssignVariableFromString (char *, int *);
extern bool		GetVariableName (char *, long *, char **, bool *);
extern char		IsValidVariableName (char *);
extern VariableList	*LookUpVariable (char *, int, bool);
extern int		SearchVariable (const void *, const void *);
extern void		BuildExtensionLists (void);

/*
 * Some missing EMX functions
 */

#if defined (__EMX__)
extern char		*ltoa (long, char *, int);
extern int		cwait (int *, int, int);
#endif

/*
 * Memory management
 */

extern char		*AllocateMemoryCell (size_t);
extern void		ReleaseMemoryCell (void *);
extern void		ReleaseAList (char **);
extern void		ReleaseMemoryArea (int);
extern void		SetMemoryAreaNumber (void *, int);
extern int		GetMemoryAreaNumber (void *);
extern void		*DuplicateMemoryCell (void *);
extern void		*GetAllocatedSpace (size_t);
extern void		*ReAllocateSpace (void *, size_t);
extern char		*StringSave (char *);
extern char		*StringCopy (char *);
extern size_t		GetMemoryCellSize (void *);

#ifdef DEBUG_MEMORY
extern void		DumpMemoryCells (int);
#define exit(x)		DumpMemoryCells (x)
#endif

/*
 * UNIX File I/O function emulation
 */

extern int		S_open (bool, char *, int);
extern int		S_close (int, bool);
extern void		S_fclose (FILE *, bool);
extern bool		S_stat (char *, struct stat *);
extern int		S_dup (int);
extern int		S_dup2 (int, int);
extern bool		S_access (char *, int);
extern bool		S_chdir (char *);
extern bool		S_getcwd (char *, int);
extern int		S_Remap (int, int);
extern int		OpenAPipe (void);
extern void		CloseThePipe (int);
extern void		CloseAllHandlers (void);

#if (OS_TYPE == OS_UNIX)
#  define CheckDOSFileName(a)	(a)
#  define GetCurrentDrive()	0
#  define SetCurrentDrive(a)	-1
#  define GetDriveLetter(a)	0
#  define GetDriveNumber(a)	0
#  define GetRootDiskDrive()	0
#else
extern char		*CheckDOSFileName (char *);
extern unsigned int	GetCurrentDrive (void);
extern int		SetCurrentDrive (unsigned int);
extern char	 	GetDriveLetter (unsigned int);
extern unsigned int	GetDriveNumber (char);
extern int		GetRootDiskDrive (void);
#endif

extern int		CloseFile (FILE *);
extern FILE		*FOpenFile (char *, char *);
extern FILE		*ReOpenFile (int, char *);
extern void		FlushStreams (void);
extern void		ChangeFileDescriptorStatus (int, bool);

/*
 * Shell Functions
 */

#define PF_MODE_NORMAL	0		/* Normal command		*/
#define PF_MODE_ASYNC	1		/* Async command		*/
#define PF_MODE_COPROC	2		/* Co-process command		*/
#define PF_MODE_NO	3		/* No mode			*/

extern FunctionList	*LookUpFunction (char *, bool);
extern bool		SaveFunction (C_Op *);
extern void		DeleteFunction (C_Op *);
extern void		PrintFunction (C_Op *, int);
extern C_Op		*CopyFunction (C_Op *);
extern int		PrintAllFunctions (void);
extern void		DeleteAllFunctions (void);

/*
 * Alias processing
 */

extern AliasList	*LookUpAlias (char *, bool);
extern void		DeleteAlias (char *);
extern bool		SaveAlias (char *, char *, bool);
extern void		PrintAlias (char *);
extern bool		IsValidAliasName (char *, bool);
extern void		UnTrackAllAliases (void);
extern int		PrintAllAlias (bool);

/*
 * Here document processing
 */

extern void		ScrapHereList (void);
extern void		FreeAllHereDocuments (int);
extern void		SaveHereDocumentInfo (IO_Actions *);
extern int		OpenHereFile (char *, bool);

/*
 * Job Processing
 */

#if (OS_TYPE != OS_DOS)
extern int		AddNewJob (PID, unsigned short, char *);
extern void		DeleteJob (PID);
extern void		DeleteJobBySession (unsigned short);
extern int		PrintJobs (bool);
extern int		NumberOfActiveJobs (void);
extern JobList		*LookUpJob (int);
extern JobList		*SearchForJob (char *);
extern int		PrintProcessTree (pid_t);
#endif

/*
 * History Processing
 */

extern void		AddHistory (bool);
extern void		LoadHistory (void);
extern void		DumpHistory (void);
extern void		ClearHistory (void);
extern void		PrintHistory (bool, bool, int, int, FILE *);
extern int		GetLastHistoryEvent (void);
extern int		GetFirstHistoryEvent (void);
extern char		*GetHistoryRecord (int);

#if (OS_TYPE != OS_DOS)
extern char		*GetLastHistoryString (void);
#endif

extern int		SearchHistory (char *);
extern void		FlushHistoryBuffer (void);
extern void		DeleteLastHistory (void);
extern char		CleanUpBuffer (int, char *, int);

/*
 * Interrupt handling
 */

#if (OS_TYPE == OS_DOS)
#  if (OS_SIZE == OS_16)
extern void interrupt far SW_Int24 (void);	/* Int 24 New address	*/
extern void interrupt far SW_Int23 (void);	/* Int 23 New address	*/
extern void interrupt far SW_Int00 (void);	/* Int 00 New address	*/
extern bool far		SW_I23_InShell;		/* In the shell		*/
#  elif !defined (__EMX__)
extern int	__far	HardErrorHandler (unsigned int, unsigned int,
					  unsigned int *);
#  endif
#endif

#if (OS_TYPE == OS_DOS)
#  if defined (__TURBOC__)
#    define GetInterruptVector(x)	(void (interrupt far *)())getvect ((x))
#    define SetInterruptVector(x,y)	setvect ((x), (y))
#  else
#    define GetInterruptVector(x)	_dos_getvect ((x))
#    define SetInterruptVector(x,y)	_dos_setvect ((x), (y))
#  endif
#endif

/*
 * XMS Driver functions
 */

#ifdef OS_SWAPPING
extern unsigned long far SW_XMS_Driver;		/* XMS Driver Interface	*/
extern int		SW_XMS_Gversion (void);
extern int		SW_XMS_Allocate (unsigned int);
extern int		SW_XMS_Free (int);
extern int		SW_XMS_Available (void);
#endif

/*
 * Modified getopt for shell
 */

extern int		OptionIndex;		/* optind		*/
extern int		OptionStart;		/* start character	*/
extern char		*OptionArgument;	/* optarg		*/

extern int		GetOptions (int, char **, char *, int);
extern void		ResetGetOptions (void);

/*
 * Save structure for getopts command
 */

typedef struct GetoptsIndex {
    int		Index;
    int		SubIndex;
} GetoptsIndex;

extern void		ResetGetoptsValues (bool);
extern void		GetGetoptsValues (GetoptsIndex *);
extern void		SaveGetoptsValues (int, int);

/*
 * General Functions
 */


/*
 * Flag values
 */

#define GETOPT_PLUS	0x01		/* Allow plus sign		*/
#define GETOPT_MESSAGE	0x02		/* Print error message		*/
#define GETOPT_PRINT	0x04		/* doecho special		*/
#define GETOPT_AMISSING	0x08		/* doset special		*/

/*
 * TSEARCH Functions
 */

typedef enum { preorder, postorder, endorder, leaf }	VISIT;

/*
 * Tree functions
 */

extern void	*tsearch (void *, void **, int (*)(const void *, const void *));
extern void	*tfind (void *, void **, int (*)(const void *, const void *));
extern void	*tdelete (void *, void **, int (*)(const void *, const void *));
extern void	twalk (const void *, void (*)(const void *, VISIT, int));

/*
 * DEBUG
 */

#ifdef DEBUG_ON
#  define DPRINT(a,b)	db_printf b
extern void		db_printf (char *, ...);
#else
#  define DPRINT(a,b)
#endif
