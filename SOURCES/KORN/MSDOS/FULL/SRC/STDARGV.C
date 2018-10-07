/* MS-DOS stdargv Function
 *
 * MS-DOS stdargv - Copyright (c) 1990,4 Data Logic Limited.
 *
 * This code is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/stdargv.c,v 2.9 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: stdargv.c,v $
 *	Revision 2.9  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.8  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.7  1994/01/20  14:51:43  istewart
 *	Release 2.3 Beta 1
 *
 *	Revision 2.6  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.5  1993/08/25  16:04:22  istewart
 *	Add support for MSC 6 which requires osmajor and osminor to be set up
 *	for MSDOS.
 *
 *	Revision 2.4  1993/06/14  10:59:58  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.3  1993/06/02  09:54:21  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.2  1992/12/14  11:12:37  istewart
 *	BETA 215 Fixes and 2.1 Release
 *
 *	Revision 2.1  1992/11/06  10:03:44  istewart
 *	214 Beta test updates
 *
 *	Revision 2.0  1992/04/13  17:39:09  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 *
 *
 * MODULE DEFINITION:
 *
 * This function expandes the command line parameters in a UNIX like manner.
 * Wild character *?[] are allowed in file names. @filename causes command lines
 * to be read from filename.  Strings between " or ' are not expanded.  All
 * entries in the array are malloced.
 *
 * This function replaces the standard Microsoft C5.1 & C6.0 C Run-Time
 * start up line processing function (_setargv in stdargv.obj).
 *
 * For OS/2 2.x, this function replaces the standard IBM C Set/2 C Run-Time
 * start up line processing function (_setuparg in stdargv.obj).
 *
 * For Turbo C, this function replaces the standard Borland C Run-Time
 * start up line processing function (_setargv).
 *
 * For WatCom C, this function replaces the standard WatCom C Run-Time
 * start up line processing function (__Init_Argv).
 *
 * To get the OS2 16-bit version, compile with -DOS2
 * To get the OS2 32-bit version, compile with -D__OS2__
 * To get the NT 32-bit version, compile with -DWIN32
 *
 * Author:
 *	Ian Stewartson
 *	Data Logic, Queens House, Greenhill Way
 *	Harrow, Middlesex  HA1 1YR, UK.
 *	istewart@datlog.co.uk or ukc!datlog!istewart
 */

#include <sys/types.h>			/* MS-DOS type definitions      */
#include <sys/stat.h>			/* File status definitions	*/
#include <stdio.h>			/* Standard I/O delarations     */
#include <stdlib.h>			/* Standard library functions   */
#include <errno.h>			/* Error number declarations    */

#if defined (OS2) || defined (__OS2__)
#  define INCL_DOSSESMGR
#  define INCL_DOSMEMMGR
#  define INCL_DOSPROCESS
#  define INCL_DOSMODULEMGR
#  define INCL_WINSWITCHLIST
#  include <os2.h>			/* OS2 functions declarations   */
#elif (WIN32)
#  include <windows.h>			/* WIN NT functions declarations   */
#else
#  include <dos.h>			/* DOS functions declarations   */
#  include <bios.h>			/* BIOS functions declarations  */
#endif

#include <ctype.h>			/* Character type declarations  */
#include <string.h>			/* String library functions     */
#include <limits.h>			/* String library functions     */
#include <fcntl.h>			/* File Control Declarations    */
#include <dirent.h>			/* Direction I/O functions	*/
#include <unistd.h>
#include <glob.h>			/* Globbing functions		*/

/*
 *  DATA DEFINITIONS:
 */

#define MAX_LINE	160		/* Max line length		*/
#define S_ENTRY		sizeof (char *)

/*
 *  DATA DECLARATIONS:
 */

static void	_Ex_CommandLine (char *);	/* Expand file		*/
static void	_Ex_ExpandIndirectFile (char *);
static char	*_Ex_GetSpace (int, char *);	/* Get space		*/
static void	_Ex_AddArgument (char *);	/* Add argument		*/
static char	*_Ex_SkipWhiteSpace (char *);	/* Skip spaces		*/
static char	*_Ex_ConvertToUnixFormat (char *);
static void	_Ex_ExpandField (char *);	/* Split file name	*/
static void	_Ex_FatalError (int, char *, char *);
static char	*_Ex_ConvertEnvVariables (char *);
static char	*_EX_OutOfMemory = "%s: %s\n";

#if defined (OS2) || defined (__OS2__)
static void	_Ex_ProcessEMXArguments (char *);
static void	_Ex_SetUpWindowName (void);
#elif (WIN32)
static void	_Ex_SetUpWindowName (void);
#endif

/*
 * Command Line pointers
 */

#if defined (__OS2__)
#  define ARG_ARRAY	_argv
#  define ARG_COUNT	_argc
#  define ENTRY_POINT	_setuparg
#elif defined (__TURBOC__)
#  define ARG_ARRAY	_C0argv
#  define ARG_COUNT	_C0argc
#  define ENTRY_POINT	_setargv
#elif defined (__WATCOMC__)
#  define ARG_ARRAY	___Argv
#  define ARG_COUNT	___Argc
#  define ENTRY_POINT	__Init_Argv
#else
#  define ARG_ARRAY	__argv
#  define ARG_COUNT	__argc
#  define ENTRY_POINT	_setargv
#endif

extern void	ENTRY_POINT (void);
extern char	**ARG_ARRAY; 		/* Current argument address	*/
extern int	ARG_COUNT; 		/* Current argument count	*/

/*
 * General OS independent start of the command line and program name.
 */

#if defined (__OS2__)
char 		*_ACmdLine;
char 		*_APgmName; 		/* Program name			*/
#else
#  if defined (OS2)
extern ushort far _aenvseg;
extern ushort far _acmdln;
#  endif
char far	*_ACmdLine;
char far	*_APgmName; 		/* Program name			*/
#endif

/*
 *  MODULE ABSTRACT: _setargv
 *
 *  UNIX like command line expansion
 */

/*
 * OS/2 2.x (32-bit) version
 */


#if defined (__OS2__)
void	ENTRY_POINT (void)
{
    char	*argvp;
    APIRET	rc;
    PTIB	ptib;
    PPIB	ppib;
    char	*MName;

/* Get the command line and program name */

    if (rc = DosGetInfoBlocks (&ptib, &ppib))
    {
        fprintf (stderr, "DosGetInfoBlocks: Cannot find command line (%d)\n",
		 rc);
        exit (1);
    }

    if ((MName = malloc (PATH_MAX + NAME_MAX + 3)) == (char *)NULL)
	_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

    if (rc = DosQueryModuleName (ppib->pib_hmte, NAME_MAX + PATH_MAX + 2,
    				 MName))
    {
        fprintf (stderr, "DosQueryModuleName: Cannot get program name (%d)\n",
		 rc);
        exit (1);
    }

/* Save the program name and process the command line */

    _APgmName = MName;
    _Ex_ProcessEMXArguments (ppib->pib_pchcmd);

/* Set up the Window name.  Give up if it does not work.  */

    _Ex_SetUpWindowName ();
    _Ex_AddArgument ((char *)NULL);
    --ARG_COUNT;
}

#elif defined (OS2)

/*
 * OS/2 1.x (16-bit) version
 */

void	ENTRY_POINT (void)
{
#  if !defined (M_I86LM) && !defined (__LARGE__)
    char far		*s; 		/* Temporary string pointer    	*/
    char		buf[MAX_LINE];	/* Temporary space		*/
    char		*cp;
#  endif
    char far		*argvp = (char far *)((((long)_aenvseg) << 16));
    ushort		off = _acmdln;

    while (--off)
    {
	if (argvp[off - 1] == 0)
 	    break;
    }

/* Add program name */

    _APgmName =  &argvp[off];

    if (argvp[_acmdln] == 0)
    {
#  if !defined (M_I86LM) && !defined (__LARGE__)
	cp = buf;
	s = _APgmName;
	while (*(cp++) = *(s++))
	    continue;

	_Ex_AddArgument (_Ex_ConvertToUnixFormat (buf));
#  else
	_Ex_AddArgument (_Ex_ConvertToUnixFormat (_APgmName));
#  endif
    }

    else
    {
	argvp += _acmdln;

#  if !defined (M_I86LM) && !defined (__LARGE__)
	cp = buf;
	s = argvp;
	while (*(cp++) = *(s++))
	    continue;

	off = strlen (buf);
	_Ex_AddArgument (_Ex_ConvertToUnixFormat (buf));
	argvp += off + 1;

	cp = buf;
	s = argvp;
	while (*(cp++) = *(s++))
	    continue;

	_Ex_CommandLine (buf);
#  else
	_Ex_ProcessEMXArguments (argvp);
#  endif
    }

/* Set up the Window name.  Give up if it does not work.  */

    _Ex_SetUpWindowName ();
    _Ex_AddArgument ((char *)NULL);
    --ARG_COUNT;
}

#elif (WIN32)

/*
 * Windows NT version
 */

void	ENTRY_POINT (void)
{
    char	*cline = GetCommandLine ();
    char	*MName;

#ifdef TEST
    printf ("Got Command line as |%s|\n", cline);
#endif

/* Get the command line and program name */

    if (!*cline)
    {
	if ((MName = malloc (MAX_PATH)) == (char *)NULL)
	    _Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

	if (!GetModuleFileName (0, MName, MAX_PATH))
	{
	    fprintf (stderr, "GetModuleFileName: Cannot get program name (%ld)\n",
		     GetLastError ());
	    exit (1);
	}

	cline = MName;

#ifdef TEST
        printf ("Got command program name |%s|\n", cline);
#endif
    }

/* Save the program name and process the command line */

    _Ex_CommandLine (_ACmdLine = cline);
    _Ex_AddArgument ((char *)NULL);
    _APgmName = _Ex_ConvertToUnixFormat (ARG_ARRAY[0]);

/* Set up the Window name.  Give up if it does not work.  */

    _Ex_SetUpWindowName ();
    --ARG_COUNT;
}

#else

/*
 * MSDOS version
 */

void	ENTRY_POINT (void)
{
    char far		*s; 		/* Temporary string pointer    	*/
    union REGS		r;

#  if !defined (M_I86LM) && !defined (__LARGE__)
    char		buf[MAX_LINE];	/* Temporary space		*/
#  endif
					/* Set up pointer to command line */
    unsigned int	envs = *(int far *)((((long)_psp) << 16) + 0x02cL);
    unsigned int	Length;

/* For reasons that escape me, MSC 6.0 does sets up _osmajor and _osminor
 * in the command line parser!
 */

   r.h.ah = 0x30;
   intdos (&r, &r);
   _osminor = r.h.ah;
   _osmajor = r.h.al;

/* Check the length */

    _ACmdLine = (char far *)((((long)_psp) << 16) + 0x080L);

    if ((Length = (unsigned int)*(_ACmdLine++)) > 127)
	Length = 127;

    _ACmdLine[Length] = 0;

/* Command line can be null or 0x0d terminated - convert to null */

    if ((s = strchr (_ACmdLine, 0x0d)) != (char *)NULL)
	*s = 0;

#ifdef TEST
    printf ("_psp line = <%s>\n", _ACmdLine);
#endif

/* Set up global parameters and expand */

    ARG_COUNT = 0;

/* Get the program name */

    if ((_osmajor <= 2) || (envs == 0))
	s = "unknown";

/* In the case of DOS 3+, we look in the environment space */

    else
    {
	s = (char far *)(((long)envs) << 16);

	while (*s)
	{
	    while (*(s++) != 0)
		continue;
	}

	s += 3;
    }

/* Add the program name		*/

    _APgmName = s;

#  if !defined (M_I86LM) && !defined (__LARGE__)
    cp = buf;
    while (*(cp++) = *(s++))
	continue;

    _Ex_AddArgument (_Ex_ConvertToUnixFormat (buf));

    s  = _ACmdLine;
    cp = buf;
    while (*(cp++) = *(s++))
	continue;

    _Ex_CommandLine (buf);
#  else
    _Ex_AddArgument (_Ex_ConvertToUnixFormat (s));
    _Ex_CommandLine (_ACmdLine);
#  endif

    _Ex_AddArgument ((char *)NULL);
    --ARG_COUNT;
}
#endif

/*
 * Expand the DOS Command line
 */

static void	_Ex_CommandLine (argvp)
char		*argvp;			/* Line to expand    		*/
{
    char	*spos;			/* End of string pointer	*/
    char	*cpos;			/* Start of string pointer	*/
    char	*tpos;
    char	*fn;			/* Extracted file name string	*/

/* Search for next separator */

    spos = argvp;

    while (*(cpos = _Ex_SkipWhiteSpace (spos)))
    {

/* Extract string argument */

	if ((*cpos == '"') || (*cpos == '\''))
	{
	    spos = cpos + 1;

	    do
	    {
		if ((spos = strchr (spos, *cpos)) != (char *)NULL)
		{
		    spos++;
		    if (spos[-2] != '\\')
			break;
		}

		else
		    spos = &spos[strlen (cpos)];

	    } while (*spos);

	    fn	= _Ex_GetSpace (spos - cpos - 2, cpos + 1);

/* Remove escaped characters */

	   tpos = fn;

	   while ((tpos = strchr (tpos, *cpos)) != (char *)NULL)
	       strcpy (tpos - 1, tpos);
	}

/* Extract normal argument */

	else
	{
	    spos = cpos;
	    while (!isspace (*spos) && *spos)
		spos++;

	    fn = _Ex_GetSpace (spos - cpos, cpos);
	}

/* Process argument */

	if (*cpos != '\'')
	    fn = _Ex_ConvertEnvVariables (fn);

	switch (*cpos)
	{
	    case '@':		/* Expand file	    			*/
		_Ex_ExpandIndirectFile (fn);
		break;

	    case '"':		/* Expand string	    		*/
	    case '\'':
		_Ex_AddArgument (fn);
		break;

	    default:		/* Expand field	    			*/
		_Ex_ExpandField (fn);
	}

	free (fn);
    }
}

/* Expand an indirect file Argument */

static void	_Ex_ExpandIndirectFile (file)
char		*file;		/* Expand file name	    		*/
{
    FILE    	*fp;		/* File descriptor	    		*/
    char	*EoLFound;	/* Pointer				*/
    int		c_maxlen = MAX_LINE;
    char	*line;		/* Line buffer		    		*/
    char	*eolp;

/* If file open fails, expand as a field */

#ifdef __OS2__
    if ((fp = fopen (file + 1, "r")) == NULL)
#else
    if ((fp = fopen (file + 1, "rt")) == NULL)
#endif
	_Ex_FatalError (errno, "%s: Cannot open re-direct file - %s (%s)\n",
			file + 1);

/* Grab some memory for the line */

    if ((line = malloc (c_maxlen)) == (char *)NULL)
	_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

/* For each line in the file, remove EOF characters and add argument */

    while (fgets (line, c_maxlen, fp) != (char *)NULL)
    {
	EoLFound = strchr (line, '\n');
	eolp = line;

/* Handle continuation characters */

	while (TRUE)
	{

/* Check for a continuation character */

	    if (((EoLFound = strchr (eolp, '\n')) != (char *)NULL) &&
		(*(EoLFound - 1) == '\\'))
	    {
		*(EoLFound - 1) = '\n';
		*EoLFound = 0;
		EoLFound = (char *)NULL;
	    }

	    else if (EoLFound == (char *)NULL)
		EoLFound = strchr (line, 0x1a);

	    if (EoLFound != (char *)NULL)
		break;

/* Find the end of the line */

	    c_maxlen = strlen (line);

/* Get some more space */

	    if ((line = realloc (line, c_maxlen + MAX_LINE)) == (char *)NULL)
		_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

	    eolp = &line[c_maxlen];

	    if (fgets (eolp, MAX_LINE, fp) == (char *)NULL)
		break;
	}

/* Terminate the line and add it to the argument list */

	if (EoLFound != (char *)NULL)
	    *EoLFound = 0;

	_Ex_AddArgument (line);
    }

    if (ferror(fp))
	_Ex_FatalError (errno, "%s: %s (%s)\n", file + 1);

    free (line);
    fclose (fp);

/* Delete tempoary files */

    if (((line = strrchr (file + 1, '.')) != (char *)NULL) &&
	(stricmp (line, ".tmp") == 0))
	unlink (file + 1);			/* Delete it		*/
}

/* Get space for an argument name */

static char	*_Ex_GetSpace (length, in_s)
int		length;			/* String length                */
char		*in_s;                  /* String address		*/
{
    char	*out_s;			/* Malloced space address       */

    if ((out_s = malloc (length + 1)) == (char *)NULL)
	_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

/* Copy string for specified length */

    strncpy (out_s, in_s, length);
    out_s[length] = 0;

    return (out_s);
}

/* Append an argument to the array */

static void	_Ex_AddArgument (Argument)
char		*Argument;			/* Argument to add		*/
{
    if (ARG_COUNT == 0)
	ARG_ARRAY = (char **)malloc (50 * S_ENTRY);

    else if ((ARG_COUNT % 50) == 0)
	ARG_ARRAY = (char **)realloc (ARG_ARRAY, (ARG_COUNT + 50) * S_ENTRY);

    if (ARG_ARRAY == (char **)NULL)
	_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

    if (Argument == (char *)NULL)
	ARG_ARRAY[ARG_COUNT++] = (char *)NULL;

    else
	ARG_ARRAY[ARG_COUNT++] = _Ex_GetSpace (strlen (Argument), Argument);
}

/*  Skip over spaces */

static char	*_Ex_SkipWhiteSpace (a)
char		*a;			/* String start address		*/
{
    while (isspace(*a))
        a++;

    return (a);
}

/* Convert name to Unix format */

static char	*_Ex_ConvertToUnixFormat (a)
char		*a;
{
    char	*sp = a;

    while ((a = strchr (a, '\\')) != (char *)NULL)
	*(a++) = '/';

#if !defined (OS2) && !defined (__OS2__) && !defined (WIN32)
    return strlwr (sp);
#else
    if (!IsHPFSFileSystem (sp))
	strlwr (sp);

    return sp;
#endif

}

/* Find the location of meta-characters.  If no meta, add the argument and
 * return NULL.  If meta characters, return position of end of directory
 * name.  If not multiple directories, return -1
 */

static void	_Ex_ExpandField (file)
char		*file;
{
    int		i = 0;
    glob_t	gp;

    if (strpbrk (file, "?*[]\\") == (char *)NULL)
    {
	_Ex_AddArgument (file);
	return;
    }

    if (glob (file, GLOB_NOCHECK, (int (_CDECL *)(char *, int))NULL , &gp))
	_Ex_FatalError (ENOMEM, _EX_OutOfMemory, (char *)NULL);

    i = 0;

    while (i < gp.gl_pathc)
	_Ex_AddArgument (gp.gl_pathv[i++]);

    globfree (&gp);
}

/* Fatal errors */

static void	_Ex_FatalError (ecode, format, para)
int		ecode;
char		*format;
char		*para;
{
    fprintf (stderr, format, "stdargv", strerror (ecode), para);
    exit (1);
}

/* Process Environment - note that field is a malloc'ed field */

static char	*_Ex_ConvertEnvVariables (field)
char		*field;
{
    char	*sp, *cp, *np, *ep;
    char	save;
    int		b_flag;

    sp = field;

/* Replace any $ strings */

    while ((sp = strchr (sp, '$')) != (char *)NULL)
    {

/* If ${...}, find the terminating } */

	if (*(cp = ++sp) == '{')
	{
	    b_flag = 1;
	    ++cp;

	    while (*cp && (*cp != '}'))
		cp++;
	}

/* Else must be $..., find the terminating non-alphanumeric */

	else
	{
	    b_flag = 0;

	    while (isalnum (*cp))
		cp++;
	}

/* Grab the environment variable */

	if (cp == sp)
	    continue;

/* Get its value */

	save = *cp;
	*cp = 0;
	ep = getenv (sp + b_flag);
	*cp = save;

	if (ep != (char *)NULL)
	{
	    np = _Ex_GetSpace (strlen(field) - (cp - sp) + strlen (ep) - 1, field);
	    strcpy (&np[sp - field - 1], ep);
	    free (field);
	    strcpy ((sp = &np[strlen(np)]), cp + b_flag);
	    field = np;
	}
    }

    return field;
}

/*
 * Handle EMX style arguments
 */

#if defined (OS2) || defined (__OS2__)
static void	_Ex_ProcessEMXArguments (char *argvp)
{
    char	*cp;
    char	*s = argvp;

    _Ex_AddArgument (_Ex_ConvertToUnixFormat (argvp));
    argvp += strlen (argvp) + 1;
    _ACmdLine = argvp;
#ifdef TEST
    printf ("argvp line = <%s>\n", _ACmdLine);
#endif

/*
 * Add support in OS2 version for Eberhard Mattes EMX interface to commands.
 */

    if ((*argvp) && (*(cp = argvp + strlen (argvp) + 1) == '~') &&
	(strcmp (s, _Ex_ConvertToUnixFormat (cp + 1)) == 0))
    {

/* Skip over the program name at string 2 to the start of the first
 * argument at string 3
 */

	argvp += strlen (argvp) + 1;
	argvp += strlen (argvp) + 1;

	while (*argvp)
	{
	    cp = (*argvp == '~') ? argvp + 1 : argvp;

	    if (*cp == '@')
		_Ex_ExpandIndirectFile (cp);

	    else
		_Ex_AddArgument (cp);

	    argvp += strlen (argvp) + 1;
	}
    }

    else
	_Ex_CommandLine (argvp);
}

/*
 * Set up the Window Name
 */

static void	_Ex_SetUpWindowName (void)
{
    HSWITCH		hswitch;
    SWCNTRL		swctl;
    char		*cp;

    if (((hswitch = WinQuerySwitchHandle (0, getpid ()))) &&
	(!WinQuerySwitchEntry (hswitch, &swctl)))
    {
	if ((cp = strrchr (ARG_ARRAY[0], '/')) == (char *)NULL)
	    cp = ARG_ARRAY[0];

	else
	    ++cp;

	strncpy (swctl.szSwtitle, cp, MAXNAMEL);
	swctl.szSwtitle[MAXNAMEL] = 0;

	if ((cp = strrchr (swctl.szSwtitle, '.')) != (char *)NULL)
	    *cp = 0;

	WinChangeSwitchEntry (hswitch, &swctl);
    }
}
#endif

/*
 * Windows NT version
 */

#ifdef WIN32
static void	_Ex_SetUpWindowName (void)
{
    char		*cp;

    if ((cp = strrchr (ARG_ARRAY[0], '/')) == (char *)NULL)
	cp = ARG_ARRAY[0];

    else
	++cp;

    SetConsoleTitle (cp);
}
#endif


/*
 * Test main program
 */

#ifdef TEST
int	main (int argc, char **argv)
{
    int		i;

    printf ("_ACmdLine = <%s>\n", _ACmdLine);

    for (i = 0; i < argc; i++)
	printf ("Arg %d = |%s|\n", i, argv[i]);

    return 0;
}
#endif
