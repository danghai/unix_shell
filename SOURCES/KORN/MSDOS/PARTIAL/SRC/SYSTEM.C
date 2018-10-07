/* MS-DOS System Function with Swaping - system (3C)
 *
 * MS-DOS System - Copyright (c) 1990,1,2 Data Logic Limited.
 *
 * This code is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form.
 *
 * Author:
 *	Ian Stewartson
 *	Data Logic, Queens House, Greenhill Way
 *	Harrow, Middlesex  HA1 1YR, UK.
 *	istewart@datlog.co.uk or ukc!datlog!istewart
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/system.c,v 2.4 1993/08/25 16:04:22 istewart Exp $
 *
 *    $Log: system.c,v $
 *	Revision 2.4  1993/08/25  16:04:22  istewart
 *	Add support for new options
 *
 *	Revision 2.3  1993/06/14  10:59:58  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.2  1993/06/02  09:52:35  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.1  1993/01/26  18:35:36  istewart
 *	Fix OS2 version bug (missing semi-colon).
 *
 *	Revision 2.0  1992/05/21  16:49:54  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 *
 * MODULE DEFINITION:
 *
 * This is a version of the standard system(3c) library function.  The only
 * difference is that it supports swapping and MS-SHELL EXTENDED_LINE
 * processing.
 *
 * To get the OS2 version, compile with -DOS2
 *
 * There are four macros which can be changed:
 *
 * GET_ENVIRON		To get a variable value from the environment
 * FAIL_ENVIRON		The result on failure
 * FATAL_ERROR		Handle a fatal error message
 * SHELL_SWITCH		The command switch to the SHELL.
 *
 * This module replaces the standard Microsoft SYSTEM (3C) call.  It should
 * work with most models.  It has been tested in Large and Small model.
 * When you link a program using the swapper, the swapper front end
 * (swap.obj) must be the first object model on the linker command line so
 * that it is located immediately after the PSP.  For example:
 *
 *	link swap+x1+x2+x3+system,x1;
 * or
 *	cl -o z1 swap.obj x1.obj x2.obj x3.obj system
 *
 * The location of the system object is not relevent.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

#if defined (__TURBOC__)
#  include <dir.h>
#endif

#ifdef DL_MAKE
#  include "make.h"
#endif

#if defined(OS2) || defined (__OS2__)
#  define INCL_DOSSESMGR
#  define INCL_DOSMEMMGR
#  define INCL_DOSPROCESS
#  define INCL_WINSWITCHLIST
#  define INCL_DOSERRORS
#  include <os2.h>
#else
#  include <dos.h>
#endif

#ifdef __OS2__
#  define F_LOCAL
#else
#  define F_LOCAL		near
#endif

#ifndef P_WAIT
#  define P_WAIT	0
#endif

/*
 * Externals declared by the swapper
 */

#if !defined(OS2) && !defined (__OS2__)
extern char far		cmd_line[];	/* Command line			*/
extern char far		path_line[];	/* Process path			*/
extern unsigned int far	SW_intr;	/* interrupt pending		*/
extern unsigned int far	SW_Blocks;	/* Number of blocks to read	*/
extern int far		SW_fp;		/* File or EMS Handler		*/
extern unsigned int far	SW_EMsize;	/* Number of extend memory blks	*/
extern unsigned long far SW_EMstart;	/* Start addr of extend mem	*/

#define SWAP_TO_DISK	1		/* Swap to disk			*/
#define SWAP_TO_Ext	2		/* Swap to extended memory	*/
					/* Not recommended - no mgt	*/
#define SWAP_TO_EMS	3		/* Swap to EMS			*/
#define SWAP_TO_XMS	4		/* Swap to XMS			*/

extern unsigned int far	SW_Mode;	/* Type of swapping to do	*/
extern unsigned int far	SW_EMSFrame;	/* EMS Frame segment		*/
extern bool far		SW_I23_InShell;	/* In the shell			*/

/* Functions */

static void F_LOCAL	ClearSwapFile (void);
extern int far		SA_spawn (char **);
extern void (interrupt far *SW_I23_V) (void);	/* Int 23 address	*/
extern void (far	*SW_XMS_Driver) (void);	/* XMS Driver Interface	*/
extern int far		SW_XMS_Gversion (void);
extern int far		SW_XMS_Allocate (unsigned int);
extern int far		SW_XMS_Free (int);
extern unsigned int far	SW_XMS_Available (void);
extern void interrupt far SW_Int23 (void);	/* Int 23 New address	*/
extern void interrupt far SW_Int00 (void);	/* Int 00 New address	*/
#endif

#define FFNAME_MAX	(PATH_MAX + NAME_MAX + 3)

/*
 * Convert to bool
 */

#define C2bool(c)	(bool)((c) ? TRUE : FALSE)

/* Set these to the appropriate values to get environment variables.  For
 * make the following values work.  Normally, getenv and (char *)NULL should
 * be used.
 */

#ifdef DL_MAKE
#define GET_ENVIRON(p)		GetMacroValue (p)
#define FAIL_ENVIRON		Nullstr
#define FATAL_ERROR(a)		PrintFatalError (a)
#define SHELL_SWITCH		"-ec"
#define WINDOW_NAME		"DL Make"
#else
#define FATAL_ERROR(a)		{ fputs (a, stderr); fputc ('\n', stderr); exit (1); }
#define GET_ENVIRON(p)		getenv (p)
#define FAIL_ENVIRON		(char *)NULL
#define SHELL_SWITCH		"-c"
#ifdef TEST
#define WINDOW_NAME		"TEST"
#else
#define WINDOW_NAME
#endif
#endif

/* Declarations */

				/* Open in create mode for swap file	*/
#define O_SMASK		(O_RDWR | O_CREAT | O_TRUNC | O_BINARY)

/*
 * Result from FindLocationOfExecutable
 */

#define EXTENSION_NOT_FOUND	0	/* Cannot find file		*/
#define EXTENSION_EXECUTABLE	1	/* OS/2 or DOS .exe or .com	*/

/*
 * Some MSDOS Swapper info
 */

#if !defined(OS2) && !defined (__OS2__)
#define CMD_LINE_MAX	127	/* Max command line length		*/

/* MSDOS Memory Control Block chain structure */

#pragma pack (1)
struct MCB_list	{
    char		MCB_type;	/* M or Z			*/
    unsigned int	MCB_pid;	/* Process ID			*/
    unsigned int	MCB_len;	/* MCB length			*/
};
#pragma pack ()

#define MCB_CON		'M'		/* More MCB's			*/
#define MCB_END		'Z'		/* Last MCB's			*/

/* Swap Mode */

#define SWAP_OFF	0x0000		/* No swapping			*/
#define SWAP_DISK	0x0001		/* Disk only			*/
#define SWAP_EXTEND	0x0002		/* Extended memory		*/
#define SWAP_EXPAND	0x0004		/* Expanded memory		*/

static int	Swap_Mode = (SWAP_DISK | SWAP_EXPAND | SWAP_EXTEND);
static char	*NoSwapFiles = "No Swap files";
static char	*MS_emsg = "WARNING - %s Error (%x)";
static char	*MS_Space = "WARNING - %s out of space";
static char	*SwapFailed = "%s swap failed (%x)";
static char	*Swap_File = (char *)NULL;	/* Swap file	*/
#endif

static char	*Extend_file = (char *)NULL;

#if defined (OS2) || defined (__OS2__)

#define CMD_LINE_MAX		16000
#define EXTENSION_COUNT		2
static char	*Extensions [] = { "", ".exe"};
static char	path_line[FFNAME_MAX];	/* Execution path		*/
static char	FailName[NAME_MAX + PATH_MAX + 3];

#else

#define EXTENSION_COUNT		3
static char	*Extensions [] = { "", ".exe", ".com"};

#endif

/*
 * Word List structure
 */

typedef struct wdblock {
    short	w_bsize;
    short	w_nword;
    char	*w_words[1];
} Word_B;

/*
 * Extract field from a line
 */

typedef struct Fields {
    FILE	*FP;			/* File handler			*/
    char	*Line;			/* Line buffer			*/
    int		LineLength;		/* Line Length			*/
    Word_B	*Fields;	/* ptr to the start of fields	*/
} LineFields;

/*
 * Program type
 */

static struct ExecutableProcessing {
    char		*Name;
    unsigned int	Flags;
    unsigned char	FieldSep;
} ExecProcessingMode;

/* Flags set a bit to indicate program mode */

#define EP_NONE		0x000		/* Use PSP command line		*/
#define EP_DOSMODE	0x001		/* Use DOS mode extended line	*/
#define EP_UNIXMODE	0x002		/* Use UNIX mode extended line	*/
#define EP_NOEXPAND	0x004		/* Use -f for this command	*/
#define EP_ENVIRON	0x008		/* Use environ for variable	*/
#define EP_NOSWAP	0x010		/* Do not swap for this command	*/
#define EP_COMSPEC	0x020		/* Special for .bat files	*/
#define EP_EXPORT	0x040		/* Use -m for this command	*/
#define EP_CONVERT	0x080		/* Use conversion		*/
#define EP_NOWORDS	0x100		/* Do word expansion		*/
#define EP_NOQUOTE	0x200		/* No quote protection		*/

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
 * Common fields in EXTENDED_LINE file
 */

#define COMMON_FIELD_COUNT	5

static struct CommonFields {
    char		*Name;
    unsigned int	Flag;
} CommonFields [] = {
    { "switch",		EP_CONVERT },
    { "export",		EP_EXPORT },
    { "noswap",		EP_NOSWAP },
    { "noquotes",	EP_NOQUOTE }
};

/*
 * Functions
 */

#if !defined(OS2) && !defined (__OS2__)
static bool F_LOCAL	Get_XMS_Driver (void);
static bool F_LOCAL	Get_EMS_Driver (void);
static bool F_LOCAL	EMS_error (char *, int);
static bool F_LOCAL	XMS_error (char *, int);
static int F_LOCAL	XMS_Close (void);
static int F_LOCAL	EMS_Close (void);
static int F_LOCAL	SwapToDiskError (int, char *);
static int F_LOCAL	SwapToMemory (int);
static void F_LOCAL	SetUpSwapper (void);
#endif

static void F_LOCAL	S_getcwd (char *, int);
static int F_LOCAL	CountDoubleQuotes (char *);
static bool F_LOCAL	IsValidVariableName (char *);
static int F_LOCAL	CountNumberArguments (char **);
static bool F_LOCAL	ConvertNumericValue (char *, long *, int);
static bool F_LOCAL	CheckParameterLength (char **);
static int F_LOCAL	WordBlockSize (Word_B *);
static Word_B * F_LOCAL AddWordToBlock (char *, Word_B *);
static void F_LOCAL	ClearExtendedLineFile (void);
static int F_LOCAL	ExecuteProgram (char *, char **);
static int F_LOCAL	FindLocationOfExecutable (char *, char *);
static char * F_LOCAL	GenerateTemporaryFileName (void);
static char * F_LOCAL	BuildNextFullPathName (char *, char *, char *);
static void F_LOCAL	CheckProgramMode (char *);
static unsigned int F_LOCAL CheckForCommonOptions (LineFields *, int);
static void F_LOCAL	SetCurrentDrive (unsigned int);
static int F_LOCAL	SpawnProcess (void);
static int F_LOCAL	BuildCommandLine (char *, char **);
static char * F_LOCAL	GenerateFullExecutablePath (char *);
static bool F_LOCAL	WriteToExtendedFile (FILE *, char *);
static size_t F_LOCAL	WhiteSpaceLength (char *, bool *);
static int F_LOCAL	StartTheProcess (char *, char **);
static char ** F_LOCAL	ProcessSpaceInParameters (char **);
static void F_LOCAL	ConvertPathToFormat (char *);
static char * F_LOCAL	BuildOS2String (char **, char);
static int		ExtractFieldsFromLine (LineFields *);
static Word_B * F_LOCAL SplitString (char *, Word_B *);
#if defined(OS2) || defined (__OS2__)
static void F_LOCAL	SetWindowName (void);
static int F_LOCAL	OS2_DosExecProgram (char *, char **);
static char * F_LOCAL	InsertCharacterAtStart (char *);
#endif

#if defined(M_I86LM)
#define FAR_strcpy	strcpy
#define FAR_memcpy	memcpy
#define FAR_memcmp	memcmp
#else
#define FAR_strcpy	_fstrcpy
#define FAR_memcpy	_fmemcpy
#define FAR_memcmp	_fmemcmp
#endif


/*
 * Test program
 */

#ifdef TEST
int main (int argc, char **argv)
{
    int		i;

    for (i = 1; i < argc; i++)
	printf ("Result = %d\n", system (argv[i]));

    return 0;
}
#endif

/*
 * System function with swapping
 */

int		system (char const *arg2)
{
    char	*argv[4];
    char	*ep;
    int		res, serrno, len;
    char	p_name[PATH_MAX + NAME_MAX + 3];
    char	cdirectory[PATH_MAX + 4];	/* Current directory	*/
    char	*SaveEV = (char *)NULL;
    bool	UsedComSpec = FALSE;

/* Set up argument array */

    argv[1] = SHELL_SWITCH;

    if ((argv[0] = GET_ENVIRON ("SHELL")) == FAIL_ENVIRON)
    {
	argv[0] = GET_ENVIRON ("COMSPEC");
	argv[1] = "/c";
	UsedComSpec = TRUE;
    }

    if (argv[0] == FAIL_ENVIRON)
	FATAL_ERROR ("No Shell available");

    argv[2] = (char *)arg2;
    argv[3] = (char *)NULL;

/* Check to see if the file exists.  First check for command.com to use /
 * instead of -
 */

    if ((ep = strrchr (argv[0], '/')) == (char *)NULL)
	ep = argv[0];

    else
	++ep;

/* Check the program mode */

    CheckProgramMode (*argv);

/* Check for command.com */

#if !defined(OS2) && !defined (__OS2__)
    if (!stricmp (ep, "command.com") || !stricmp (ep, "command"))
    {
	union REGS	r;

	r.x.ax = 0x3700;
	intdos (&r, &r);

	if ((r.h.al == 0) && (_osmajor < 4))
	    *argv[1] = (char)(r.h.dl);

	if (ExecProcessingMode.Flags & EP_CONVERT)
	    ExecProcessingMode.Flags |= EP_COMSPEC;
    }
#endif

/* If we used COMSPEC, default to NOQUOTES */

    if (UsedComSpec)
	ExecProcessingMode.Flags |= EP_NOQUOTE;

/* Convert arguments.  If this is COMSPEC for a batch file command, skip over
 * the first switch
 */

    if (ExecProcessingMode.Flags & EP_COMSPEC)
	len = 2;

/*
 * Convert from UNIX to DOS format: Slashes to Backslashes in paths and
 * dash to slash for switches
 */

    if (ExecProcessingMode.Flags & EP_CONVERT)
    {
	while ((ep = argv[len++]) != (char *)NULL)
	{
	    ConvertPathToFormat (ep);

	    if (*ep == '-')
		*ep = '/';
	}
    }

/* Save the current directory */

    getcwd (cdirectory, PATH_MAX + 3);

/* If pass in environment, set up environment variable */

    if (ExecProcessingMode.Flags & EP_ENVIRON)
    {
	if ((SaveEV = GET_ENVIRON (ExecProcessingMode.Name)) != FAIL_ENVIRON)
	    SaveEV = strdup (ExecProcessingMode.Name);

/* Get some space for the environment variable */

	if ((ep = malloc (strlen (ExecProcessingMode.Name) + strlen (argv[1]) +
			  strlen (argv[2]) + 3)) == (char *)NULL)
	{
	    if (SaveEV != (char *)NULL)
		free (SaveEV);

	    free (ExecProcessingMode.Name);
	    return -1;
	}

	sprintf (ep, "%s=%s%c%s", ExecProcessingMode.Name, argv[1],
		 ExecProcessingMode.FieldSep, argv[2]);

/* Stick it in the environment */

	if (putenv (ep))
	{
	    free (ExecProcessingMode.Name);
	    return -1;
	}

	argv[1] = ExecProcessingMode.Name;
	argv[2] = (char *)NULL;
    }

/* Start off on the search path for the executable file */

    res = (FindLocationOfExecutable (p_name, argv[0]))
		? ExecuteProgram (p_name, argv) : -1;

    serrno = errno;

/* Restore the current directory */

    SetCurrentDrive (tolower(*cdirectory) - 'a' + 1);

    if (chdir (&cdirectory[2]) != 0)
    {
	fputs ("system: WARNING - current directory reset to /\n", stderr);
	chdir ("/");
    }

/* Clean up environment.  Restore original value */

    if (ExecProcessingMode.Flags & EP_ENVIRON)
    {
	len = strlen (ExecProcessingMode.Name) + 2;

	if (SaveEV != (char *)NULL)
	    len += strlen (SaveEV);

	if ((ep = malloc (len)) != (char *)NULL)
	{
	    sprintf (ep, "%s=", ExecProcessingMode.Name,
		     (SaveEV == (char *)NULL) ? "" : SaveEV);

	    putenv (ep);
	}

/* Release memory */

	if (SaveEV != (char *)NULL)
	    free (SaveEV);

	free (ExecProcessingMode.Name);
    }

    errno = serrno;
    return res;
}

/*
 * Exec or spawn the program ?
 */

static int F_LOCAL ExecuteProgram (char	*path, char **parms)
{
    int			res;
#if !defined (OS2) && !defined (__OS2__)
    char		*ep;
    unsigned int	size = 0;
    int			serrno;
    unsigned int	c_cur = (unsigned int)(_psp - 1);
    struct MCB_list	*mp = (struct MCB_list *)((unsigned long)c_cur << 16L);
#endif

/* Check to see if the file exists */

    strcpy (path_line, path);

/* Check we have access to the file */

    if (access (path_line, F_OK) != 0)
	return -1;

/* Process the command line.  If no swapping, we have executed the program */

    res = BuildCommandLine (path_line, parms);

#if defined (OS2) || defined (__OS2__)
    SetWindowName ();
    ClearExtendedLineFile ();
    return res;
#else
    if ((Swap_Mode == SWAP_OFF) || res)
    {
	ClearExtendedLineFile ();
	return res;
    }

/* Find the length of the swap area */

    while ((mp = (struct MCB_list *)((unsigned long)c_cur << 16L))->MCB_type
	    == MCB_CON)
    {
	if (c_cur >= 0x9ffe)
	    break;

	if ((mp->MCB_pid != _psp) && (mp->MCB_pid != 0) &&
	    (mp->MCB_type != MCB_END))
	{
	    ClearExtendedLineFile ();
	    FATAL_ERROR ("Fatal: Memory chain corrupt");
	    return -1;
	}

	c_cur += (mp->MCB_len + 1);
	size += mp->MCB_len + 1;
    }

/*
 * Convert swap size from paragraphs to 16K blocks.
 */

    if (size == 0)
	size = mp->MCB_len + 1;

    SW_Blocks = (size / 0x0400) + 1;

/* OK Now we've set up the FCB's, command line and opened the swap file.
 * Get some sys info for the swapper and execute my little assembler
 * function to swap us out
 */

/* Ok - 3 methods of swapping */

/* If expanded memory - try that */

    if ((Swap_Mode & SWAP_EXPAND) && Get_EMS_Driver ())
    {
	SW_Mode = 3;			/* Set Expanded memory swap	*/

	if ((res = SwapToMemory (SWAP_EXPAND)) != -2)
	    return res;
    }

    if ((Swap_Mode & SWAP_EXTEND) && Get_XMS_Driver ())
    {
	SW_Mode = (SW_fp == -1) ? 2 : 4;/* Set Extended memory or XMS driver */

	if ((res = SwapToMemory (SWAP_EXTEND)) != -2)
	    return res;

	Swap_Mode &= ~SWAP_EXTEND;
    }

/* Try the disk if available */

    if (Swap_Mode & SWAP_DISK)
    {
	SW_fp = open ((ep = GenerateTemporaryFileName ()), O_SMASK);

	if (SW_fp < 0)
	    return SwapToDiskError (ENOSPC, NoSwapFiles);

/* Save the swap file name ? */

	if ((Swap_File = strdup (ep)) == (char *)NULL)
	    Swap_File = (char *)NULL;

	SW_Mode = 1;			/* Set Disk file swap		*/

/* Execute the program */

	res = SpawnProcess ();

/* Close the extended command line file */

	ClearExtendedLineFile ();

/* Check for out of swap space */

	if (res == -2)
	    return SwapToDiskError (errno, "Swap file write failed");

/* Close the swap file */

	serrno = errno;
	ClearSwapFile ();
	errno = serrno;

/* Return the result */

	return res;
    }

/* No swapping available - give up */

    ClearExtendedLineFile ();
    fputs ("system: WARNING - All Swapping methods failed\n", stderr);
    Swap_Mode = SWAP_OFF;
    errno = ENOSPC;
    return -1;
#endif
}

/*
 * Find the location of an executable and return it's full path
 * name
 */

static int F_LOCAL FindLocationOfExecutable (char *FullPath, char *name)
{
    char	*sp;			/* Path pointers	*/
    char	*ep;
    char	*xp;			/* In file name pointers */
    char	*xp1;
    int		i;

/* Scan the path for an executable */

    sp = ((strchr (name, '/') != (char *)NULL) ||
	  (*(name + 1) == ':')) ? ""
				: GET_ENVIRON ("PATH");

    if (sp == (char *)NULL)
        sp = "";

    do
    {
	sp = BuildNextFullPathName (sp, name, FullPath);
	ep = &FullPath[strlen (FullPath)];

/* Get start of file name */

	if ((xp1 = strrchr (FullPath, '/')) == (char *)NULL)
	    xp1 = FullPath;

	else
	    ++xp1;

/* Look up all 5 types */

	for (i = 0; i < EXTENSION_COUNT; i++)
	{
	    strcpy (ep, Extensions[i]);

	    if (access (FullPath, F_OK) == 0)
	    {

/* If no extension or .sh extension, check for shell script */

		if ((xp = strrchr (xp1, '.')) == (char *)NULL)
		    continue;

		else if (!stricmp (xp, ".exe") ||
			 !stricmp (xp, ".com"))
		    return EXTENSION_EXECUTABLE;
	    }
	}
    } while (sp != (char *)NULL);

/* Not found */

    errno = ENOENT;
    return EXTENSION_NOT_FOUND;
}


/*
 * Check the program type
 */

static void F_LOCAL CheckProgramMode (char *Pname)
{
    char		*sp, *sp1;		/* Line pointers	*/
    int			nFields;
    char		*SPname;
    LineFields		LF;
    long		value;

/* Set not found */

    ExecProcessingMode.Flags = EP_NONE;

/* Check not a function */

    if ((Pname == (char *)NULL) ||
        ((sp = GET_ENVIRON ("EXTENDED_LINE")) == FAIL_ENVIRON))
        return;

/* Get some memory for the input line and the file name */

    sp1 = ((sp1 = strrchr (Pname, '/')) == (char *)NULL) ? Pname : sp1 + 1;

    if (*(sp1 + 1) == ':')
	sp1 += 2;

    if ((SPname = strdup (sp1)) == (char *)NULL)
        return;

    if ((LF.Line = calloc (LF.LineLength = 200, 1)) == (char *)NULL)
    {
	free ((void *)SPname);
	return;
    }

/* Remove terminating .exe etc */

    if ((sp1 = strrchr (SPname, '.')) != (char *)NULL)
        *sp1 = 0;

/* Open the file */

    if ((LF.FP = fopen (sp, "r")) == (FILE *)NULL)
    {
	free ((void *)LF.Line);
	free ((void *)SPname);
	return;
    }

/* Initialise the internal buffer */

    LF.Fields = (Word_B *)NULL;

/* Scan for the file name */

    while ((nFields = ExtractFieldsFromLine (&LF)) != -1)
    {
        if (nFields < 2)
            continue;

/* Remove terminating .exe etc */

	if ((sp = strrchr (LF.Fields->w_words[0], '.')) != (char *)NULL)
	    *sp = 0;

        if (stricmp (LF.Fields->w_words[0], SPname))
            continue;

/* What type? */

	if (stricmp (LF.Fields->w_words[1], "unix") == 0)
	    ExecProcessingMode.Flags = (unsigned int )(EP_UNIXMODE |
				CheckForCommonOptions (&LF, 2));

	else if (stricmp (LF.Fields->w_words[1], "dos") == 0)
	    ExecProcessingMode.Flags = (unsigned int )(EP_DOSMODE |
				CheckForCommonOptions (&LF, 2));

/* Must have a valid name and we can get memory for it */

	else if ((stricmp (LF.Fields->w_words[1], "environ") == 0) &&
		 (nFields >= 3) &&
		 (!IsValidVariableName (LF.Fields->w_words[2])) &&
		 ((ExecProcessingMode.Name =
		     strdup (LF.Fields->w_words[2])) != (char *)NULL))
	{
	    ExecProcessingMode.Flags = EP_ENVIRON;
	    ExecProcessingMode.FieldSep = 0;

	    if ((nFields >= 4) &&
		ConvertNumericValue (LF.Fields->w_words[3], &value, 0))
		ExecProcessingMode.FieldSep = (unsigned char)value;

	    if (!ExecProcessingMode.FieldSep)
		ExecProcessingMode.FieldSep = ' ';
	}

	else
	    ExecProcessingMode.Flags = CheckForCommonOptions (&LF, 1);

        break;
    }

    fclose (LF.FP);
    free ((void *)LF.Line);
    free ((void *)SPname);
}

/*
 * Check for common fields
 */

static unsigned int F_LOCAL CheckForCommonOptions (LineFields *LF, int Start)
{
    unsigned int	Flags = 0;
    int			i, j;

    if (LF->Fields == (Word_B *)NULL)
	return 0;

    for (i = Start; i < LF->Fields->w_nword; i++)
    {
	for (j = 0; j < COMMON_FIELD_COUNT; ++j)
	{
	    if (!stricmp (LF->Fields->w_words[i], CommonFields[j].Name))
	    {
		Flags |= CommonFields[j].Flag;
		break;
	    }
	}
    }

    return Flags;
}

/*
 * Convert path format to DOS format
 */

static void F_LOCAL ConvertPathToFormat (char *path)
{
    while ((path = strchr (path, '/')) != (char *)NULL)
	*path = '\\';
}

/*
 * Set the current drive number and return the number of drives.
 */

static void F_LOCAL SetCurrentDrive (unsigned int drive)
{
#if defined (OS2) || defined (__OS2__)
    DosSelectDisk ((USHORT)drive);

#elif defined (__TURBOC__)
   return setdisk (cdr - 1);

#else
    unsigned int	ndrives;

    _dos_setdrive (drive, &ndrives);
#endif
}


/*
 * Clear Extended command line file
 */

static void F_LOCAL ClearExtendedLineFile (void)
{
    if (Extend_file != (char *)NULL)
    {
	unlink (Extend_file);
	free ((void *)Extend_file);
    }

    Extend_file = (char *)NULL;
}


/* Set up command line.  If the EXTENDED_LINE variable is set, we create
 * a temporary file, write the argument list (one entry per line) to the
 * this file and set the command line to @<filename>.  If NOSWAPPING, we
 * execute the program because I have to modify the argument line
 */

static int F_LOCAL BuildCommandLine (char *path, char **argv)
{
    char		**pl = argv;
    FILE		*fd;
    bool		found;
    char		*new_args[3];
#if defined (OS2) || defined (__OS2__)
    char		cmd_line[NAME_MAX + PATH_MAX + 3];
#endif

/* Translate process name to MSDOS format */

    if (GenerateFullExecutablePath (path) == (char *)NULL)
	return -1;

/* Extended command line processing */

    Extend_file = (char *)NULL;		/* Set no file		*/
    found = C2bool ((ExecProcessingMode.Flags & EP_UNIXMODE) ||
		    (ExecProcessingMode.Flags & EP_DOSMODE));

/* Set up a blank command line */

    cmd_line[0] = 0;
    cmd_line[1] = 0x0d;

/* If there are no parameters, or they fit in the DOS command line
 * - start the process */

    if ((*(++pl) == (char *)NULL) || CheckParameterLength (pl))
	return StartTheProcess (path, argv);

/* If we can use an alternative approach - indirect files, use it */

    else if (found)
    {
	char	**pl1 = pl;

/* Check parameters don't contain a re-direction parameter */

	while (*pl1 != (char *)NULL)
	{
	    if (**(pl1++) == '@')
	    {
		found = FALSE;
		break;
	    }
	}

/* If we find it - create a temporary file and write the stuff */

	if ((found) &&
	    ((fd = fopen (Extend_file = GenerateTemporaryFileName (),
			  "w")) != (FILE *)NULL))
	{
	    if ((Extend_file = strdup (Extend_file)) == (char *)NULL)
		Extend_file = (char *)NULL;

/* Copy to end of list */

	    do
	    {
		if (!WriteToExtendedFile (fd, *pl))
		    return -1;
	    } while (*(pl++) != (char *)NULL);

/* Set up cmd_line[1] to contain the filename */

#if defined (OS2) || defined (__OS2__)
	    memset (cmd_line, 0, NAME_MAX + PATH_MAX + 3);
#else
	    memset (cmd_line, 0, CMD_LINE_MAX);
#endif
	    cmd_line[1] = ' ';
	    cmd_line[2] = '@';
	    strcpy (&cmd_line[3], Extend_file);
	    cmd_line[0] = (char)(strlen (Extend_file) + 2);

/* Correctly terminate cmd_line in no swap mode */

#if !defined (OS2) && !defined (__OS2__)
	    if (!(ExecProcessingMode.Flags & EP_NOSWAP) &&
		(Swap_Mode != SWAP_OFF))
		cmd_line[cmd_line[0] + 2] = 0x0d;
#endif

/* If the name in the file is in upper case - use \ for separators */

	    if (ExecProcessingMode.Flags & EP_DOSMODE)
		ConvertPathToFormat (&cmd_line[2]);

/* OK we are ready to execute */

#if !defined (OS2) && !defined (__OS2__)
	    if ((ExecProcessingMode.Flags & EP_NOSWAP) ||
		(Swap_Mode == SWAP_OFF))
	    {
#endif
		new_args[0] = *argv;
		new_args[1] = &cmd_line[2];
		new_args[2] = (char *)NULL;

		return StartTheProcess (path, new_args);
#if !defined (OS2) && !defined (__OS2__)
	    }

	    else
		return 0;
#endif
	}
    }

    return -1;
}

/*
 * Set up the Window name.  Give up if it does not work.
 */

#if defined (OS2) || defined (__OS2__)
static void F_LOCAL SetWindowName (void)
{
    HSWITCH		hswitch;
    SWCNTRL		swctl;

    if (!(hswitch = WinQuerySwitchHandle (0, getpid ())))
	return;

    if (WinQuerySwitchEntry (hswitch, &swctl))
	return;

    strncpy (swctl.szSwtitle, WINDOW_NAME, sizeof (swctl.szSwtitle));
    swctl.szSwtitle[sizeof (swctl.szSwtitle) - 1] = 0;

    WinChangeSwitchEntry (hswitch, &swctl);
}
#endif


/*
 * Extract the next path from a string and build a new path from the
 * extracted path and a file name
 */

static char * F_LOCAL BuildNextFullPathName
			    (char *path_s,	/* Path string		*/
			     char *file_s,	/* File name string	*/
			     char *output_s)	/* Output path		*/
{
    char	*s = output_s;
    int		fsize = 0;

    while (*path_s && (*path_s != ';') && (fsize++ < FFNAME_MAX))
	*s++ = *path_s++;

    if ((output_s != s) && (*(s - 1) != '/') && (fsize++ < FFNAME_MAX))
	*s++ = '/';

    *s = 0;

    if (file_s != (char *)NULL)
	strncpy (s, file_s, FFNAME_MAX - fsize);

    output_s[FFNAME_MAX - 1] = 0;

    return (*path_s ? ++path_s : (char *)NULL);
}

/*
 * Get and process configuration line:
 *
 * <field> = <field> <field> # comment
 *
 * return the number of fields found.
 */

int	ExtractFieldsFromLine (LineFields *fd)
{
    char	*cp;
    int		len;
    Word_B	*wb;

    if (fgets (fd->Line, fd->LineLength - 1, fd->FP) == (char *)NULL)
    {
	fclose (fd->FP);
	return -1;
    }

/* Remove the EOL */

    if ((cp = strchr (fd->Line, '\n')) != (char *)NULL)
	*cp = 0;

/* Remove the comments at end */

    if ((cp = strchr (fd->Line, '#')) != (char *)NULL)
	*cp = 0;

/* Extract the fields */

    if (fd->Fields != (Word_B *)NULL)
	fd->Fields->w_nword = 0;

    fd->Fields = SplitString (fd->Line, fd->Fields);

    if (WordBlockSize (fd->Fields) < 2)
	return 1;

/* Check for =.  At end of first field? */

    wb = fd->Fields;
    len = strlen (wb->w_words[0]) - 1;

    if (wb->w_words[0][len] == '=')
	wb->w_words[0][len] = 0;

/* Check second field for just being equals */

    else if (wb->w_nword < 3)
	wb->w_nword = 1;

    if (strcmp (wb->w_words[1], "=") == 0)
    {
	(wb->w_nword)--;
	memcpy (wb->w_words + 1, wb->w_words + 2,
		(wb->w_nword - 1) * sizeof (void *));
    }

/* Check the third field for starting with an equals */

    else if (*(wb->w_words[2]) == '=')
	strcpy (wb->w_words[2], wb->w_words[2] + 1);

    else
	wb->w_nword = 1;

    return wb->w_nword;
}

/*
 * Split the string up into words
 */

static Word_B * F_LOCAL SplitString (char *string, Word_B *wb)
{
    while (*string)
    {
	while (isspace (*string))
	    *(string++) = 0;

	if (*string)
	    wb = AddWordToBlock (string, wb);

	while (!isspace (*string) && *string)
	    ++string;
    }

    return wb;
}

/*
 * Generate a temporary filename
 */

static char * F_LOCAL GenerateTemporaryFileName (void)
{
    static char	tmpfile[FFNAME_MAX];
    char	*tmpdir;	/* Points to directory prefix of pipe	*/
    static int	temp_count = 0;
    char	*sep = "/";

/* Find out where we should put temporary files */

    if (((tmpdir = GET_ENVIRON ("TMP")) == FAIL_ENVIRON) &&
	((tmpdir = GET_ENVIRON ("HOME")) == FAIL_ENVIRON) &&
	((tmpdir = GET_ENVIRON ("TMPDIR")) == FAIL_ENVIRON))
	tmpdir = ".";

    if (strchr ("/\\", tmpdir[strlen (tmpdir) - 1]) != (char *)NULL)
	sep = "";

/* Get a unique temporary file name */

    while (1)
    {
	sprintf (tmpfile, "%s%ssht%.5u.tmp", tmpdir, sep, temp_count++);

	if (access (tmpfile, F_OK) != 0)
	    break;
    }

    return tmpfile;
}

/*
 * Write a command string to the extended file
 */

static bool F_LOCAL WriteToExtendedFile (FILE *fd, char *string)
{
    char	*sp = string;
    char	*cp = string;
    bool	WriteOk = TRUE;

    if (string == (char *)NULL)
    {
	if (fclose (fd) != EOF)
	    return TRUE;

	WriteOk = FALSE;
    }

    else if (strlen (string))
    {

/* Write the string, converting newlines to backslash newline */

	while (WriteOk && (cp != (char *)NULL))
	{
	    if ((cp = strchr (sp, '\n')) != (char *)NULL)
		*cp = 0;

	    if (fputs (sp, fd) == EOF)
		WriteOk = FALSE;

	    else if (cp != (char *)NULL)
		WriteOk = C2bool (fputs ("\\\n", fd) != EOF);

	    sp = cp + 1;
	}
    }

    if (WriteOk && (fputc ('\n', fd) != EOF))
	return TRUE;

    fclose (fd);
    ClearExtendedLineFile ();
    errno = ENOSPC;
    return FALSE;
}

/*
 * Convert the executable path to the full path name
 */

static char * F_LOCAL GenerateFullExecutablePath (char *path)
{
    char		cpath[PATH_MAX + 6];
    char		npath[PATH_MAX + NAME_MAX + 6];
    char		n1path[PATH_MAX + 6];
    char		*p;
    int			drive;

/* Get path in DOS format */

    ConvertPathToFormat (path);

#if defined (OS2) || defined (__OS2__)
    if (!IsHPFSFileSystem (path))
	strupr (path);
#else
    strupr (path);
#endif

/* Get the current path */

    getcwd (cpath, PATH_MAX + 3);
    strcpy (npath, cpath);

/* In current directory ? */

    if ((p = strrchr (path, '\\')) == (char *)NULL)
    {
	 p = path;

/* Check for a:program case */

	 if (*(p + 1) == ':')
	 {
	    p += 2;

/* Get the path of the other drive */

	   S_getcwd (npath, tolower (*path) - 'a' + 1);
	 }
    }

/* In root directory */

    else if ((p - path) == 0)
    {
	++p;
	strcpy (npath, "x:\\");
	*npath = *path;
	*npath = *cpath;
    }

    else if (((p - path) == 2) && (*(path + 1) == ':'))
    {
	++p;
	strcpy (npath, "x:\\");
	*npath = *path;
    }

/* Find the directory */

    else
    {
	*(p++) = 0;

/* Change to the directory containing the executable */

	drive = (*(path + 1) == ':') ? tolower (*path) - 'a' + 1
#if defined (OS2) || defined (__OS2__)
					    : _getdrive ();
#else
					    : 0;
#endif

/* Save the current directory on this drive */

	S_getcwd (n1path, drive);

/* Find the directory we want */

	if (chdir (path) < 0)
	    return (char *)NULL;

	S_getcwd (npath, drive);		/* Save its full name */
	chdir (n1path);				/* Restore the original */

/* Restore our original directory */

	if (chdir (cpath) < 0)
	    return (char *)NULL;
    }

    if (npath[strlen (npath) - 1] != '\\')
	strcat (npath, "\\");

    strcat (npath, p);
    return strcpy (path, npath);
}

/*
 * Execute or spawn the process
 */

static int F_LOCAL StartTheProcess (char *path, char **argv)
{
#if !defined (OS2) && !defined (__OS2__)
    return ((ExecProcessingMode.Flags & EP_NOSWAP) || (Swap_Mode == SWAP_OFF))
		? spawnv (P_WAIT, path, ProcessSpaceInParameters (argv))
		: 0;
#else
    return OS2_DosExecProgram (path, argv);
#endif
}


/*
 * Special OS/2 processing for execve and spawnv
 */

#if defined (OS2) || defined (__OS2__)
static int F_LOCAL OS2_DosExecProgram (char *Program, char **argv)
{
#  if defined (__OS2__)
    APIRET		ErrorCode;
#  else
    USHORT		ErrorCode;
#  endif
    void		(*sig_int)();		/* Interrupt signal	*/
    RESULTCODES		rescResults;
    PID			pidProcess;
    PID			pidWait;
    char		*OS2Environment;
    char		*OS2Arguments;
    char		**SavedArgs;
    int			argc;

/* Set the error module to null */

    *FailName = 0;


/* Build OS/2 argument string
 *
 * 1.  Count the number of arguments.
 * 2.  Add 2 for: 1 - NULL; 2 - argv[0]; 3 - the stringed arguments
 * 3.  save copy of arguments at offset 2.
 * 4.  On original argv, process white space and convert to OS2 Argument string.
 * 5.  Set up program name at offset 2 as ~argv
 * 6.  Convert zero length args to "~" and args beginning with ~ to ~~
 * 7.  Build OS2 Argument string (at last).
 */

    argc = CountNumberArguments (argv);

    if ((SavedArgs = (char **)malloc ((argc + 3) * sizeof (char *)))
    		   == (char **)NULL)
	return -1;

    memcpy (SavedArgs + 2, argv, (argc + 1) * sizeof (char *));

/* Set program name at Offset 0 */

    SavedArgs[0] = *argv;

/* Build OS2 Argument string in Offset 1 */

    if ((SavedArgs[1] = BuildOS2String (ProcessSpaceInParameters (&argv[1]),
					' ')) == (char *)NULL)
	return -1;

/* Set up the new arg 2 - ~ + programname */

    if ((SavedArgs[2] = InsertCharacterAtStart (*argv)) == (char *)NULL)
	return -1;

/* Convert zero length args and args starting with a ~ */

    for (argc = 3; SavedArgs[argc] != (char *)NULL; argc++)
    {
	if (strlen (SavedArgs[argc]) == 0)
	    SavedArgs[argc] = "~";

	else if ((*SavedArgs[argc] == '~') &&
		 ((SavedArgs[argc] = InsertCharacterAtStart (SavedArgs[argc]))
			== (char *)NULL))
	    return -1;
    }

/* Build the full argument list */

    if ((OS2Arguments = BuildOS2String (SavedArgs, 0)) == (char *)NULL)
	return -1;

/* Build OS/2 environment string */

    if ((OS2Environment = BuildOS2String (environ, 0)) == (char *)NULL)
	return -1;

/* Change signal handling */

    sig_int = signal (SIGINT, SIG_DFL);

/* Exec it */

    ErrorCode = DosExecPgm (FailName, sizeof (FailName), EXEC_ASYNCRESULT,
			    OS2Arguments,
			    OS2Environment,
			    &rescResults, Program);

    signal (SIGINT, SIG_IGN);

/* If the process started OK, wait for it */

    if ((ErrorCode == NO_ERROR) || (ErrorCode == ERROR_INTERRUPT))
    {
	pidWait = rescResults.codeTerminate;

/* Re-try on interrupted system calls - and kill the child.  Why, because
 * sometimes the kill does not go to the child
 */

	while ((ErrorCode = DosCwait (DCWA_PROCESS, DCWW_WAIT,
				      &rescResults, &pidProcess,
				      pidWait)) == ERROR_INTERRUPT)
	    DosKillProcess (DKP_PROCESS, pidWait);
    }

    signal (SIGINT, sig_int);

/*
 * What happened ?.  OS/2 Error - Map to UNIX errno.  Why can't people
 * write libraries right??  Or provide access at a high level.  We could
 * call _dosret if the interface did not require me to write more
 * assembler.
 */

    if (ErrorCode == 0)
	return rescResults.codeResult;

    fprintf (stderr, "system: DosExecPgm failed OS2 Error %d (%s)\n",
    	     ErrorCode, FailName);

    switch (ErrorCode)
    {
	case ERROR_NO_PROC_SLOTS:
	    errno = EAGAIN;
	    return -1;

	case ERROR_NOT_ENOUGH_MEMORY:
	    errno = ENOMEM;
	    return -1;

	case ERROR_ACCESS_DENIED:
	case ERROR_DRIVE_LOCKED:
	case ERROR_LOCK_VIOLATION:
	case ERROR_SHARING_VIOLATION:
	    errno = EACCES;
	    return -1;

	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_PROC_NOT_FOUND:
	    errno = ENOENT;
	    return -1;

	case ERROR_BAD_ENVIRONMENT:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_FUNCTION:
	case ERROR_INVALID_ORDINAL:
	case ERROR_INVALID_SEGMENT_NUMBER:
	case ERROR_INVALID_STACKSEG:
	case ERROR_INVALID_STARTING_CODESEG:
	    errno = EINVAL;
	    return -1;

	case ERROR_TOO_MANY_OPEN_FILES:
	    errno = EMFILE;
	    return -1;

	case ERROR_INTERRUPT:
	case ERROR_NOT_DOS_DISK:
	case ERROR_SHARING_BUFFER_EXCEEDED:
	    errno = EIO;
	    return -1;

	default:
	    errno = ENOEXEC;
	    return -1;
    }
}

/*
 * Insert character at start of string
 *
 * Return NULL or new string
 */

static char * F_LOCAL InsertCharacterAtStart (char *string)
{
    char	*cp;

    if ((cp = (char *)malloc (strlen (string) + 2)) == (char *)NULL)
	return (char *)NULL;

    *cp = '~';
    strcpy (cp + 1, string);

    return cp;
}
#endif

/*
 * Convert any parameters with spaces in the to start and end with double
 * quotes.
 *
 * Under OS2, the old string is NOT released.
 */

static char ** F_LOCAL ProcessSpaceInParameters (char **argv)
{
    char	**Start = argv;
    char	*new;
    char	*cp;
    char	*sp;
    int		Count;

/* If noquote set, don't even try */

    if (ExecProcessingMode.Flags & EP_NOQUOTE)
	return Start;

/* Protect parameters with TABS */

    while (*argv != (char *)NULL)
    {
        if ((strchr (*argv, ' ') != (char *)NULL)	||
	    (strchr (*argv, '\t') != (char *)NULL)	||
	    (strlen (*argv) == 0))
	{

/* Count number of Double quotes in the parameter */

	    Count = CountDoubleQuotes (*argv);

/* Get some memory - give up update if out of memory */

	    if ((new = malloc (strlen (*argv) + (Count * 2) + 3))
	    	     == (char *)NULL)
	        return Start;

	    *new = '"';

/* Escape any double quotes in the string */

	    cp = *argv;
	    sp = new + 1;

	    while (*cp)
	    {
		if (*cp == '"')
		{
		    *(sp++) = '\\';
		    *(sp++) = *(cp++);
		}

		else if (*cp != '\\')
		    *(sp++) = *(cp++);

/* Handle escapes - count them */

		else
		{
		    *(sp++) = *(cp++);

		    if (*cp == '"')
		    {
			*(sp++) = '\\';
			*(sp++) = '\\';
		    }

		    else if (*cp == 0)
		    {
			*(sp++) = '\\';
			break;
		    }

		    *(sp++) = *(cp++);
		}
	    }

/* Append the terminating double quotes */

	    strcpy (sp, "\"");
	    *argv = new;
	}

/* Check for any double quotes */

	else if (Count = CountDoubleQuotes (*argv))
	{

/* Got them - escape them */

	    if ((new = malloc (strlen (*argv) + Count + 1)) == (char *)NULL)
	        return Start;

/* Copy the string, escaping DoubleQuotes */

	    cp = *argv;
	    sp = new;

	    while (*sp = *(cp++))
	    {
		if (*sp == '"')
		{
		    *(sp++) = '\\';
		    *sp = '"';
		}

		sp++;
	    }

	    *argv = new;
	}

/* Next parameter */

	argv++;
    }

    return Start;
}

/*
 * Word Block Functions
 *
 * Add a new word to a Word Block or list
 */

static Word_B * F_LOCAL AddWordToBlock (char *wd, Word_B *wb)
{

/* Do we require more space ? */

    if ((wb == (Word_B *)NULL) || (wb->w_nword >= wb->w_bsize))
    {
	int	NewCount = (wb == (Word_B *)NULL) ? 32 : wb->w_nword * 2;

	if (wb == (Word_B *)NULL)
	    wb = calloc (1, (NewCount * sizeof (char *)) + sizeof (Word_B));

	else
	    wb = realloc (wb, (NewCount * sizeof (char *)) + sizeof (Word_B));

	if (wb == (Word_B *)NULL)
	    return (Word_B *)NULL;

	wb->w_bsize = NewCount;
    }

/* Add to the list */

    wb->w_words[wb->w_nword++] = (void *)wd;
    return wb;
}

/*
 * Get the number of words in a block
 */

static int F_LOCAL WordBlockSize (Word_B *wb)
{
    return (wb == (Word_B *)NULL) ? 0 : wb->w_nword;
}

/*
 * Build the OS2 format <value>\0<value>\0 etc \0
 */

static char * F_LOCAL BuildOS2String (char **Array, char sep)
{
    int		i = 0;
    int		Length = 0;
    char	*Output;
    char	*sp, *cp;

/* Find the total data length */

    while ((sp = Array[i++]) != (char *)NULL)
	Length += strlen (sp) + 1;

    Length += 2;

    if ((Output = malloc (Length)) == (char *)NULL)
	return (char *)NULL;

/* Build the new string */

    i = 0;
    sp = Output;

/* Build the string */

    while ((cp = Array[i++]) != (char *)NULL)
    {
	while (*sp = *(cp++))
	    ++sp;

	if (!sep || (Array[i] != (char *)NULL))
	    *(sp++) = sep;
    }

    *sp = 0;
    return Output;
}

/*
 * Check Parameter line length
 *
 * Under OS2, we don't build the command line.  Just check it.
 */

static bool F_LOCAL CheckParameterLength (char **argv)
{
    int		CmdLineLength;
    char	*CommandLine;
    bool	RetVal;
    char	**SavedArgs = argv;

/* Check for special case.  If there are any special characters and we can
 * use UNIX mode, use it
 */

    if (ExecProcessingMode.Flags & EP_UNIXMODE)
    {
	while (*argv != (char *)NULL)
	{
	    if (strpbrk (*(argv++), "\"'") != (char *)NULL)
		return FALSE;
	}
    }

/*
 * Do any parameter conversion - adding quotes or backslashes, but don't
 * update argv.
 */

    CmdLineLength = CountNumberArguments (argv = SavedArgs);

    if ((SavedArgs = (char **)malloc ((CmdLineLength + 1) * sizeof (char *)))
		== (char **)NULL)
	return FALSE;

/* Save a copy of the argument addresses */

    memcpy (SavedArgs, argv, (CmdLineLength + 1) * sizeof (char *));

/* Build the command line */

    if ((CommandLine = BuildOS2String (ProcessSpaceInParameters (SavedArgs),
				       ' ')) == (char *)NULL)
    {
	free (SavedArgs);
	return FALSE;
    }

/* Check command line length */

    if ((CmdLineLength = strlen (CommandLine)) >= CMD_LINE_MAX - 2)
    {
	errno = E2BIG;
	RetVal = FALSE;
    }

/* Terminate the line */

    else
    {
#if !defined (OS2) && !defined (__OS2__)
	strcpy (cmd_line + 1, CommandLine);
	cmd_line[CmdLineLength + 1] = 0x0d;
	cmd_line[0] = (char)CmdLineLength;
#endif
	RetVal = TRUE;
    }

    free (SavedArgs);
    free (CommandLine);

    return RetVal;
}

/*
 * Count DoubleQuotes
 */

static int F_LOCAL CountDoubleQuotes (char *string)
{
    int		Count = 0;

    while ((string = strchr (string, '"')) != (char *)NULL)
    {
	Count++;
	string++;
    }

    return Count;
}

/*
 * Get a valid numeric value
 */

static bool F_LOCAL ConvertNumericValue (char *string, long *value, int base)
{
    char	*ep;

    *value = strtol (string, &ep, base);

    return C2bool (!*ep);
}

/*
 * Is this a valid variable name
 */

static bool F_LOCAL IsValidVariableName (char *s)
{
    if (!isalpha (*s) && (*s != '_'))
	return *s;

    while (*s && (isalnum (*s) || (*s == '_')))
	++s;

    return C2bool (*s);
}

/*
 * Count the number of entries in an array
 */

static int F_LOCAL CountNumberArguments (char **wp)
{
    int		Count = 0;

    while (*(wp++) != (char *)NULL)
	Count++;

    return Count;
}

/*
 * OS2 does not require swapping
 */

/*
 * Get the XMS Driver information
 */

#if !defined (OS2) && !defined (__OS2__)
static bool F_LOCAL Get_XMS_Driver (void)
{
    union REGS		or;
    struct SREGS	sr;
    unsigned int	SW_EMsize;	/* Number of extend memory blks	*/

/* Get max Extended memory pages, and convert to 16K blocks.  If Extended
 * memory swapping disabled, set to zero
 */

    SW_fp = -1;				/* Set EMS/XMS handler not	*/
					/* defined			*/

/* Is a XMS memory driver installed */

    or.x.ax = 0x4300;
    int86 (0x2f, &or, &or);

    if (or.h.al != 0x80)
    {
	or.x.ax = 0x8800;
	int86 (0x15, &or, &or);
	SW_EMsize = or.x.ax / 16;

	if ((SW_EMsize <= SW_Blocks) ||
	     (((long)(SW_EMstart - 0x100000L) +
	      ((long)(SW_Blocks - SW_EMsize) * 16L * 1024L)) < 0L))
	    return XMS_error (MS_Space, 0);

	else
	    return TRUE;
    }

/* Get the driver interface */

    or.x.ax = 0x4310;
    int86x (0x2f, &or, &or, &sr);
    SW_XMS_Driver = (void (*)())((unsigned long)(sr.es) << 16L | or.x.bx);

/* Support for version 3 of XMS driver */

    if ((SW_XMS_Gversion () & 0xff00) < 0x0200)
	return XMS_error ("WARNING - %s Version < 2", 0);

    else if (SW_XMS_Available () < (SW_Blocks * 16))
	return XMS_error (MS_Space, 0);

    else if ((SW_fp = SW_XMS_Allocate (SW_Blocks * 16)) == -1)
	return XMS_error (MS_emsg, errno);

    return TRUE;
}

/* Get the EMS Driver information */

static bool F_LOCAL Get_EMS_Driver (void)
{
    union REGS		or;
    struct SREGS	sr;
    char		*sp;

/* Set EMS/XMS handler not defined */

    SW_fp = -1;

    or.x.ax = 0x3567;
    intdosx (&or, &or, &sr);

    sp = (char *)((unsigned long)(sr.es) << 16L | 10L);

/* If not there - disable */

    if (memcmp ("EMMXXXX0", sp, 8) != 0)
	return EMS_error ("WARNING - %s not available", 0);

    or.h.ah = 0x40;			/* Check status			*/
    int86 (0x67, &or, &or);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

/* Check version greater than 3.2 */

    or.h.ah = 0x46;
    int86 (0x67, &or, &or);

    if ((or.h.ah != 0) || (or.h.al < 0x32))
	return EMS_error ("WARNING - %s Version < 3.2", 0);

/*  get page frame address */

    or.h.ah = 0x41;
    int86 (0x67, &or, &or);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

    SW_EMSFrame = or.x.bx;		/* Save the page frame		*/

/* Get the number of pages required */

    or.h.ah = 0x43;
    or.x.bx = SW_Blocks;
    int86 (0x67, &or, &or);

    if (or.h.ah == 0x088)
	return EMS_error (MS_Space, 0);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

/* Save the EMS Handler */

    SW_fp = or.x.dx;

/* save EMS page map */

    or.h.ah = 0x47;
    or.x.dx = SW_fp;
    int86 (0x67, &or, &or);

    return (or.h.ah != 0) ? EMS_error (MS_emsg, or.h.ah) : TRUE;
}

/* Print EMS error message */

static bool F_LOCAL EMS_error (char *s, int v)
{
    fputs ("system: ", stderr);
    fprintf (stderr, s, "EMS", v);
    fputc ('\n', stderr);

    Swap_Mode &= ~(SWAP_EXPAND);
    EMS_Close ();
    return FALSE;
}

/* Print XMS error message */

static bool F_LOCAL XMS_error (char *s, int v)
{
    fputs ("system: ", stderr);
    fprintf (stderr, s, "XMS", v);
    fputc ('\n', stderr);

    Swap_Mode &= ~(SWAP_EXTEND);
    XMS_Close ();
    return FALSE;
}

/* If the XMS handler is defined - close it */

static int F_LOCAL XMS_Close (void)
{
    int		res = 0;

/* Release XMS page */

    if (SW_fp != -1)
	res = SW_XMS_Free (SW_fp);

    SW_fp = -1;
    return res;
}

/* If the EMS handler is defined - close it */

static int F_LOCAL EMS_Close (void)
{
    union REGS		or;
    int			res = 0;

    if (SW_fp == -1)
	return 0;

/* Restore EMS page */

    or.h.ah = 0x48;
    or.x.dx = SW_fp;
    int86 (0x67, &or, &or);

    if (or.h.ah != 0)
	res = or.h.al;

    or.h.ah = 0x45;
    or.x.dx = SW_fp;
    int86 (0x67, &or, &or);

    SW_fp = -1;
    return (res) ? res : or.h.ah;
}

/*
 * Clear Disk swap file file
 */

static void F_LOCAL ClearSwapFile (void)
{
    close (SW_fp);

    if (Swap_File != (char *)NULL)
    {
	unlink (Swap_File);
	free ((void *)Swap_File);
    }

    Swap_File = (char *)NULL;
}

/*
 * Swap to disk error
 */

static int F_LOCAL SwapToDiskError (int error, char *ErrorMessage)
{
/* Clean up */

    ClearSwapFile ();
    Swap_Mode &= (~SWAP_DISK);
    FATAL_ERROR (ErrorMessage);
    errno = error;
    return -1;
}

/*
 * Swap to memory
 */

static int F_LOCAL	SwapToMemory (int mode)
{
    int		res;
    int		cr;

/* Swap and close memory handler */

    res = SpawnProcess ();

    cr = (SW_Mode != 3) ? XMS_Close () : EMS_Close ();

    if ((res != -2) && cr)		/* Report Close error ?		*/
    {
	res = -2;
	errno = cr;
    }

    if (res == -2)
	(SW_Mode != 3) ? XMS_error (SwapFailed, errno)
		       : EMS_error (SwapFailed, errno);

    else
    {
	ClearExtendedLineFile ();
	return res;
    }

/* Failed - disabled */

    Swap_Mode &= (~mode);
    return res;
}

/*
 * Set Interrupt handling vectors - moved from sh0.asm
 */

static int F_LOCAL	SpawnProcess (void)
{
    void	(interrupt far *SW_I00_V) (void);	/* Int 00 address */
    void	(interrupt far *SW_I23_V) (void);	/* Int 23 address*/
    int			res;
#if 0
    union REGS		r;
    unsigned char	Save;

    r.x.ax = 0x3300;
    intdos (&r, &r);
    Save = r.h.al;
    fprintf (stderr, "Break Status: %s (%u)\n", Save ? "on" : "off", Save);

    r.x.ax = 0x3301;
    r.h.dl = 1;
    intdos (&r, &r);
    fprintf (stderr, "Break Status: %s (%u)\n", r.h.al ? "on" : "off", r.h.al);
#endif

/*
 * Save current vectors
 */

#if defined (__TURBOC__)
    SW_I00_V = (void (far *)())getvect (0x00);
    SW_I23_V = (void (far *)())getvect (0x23);
#else
    SW_I00_V = _dos_getvect (0x00);
    SW_I23_V = _dos_getvect (0x23);
#endif

/*
 * Set In shell flag for Interrupt 23, and set to new interrupts
 */

    SW_I23_InShell = 0;

#if defined (__TURBOC__)
    setvect (0x23, SW_Int23);
    setvect (0x00, SW_Int00);
#else
    _dos_setvect (0x23, SW_Int23);
    _dos_setvect (0x00, SW_Int00);
#endif

    res = SA_spawn (environ);

/*
 * Restore interrupt vectors
 */

#if defined (__TURBOC__)
    setvect (0x00, SW_I00_V);
    setvect (0x23, SW_I23_V);
#else
    _dos_setvect (0x00, SW_I00_V);
    _dos_setvect (0x23, SW_I23_V);
#endif

#if 0
    r.x.ax = 0x3300;
    intdos (&r, &r);
    fprintf (stderr, "Break Status: %s (%u)\n", r.h.al ? "on" : "off", r.h.al);
    r.x.ax = 0x3301;
    r.h.dl = Save;
    intdos (&r, &r);
#endif

/*
 * Check for an interrupt
 */

    if (SW_intr)
	raise (SIGINT);

    return res;
}
#endif

/*
 * Local get current directory function to do some additional checks
 *
 * Assumes that PathName is a string of length PATH_MAX + 6.
 */

static void F_LOCAL S_getcwd (char *PathName, int drive)
{
#if defined (__TURBOC__)
    *(strcpy (PathName, "x:/")) = drive + 'a' - 1;
#endif

#if defined (__TURBOC__)
    (drive) ? getcurdir (drive, PathName + 3)
    	    : getcwd (PathName, PATH_MAX + 4);
#else
    (drive) ? _getdcwd (drive, PathName, PATH_MAX + 4)
    	    : getcwd (PathName, PATH_MAX + 4);
#endif

    PathName[PATH_MAX + 5] = 0;

/* Convert to Unix format */

#if (OS_VERSION != OS_DOS)
    if (!IsHPFSFileSystem (PathName))
	strlwr (PathName);
#else
    strlwr (PathName);
#endif
}
