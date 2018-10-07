/*
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited
 *
 * This code is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/ptype.c,v 1.2 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: ptype.c,v $
 * Revision 1.2  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 1.1  1994/02/23  09:23:38  istewart
 * Beta 233 updates
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include "sh.h"

static unsigned long		QueryApplicationType1 (int);
static void			WhenceType (char *path);
unsigned long			QueryApplicationType (const char *);

/*
 * Find out the application type
 */

unsigned long	QueryApplicationType (const char *pathname)
{
    int			fd;
    unsigned long	res;
    size_t		len;

    if (pathname == NULL)
	return EXETYPE_BAD_FILE;
    
/* Open the file */

    if ((fd = open (pathname, O_RDONLY | O_BINARY)) == -1)
	return EXETYPE_BAD_FILE;

    res = QueryApplicationType1 (fd);
    close (fd);

/* Check for .com file */

    if ((res == EXETYPE_UNKNOWN) &&
	((len = strlen (pathname)) > 5) &&
        (stricmp (&pathname[len - 4], ".com") == 0))
	return EXETYPE_DOS_CUI;

    return res;
}

/*
 * Do the actual work!
 */

static unsigned long QueryApplicationType1 (int fd)
{
    union {
	struct ExecOS2_16Header	OS2aHead;
	struct ExecOS2_32header	OS2bHead;
	struct ExecNTHeader	NTHead;
	struct ExecDosHeader	DosHead;
    }				OS_Headers;
    struct stat			fstatus;

    if ((read (fd, &OS_Headers, sizeof (struct ExecDosHeader)) !=
		sizeof (struct ExecDosHeader)) ||
        (OS_Headers.DosHead.e_magic != SIG_DOS))
	return EXETYPE_UNKNOWN;
    
/*
 * If the header size in the header is not a new header or the relocation
 * section starts before the end of the new header, it must be a DOS program
 */

    if ((OS_Headers.DosHead.e_cparhdr * 16 < sizeof (struct ExecDosHeader)) ||
	(OS_Headers.DosHead.e_lfarlc < sizeof (struct ExecDosHeader)))
        return EXETYPE_DOS_CUI;
    
    if ((fstat (fd, &fstatus) == -1) ||
	(fstatus.st_size < (off_t) OS_Headers.DosHead.e_lfanew))
        return EXETYPE_DOS_CUI;

    if ((lseek (fd, (off_t) OS_Headers.DosHead.e_lfanew, SEEK_SET) ==
		(off_t) -1) ||
	(read (fd, &OS_Headers, sizeof (OS_Headers)) != sizeof (OS_Headers)))
	return EXETYPE_BAD_IMAGE;
    
/*
 * Check for NT
 */

    if (OS_Headers.NTHead.Signature == SIG_NT)
    {
	if (OS_Headers.NTHead.FileHeader.SizeOfOptionalHeader !=
	    NT_OPTIONAL_HEADER)
	    return EXETYPE_UNKNOWN;

	switch (OS_Headers.NTHead.OptionalHeader.Subsystem)
	{
	    default:
		return EXETYPE_UNKNOWN;

	    case NT_SS_NATIVE:
		return EXETYPE_NT_NATIVE;

	    case NT_SS_WINDOWS_GUI:
		return EXETYPE_NT_WINDOWS_GUI;

	    case NT_SS_WINDOWS_CUI:
		return EXETYPE_NT_WINDOWS_CUI;

	    case NT_SS_OS2_CUI:
		return EXETYPE_NT_OS2;

	    case NT_SS_POSIX_CUI:
		return EXETYPE_NT_POSIX;
	}
    }

/* OS2 1.x */

    else if ((OS_Headers.OS2aHead.ne_magic == SIG_OS2_16) ||
	     (OS_Headers.OS2aHead.ne_magic == SIG_OS2_16LE))
    {
        printf ("ne_flags       = %.4x  ne_flagsothers = %.4x (Win Ver %d)\n",
		 OS_Headers.OS2aHead.ne_flags,
		 OS_Headers.OS2aHead.ne_flagsothers,
		 OS_Headers.OS2aHead.ne_expver);
        printf ("ne_exetyp      = %.4x (R %.2x V %.2x)\n",
		 OS_Headers.OS2aHead.ne_exetyp,
		 OS_Headers.OS2aHead.ne_ver,
		 OS_Headers.OS2aHead.ne_rev);

	if (OS_Headers.OS2aHead.ne_flags & (OS2_16_NOTP | OS2_16_IERR))
	    return EXETYPE_BAD_IMAGE;

	if (OS_Headers.OS2aHead.ne_exetyp == OS2_16_WINDOWS)
	    return EXETYPE_DOS_GUI;

/* This appears to be what Watcom generates */

	else if ((OS_Headers.OS2aHead.ne_exetyp == OS2_16_UNKNOWN) &&
		 (OS_Headers.OS2aHead.ne_flags == 0))
	    return EXETYPE_DOS_32;


/* Under OS/2, A bound app is an OS/2 app otherwise its a DOS app */

#if (OS_TYPE != OS_OS2)
	else if (OS_Headers.OS2aHead.ne_exetyp == OS2_16_UNKNOWN)
	    return EXETYPE_DOS_CUI;

	else if (OS_Headers.OS2aHead.ne_flags & OS2_16_BOUND)
	    return EXETYPE_DOS_BOUND;
#else
	else if (OS_Headers.OS2aHead.ne_exetyp == OS2_16_UNKNOWN)
	    return EXETYPE_OS2_CUI;
#endif

/* Real OS/2 app */

	else if (OS_Headers.OS2aHead.ne_exetyp == OS2_16_OS2)
	{
	    switch (OS_Headers.OS2aHead.ne_flags & OS2_16_APPTYP)
	    {
	        case OS2_16_NOTWINCOMPAT:
		    return EXETYPE_OS2_CUI;

		case OS2_16_WINCOMPAT:
		    return EXETYPE_OS2_CGUI;

		case OS2_16_WINAPI:
		    return EXETYPE_OS2_GUI;

		case 0:
#if (OS_TYPE == OS_OS2)
		    return EXETYPE_OS2_CUI;
#else
		    return EXETYPE_DOS_BOUND;
#endif
	    }
	}
    }

/* OS2 2.x */

    else if (OS_Headers.OS2bHead.e32_magic == SIG_OS2_32)
    {
        printf ("Mflags = %.8lx\n", OS_Headers.OS2bHead.e32_mflags);

        if ((OS_Headers.OS2bHead.e32_mflags & (OS2_NOTP | OS2_NOLOAD)) ||
	    (OS_Headers.OS2bHead.e32_mflags & OS2_MODMASK))
	    return EXETYPE_NOT_EXE;
	
	if ((OS_Headers.OS2bHead.e32_mflags & OS2_APPMASK) == OS2_NOPMW)
		return EXETYPE_OS2_CUI | EXETYPE_OS2_32;

	else if ((OS_Headers.OS2bHead.e32_mflags & OS2_APPMASK) == OS2_PMW)
		return EXETYPE_OS2_CGUI | EXETYPE_OS2_32;

	else if ((OS_Headers.OS2bHead.e32_mflags & OS2_APPMASK) == OS2_PMAPI)
		return EXETYPE_OS2_GUI | EXETYPE_OS2_32;
    }

/* Give UP! */

    return EXETYPE_UNKNOWN;
}

/*
 * Output file type
 */

static char	 *ExeType_Error[] = {
    "Not known",
    "Bad image",
    "Not executable",
    "File not found"
};

static char	*ExeType_Dos[] = {
    "DOS Character",
    "Windows 16-bit",
    "Watcom 32 bit",
    "OS/2 Bound"
};

static char	*ExeType_OS2[] = {
   "Not PM compatible",
   "PM compatible",
   "PM"
};

static char	*ExeType_NT[] = {
    "Native",
    "Windows GUI",
    "Windows CUI",
    "OS2",
    "POSIX"
};

static void WhenceType (char *path)
{
    unsigned long type = QueryApplicationType (path);
   
    printf ("%s [", path);

    if (type & EXETYPE_ERROR)
	printf (ExeType_Error [type - 1]);

    else if (type & EXETYPE_DOS)
	printf (ExeType_Dos [(type >> 4) - 1]);

    else if (type & EXETYPE_OS2)
	printf ("OS2 %dbit %s", (type & EXETYPE_OS2_32) ? 32 : 16,
		 ExeType_OS2 [((type & EXETYPE_OS2_TYPE) >> 8) - 1]);

    else if (type & EXETYPE_NT)
	printf ("Win NT %s subsystem", ExeType_NT [(type >> 12) - 1]);

    printf ("]\n");
}

void	main (int argc, char **argv)
{
    int		i;

    for (i = 1; i < argc; i++)
	WhenceType (argv[i]);

    exit (0);
}
