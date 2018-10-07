/* MS-DOS GLOB (3C) FUNCTION
 *
 * MS-DOS GLOB FUNCTION - Copyright (c) 1990,4 Data Logic Limited.
 *
 * This code is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/glob.c,v 2.3 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: glob.c,v $
 * Revision 2.3  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 2.2  1994/02/01  10:25:20  istewart
 * Release 2.3 Beta 2, including first NT port
 *
 * Revision 2.1  1993/06/14  10:59:32  istewart
 * More changes for 223 beta
 *
 * Revision 2.0  1992/04/13  17:39:09  Ian_Stewartson
 * MS-Shell 2.0 Baseline release
 *
 */

#include <sys/types.h>			/* MS-DOS type definitions      */
#include <sys/stat.h>			/* File status definitions	*/
#include <stdio.h>			/* Standard I/O delarations     */
#include <stdlib.h>			/* Standard library functions   */
#include <string.h>			/* String library functions     */
#include <limits.h>			/* String library functions     */
#include <dirent.h>			/* Direction I/O functions	*/
#include <ctype.h>			/* Character types function	*/
#include <unistd.h>			/* Other functions		*/


#ifdef __TURBOC__
#  include <alloc.h>			/* Malloc functions		*/
#  include <dir.h>			/* Dos directory functions	*/
#else
#  include <malloc.h>			/* Malloc functions		*/
#endif

#include <glob.h>

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
#  if defined (OS2) || defined (__OS2__)

#    define INCL_DOSDEVICES
#    define INCL_DOSMISC
#    include <os2.h>			/* OS2 functions declarations       */

#    if defined (__OS2__)
#      define DISABLE_HARD_ERRORS	DosError (FERR_DISABLEHARDERR)
#      define ENABLE_HARD_ERRORS	DosError (FERR_ENABLEHARDERR)
#    else
#      define DISABLE_HARD_ERRORS	DosError (HARDERROR_DISABLE)
#      define ENABLE_HARD_ERRORS	DosError (HARDERROR_ENABLE)
#    endif

#  elif defined (WIN32)
#  include <windows.h>
#  define DISABLE_HARD_ERRORS	SetErrorMode (0)
#  define ENABLE_HARD_ERRORS	SetErrorMode (SEM_FAILCRITICALERRORS | \
					      SEM_NOOPENFILEERRORBOX);

#  else
#    include <bios.h>			/* DOS BIOS functions		*/
#    include <dos.h>			/* DOS functions		*/
#    define DISABLE_HARD_ERRORS
#    define ENABLE_HARD_ERRORS
#  endif
#endif

/*
 * OS/2 2.x has these missing
 */

#ifndef S_IFMT
#  define	S_IFMT	0xf000	/* type of file				*/
#endif

#ifndef S_ISDIR
#  define S_ISDIR(m)	((((m) & S_IFMT) == S_IFDIR))
#endif

/*
 * Functions
 */

static int	_GP_SortCompare		_PROTO ((char **, char **));
static int	_GP_ExpandField		_PROTO ((char *, char *, glob_t *));
static int	_GP_ExpandMetaCharacters _PROTO ((char *, glob_t *));
static int	_GP_AddArgument		_PROTO ((char *, glob_t *));
static bool	_GP_MatchPattern	_PROTO ((char *, char *));
static bool	_GP_access		_PROTO ((char *, int));
static bool	_GP_stat		_PROTO ((char *, struct stat *));

static char	*_GP_MetaChars = "?*[\\";
static char	*_GP_NullString = "";

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
static	int	_GP_GetNumberofFloppyDrives (void);

#  if defined (OS2) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
static void	 _dos_setdrive (unsigned int, unsigned int *);
static void	 _dos_getdrive (unsigned int *);
#  endif

static char	*_GP_CheckForMultipleDrives	_PROTO ((char *));
#endif

/*
 * There appears to be no alloca in TurboC
 */

#ifdef __TURBOC__
#  define alloca(x)		malloc (x)
#  define alloca_free(x)	free (x)
#else
#  define alloca_free(x)
#endif

/*
 * Free up space
 */

void	globfree (gp)
glob_t	*gp;
{
    int		i = (gp->gl_flags & GLOB_DOOFFS) ? gp->gl_offs : 0;

    while (i < gp->gl_pathc)
	free (gp->gl_pathv[i++]);

    free (gp->gl_pathv);
}

/* Main search function */

int	glob (Pattern, flags, ErrorFunction, gp)
char	*Pattern;
int	flags;
int	(*ErrorFunction) _PROTO ((char *, int));
glob_t	*gp;
{
    int		ReturnValue;
    char	*PatternCopy;
    char	*cp;

/* If no append mode - initialise */

    if (!(flags & GLOB_APPEND))
    {
	gp->gl_pathc = 0;
	gp->gl_pathv = (char **)NULL;
    }

    gp->gl_flags = flags;
    gp->gl_ef = ErrorFunction;

    if ((PatternCopy = alloca (strlen (Pattern) + 1)) == (char *)NULL)
	return GLOB_NOSPACE;

/* Expand and kill environment */

    if (ReturnValue = _GP_ExpandMetaCharacters (strcpy (PatternCopy, Pattern),
						gp))
    {
	alloca_free (PatternCopy);
	return ReturnValue;
    }

/* Check for no finds.  If add value, strip out \ from the string */

    if ((gp->gl_pathc == 0) && (flags & GLOB_NOCHECK))
    {
	cp = strcpy (PatternCopy, Pattern);

	while ((cp = strpbrk (cp, "?*[")) != (char *)NULL)
	{
	    if ((cp == PatternCopy) || (*(cp - 1) != '\\'))
		cp++;

	    else
		memmove (cp - 1, cp, strlen (cp) + 1);
	}

	if (ReturnValue = _GP_AddArgument (PatternCopy, gp))
	{
	    alloca_free (PatternCopy);
	    return ReturnValue;
	}
    }

/* Terminate string */

    if ((gp->gl_pathc != 0) &&
	(ReturnValue = _GP_AddArgument ((char *)NULL, gp)))
    {
	alloca_free (PatternCopy);
	return ReturnValue;
    }

/* Get the sort length */

    ReturnValue = (gp->gl_flags & GLOB_DOOFFS) ? gp->gl_offs : 0;

    if ((!(flags & GLOB_NOSORT)) && (gp->gl_pathc > 1))
	qsort (&gp->gl_pathv[ReturnValue], gp->gl_pathc, sizeof (char *),
	       (int (*) (const void *, const void *)) _GP_SortCompare);

    alloca_free (PatternCopy);
    return 0;
}

/* Compare function for sort */

static int	_GP_SortCompare (a1, a2)
char		**a1, **a2;
{
    return strcmp (*a1, *a2);
}

/* Expand a field if it has metacharacters in it */

static int	_GP_ExpandField (CurrentDirectoryPattern, AppendString, gp)
char		*CurrentDirectoryPattern;	/* Prefix field		*/
char		*AppendString;			/* Postfix field    	*/
glob_t		*gp;
{
    int 		i;
    int			ReturnValue = 0;	/* Return Value		*/
    char		*FullFileName;		/* Search file name	*/
    char		*FileNameStart;
    char		*MatchString;		/* Match string		*/
    DIR			*DirHandler;
    struct dirent	*CurrentDirectoryEntry;

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
    unsigned int	CurrentDrive;		/* Current drive	*/
    unsigned int	MaxDrives;		/* Max drive		*/
    unsigned int	SelectedDrive;		/* Selected drive	*/
    unsigned int	x_drive, y_drive;	/* Dummies		*/
    char		*DriveCharacter;	/* Multi-drive flag	*/
    char		SDriveString[2];

/* Convert file name to lower case */

#  if defined (OS2) || defined (__OS2__) || (WIN32)
    if (!IsHPFSFileSystem (CurrentDirectoryPattern))
	strlwr (CurrentDirectoryPattern);
#  else
    strlwr (CurrentDirectoryPattern);
#  endif

/* Search all drives ? */

    if ((DriveCharacter = _GP_CheckForMultipleDrives (CurrentDirectoryPattern))
		!= (char *)NULL)
    {
	_dos_getdrive (&CurrentDrive);	/* Get number of drives		*/
	_dos_setdrive (CurrentDrive, &MaxDrives);
	SDriveString[1] = 0;

	for (SelectedDrive = 1; SelectedDrive <= MaxDrives; ++SelectedDrive)
	{
	    _dos_setdrive (SelectedDrive, &x_drive);
	    _dos_getdrive (&y_drive);
	    _dos_setdrive (CurrentDrive, &x_drive);

/* Check to see if the second diskette drive is really there */

	    if ((_GP_GetNumberofFloppyDrives () < 2) && (SelectedDrive == 2))
		continue;

/* If the drive exists and is in our list - process it */

	    *DriveCharacter = 0;
	    *SDriveString = (char)(SelectedDrive + 'a' - 1);
	    strlwr (CurrentDirectoryPattern);

	    if ((y_drive == SelectedDrive) &&
		_GP_MatchPattern (SDriveString, CurrentDirectoryPattern))
	    {
		if ((FullFileName = alloca (strlen (DriveCharacter) + 3))
				== (char *)NULL)
		    return GLOB_NOSPACE;

		*DriveCharacter = ':';
		*FullFileName = *SDriveString;
		strcpy (FullFileName + 1, DriveCharacter);

		i = _GP_ExpandField (FullFileName, AppendString, gp);
		alloca_free (FullFileName);

		if (i)
		    return i;
	    }

	    *DriveCharacter = ':';
	}

	return 0;
    }
#endif

/* Get the path length */

    MatchString = strrchr (CurrentDirectoryPattern, '/');

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
    if ((MatchString == (char *)NULL) &&
	(*(CurrentDirectoryPattern + 1) == ':'))
	MatchString = CurrentDirectoryPattern + 1;
#endif

/* Set up file name for search */

    if ((MatchString == (char *)NULL) || (*MatchString == ':'))
    {
	if ((FullFileName = alloca (NAME_MAX + 7 +
				    strlen (AppendString))) == (char *)NULL)
	    return GLOB_NOSPACE;

	if (MatchString != (char *)NULL)
	    *(strcpy (FullFileName, "x:.")) = *CurrentDirectoryPattern;

	else
	    strcpy (FullFileName, ".");

	FileNameStart = FullFileName +
			(int)((MatchString != (char *)NULL) ? 2 : 0);
    }

/* Case of /<directory>/... */

    else if ((FullFileName = alloca (NAME_MAX + 4 + strlen (AppendString) +
			    (i = (int)(MatchString - CurrentDirectoryPattern))))
			== (char *)NULL)
	    return GLOB_NOSPACE;

    else
    {
	strncpy (FullFileName, CurrentDirectoryPattern, i);
	*((FileNameStart = FullFileName + i)) = 0;
	strcpy (FileNameStart++, "/");
    }

    MatchString = (MatchString == (char *)NULL) ? CurrentDirectoryPattern
						: MatchString + 1;

/* Search for file names */

    if ((DirHandler = opendir (FullFileName)) == (DIR *)NULL)
    {
	i = 0;

	if (((gp->gl_ef != NULL) && (*gp->gl_ef)(FullFileName, errno)) ||
	    (gp->gl_flags & GLOB_ERR))
	    i = GLOB_ABEND;

	alloca_free (FullFileName);
	return i;
    }

/* Are there any matches */

    while ((CurrentDirectoryEntry = readdir (DirHandler)) !=
	    (struct dirent *)NULL)
    {
	if ((*CurrentDirectoryEntry->d_name == '.') && (*MatchString != '.'))
	    continue;

/* Check for match */

	if (_GP_MatchPattern (CurrentDirectoryEntry->d_name, MatchString))
	{
	    strcpy (FileNameStart, CurrentDirectoryEntry->d_name);

/* If the postfix is not null, this must be a directory */

	    if (strlen (AppendString))
	    {
		struct stat		statb;
		char			*p;

/* If not a directory - go to the next file */

		if (!_GP_stat (FullFileName, &statb) ||
		    !S_ISDIR (statb.st_mode & S_IFMT))
		    continue;

/* Are there any metacharacters in the postfix? */

		if ((p = strpbrk (AppendString, _GP_MetaChars)) == (char *)NULL)
		{

/* No - build the file name and check it exists */

		    strcat (strcat (FileNameStart, "/"), AppendString);

		    if (_GP_access (FullFileName, F_OK) &&
			(ReturnValue = _GP_AddArgument (FullFileName, gp)))
			break;
		}

/* Yes - build the filename upto the start of the meta characters */

		else
		{
		    if ((p = strchr (p, '/')) != (char *)NULL)
			*(p++) = 0;

		    else
			p = _GP_NullString;

/* Build the new directory name and check it out */

		    strcat (strcat (FileNameStart, "/"), AppendString);
		    ReturnValue = _GP_ExpandField (FullFileName, p, gp);

		    if (p != _GP_NullString)
		       *(--p) = '/';

/* Check for errors */

		    if (ReturnValue)
			break;
		}
	    }

/* Process this file.  If error - terminate */

	    else if (_GP_access (FullFileName, F_OK) &&
		     (ReturnValue = _GP_AddArgument (FullFileName, gp)))
		break;
	}
    }

    closedir (DirHandler);
    alloca_free (FullFileName);
    return ReturnValue;
}

/* Find the location of meta-characters.  If no meta, add the argument and
 * return.  If meta characters, expand directory containing meta characters.
 */

static int	_GP_ExpandMetaCharacters (file, gp)
char		*file;
glob_t		*gp;
{
    char	*p;
    int		ReturnValue;

/* No metas - add to string */

    if ((p = strpbrk (file, _GP_MetaChars)) == (char *)NULL)
    {
	if (!_GP_access (file, F_OK))
	    return 0;

	return _GP_AddArgument (file, gp);
    }

/* Ok - metas, find the end of the start of the directory */

    else if ((p = strchr (p, '/')) != (char *)NULL)
	*(p++) = 0;

    else
	p = _GP_NullString;

/* Continue recusive match */

    ReturnValue = _GP_ExpandField (file, p, gp);

/* Restore if necessary */

    if (p != _GP_NullString)
       *(--p) = '/';

    return ReturnValue;
}

/* Add an argument to the stack - file is assumed to be a array big enough
 * for the file name + 2
 */

static int	_GP_AddArgument (file, gp)
char		*file;
glob_t		*gp;
{
    int		Offset;
    char	**p1;
    struct stat	FileStatus;

    Offset = gp->gl_pathc + ((gp->gl_flags & GLOB_DOOFFS) ? gp->gl_offs : 0);
    p1  = gp->gl_pathv;

/* Malloc space if necessary */

    if (gp->gl_pathc == 0)
	p1 = (char **)calloc (sizeof (char *), (50 + Offset));

    else if ((gp->gl_pathc % 50) == 0)
	p1 = (char **)realloc (p1, (Offset + 50) * (sizeof (char *)));

    if (p1 == (char **)NULL)
	return GLOB_NOSPACE;

/* OK got space */

    gp->gl_pathv = p1;

/* End of list ? */

    if (file == (char *)NULL)
	p1[Offset] = (char *)NULL;

    else
    {
	if ((gp->gl_flags & GLOB_MARK) && (file[strlen (file) - 1] != '/') &&
	    _GP_stat (file, &FileStatus) && (S_ISDIR (FileStatus.st_mode)))
	    strcat (file, "/");

	if ((p1[Offset] = strdup (file)) == (char *)NULL)
	    return GLOB_NOSPACE;

	strcpy (p1[Offset], file);

/* Increment counter */

	++(gp->gl_pathc);
    }

    return 0;
}

/* Check for multi_drive prefix */

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
static char	*_GP_CheckForMultipleDrives (prefix)
char		*prefix;
{
    if (strlen (prefix) < 2)
	return (char *)NULL;

    if (((*prefix == '*') || (*prefix == '?')) && (prefix[1] == ':'))
	return prefix + 1;

    if (*prefix != '[')
	return (char *)NULL;

    while (*prefix && (*prefix != ']'))
    {
	if ((*prefix == '\\') && (*(prefix + 1)))
	    ++prefix;

	++prefix;
    }

    return (*prefix && (*(prefix + 1) == ':')) ? prefix + 1 : (char *)NULL;
}

/*
 * Some Turboc Functions to emulate MSC functions
 */

#  if defined (__TURBOC__)
static void	 _dos_getdrive (cdp)
unsigned int	*cdp;
{
    *cdp = (unsigned int)getdisk () + 1;
}

static void	 _dos_setdrive (cdr, ndp)
unsigned int	cdr;
unsigned int	*ndp;
{
   *ndp = (unsigned int)setdisk (cdr - 1);
}
#  endif

/*
 * Some OS/2 functions to emulate the DOS functions
 */

#  if defined (OS2) || defined (__OS2__)
static void	 _dos_getdrive (cdp)
unsigned int	*cdp;
{
    USHORT	cdr;
    ULONG	ndr;

    DosQCurDisk((PUSHORT)&cdr, (PULONG) &ndr);
    *cdp = (unsigned int)cdr;
}

static void	 _dos_setdrive (cdr, ndp)
unsigned int	cdr;
unsigned int	*ndp;
{
    ULONG		ulDrives;
    USHORT		usDisk;
    int			i;

    DosSelectDisk ((USHORT)cdr);

/* Get the current disk and check that to see the number of drives */

    DosQCurDisk (&usDisk, &ulDrives);        /* gets current drive        */

    for (i = 25; (!(ulDrives & (1L << i))) && (i >= 0); --i)
	continue;

    *ndp = i + 1;
}

#  elif defined (WIN32)

static void	 _dos_getdrive (cdp)
unsigned int	*cdp;
{
    char	szCurDir [MAX_PATH];

    GetCurrentDirectory (MAX_PATH, szCurDir);

    *cdp = (unsigned int)(szCurDir[0] - 'A' + 1);
}

static void	 _dos_setdrive (cdr, ndp)
unsigned int	cdr;
unsigned int	*ndp;
{
    char		szNewDrive[3];
    DWORD		dwLogicalDrives;
    unsigned int	i;

    szNewDrive[0] = cdr + 'A' - 1;
    szNewDrive[1] = ':';
    szNewDrive[2] = 0;
    *ndp = 0;

    if (!SetCurrentDirectory (szNewDrive))
	return;

    dwLogicalDrives = GetLogicalDrives ();

    for (i = 25; (!(dwLogicalDrives & (1L << i))) && i >= 0; --i)
	continue;

    *ndp = i + 1;
}
#  endif

/* Return the number of floppy disks */

#  if defined (OS2) || defined (__OS2__)
static	int	_GP_GetNumberofFloppyDrives ()
{
    BYTE	nflop = 1;

    DosDevConfig (&nflop, DEVINFO_FLOPPY, 0);

    return nflop;
}

#  elif defined (WIN32)
static	int	_GP_GetNumberofFloppyDrives ()
{
    char	szNewDrive[4];
    DWORD	dwLogicalDrives = GetLogicalDrives();
    int		LastTest = 0;
    int		i;

    strcpy (szNewDrive, "x:\\");

/* Look at each drive until we find a non-floppy which exists */

    for (i = 0; i < 25; i++)
    {
	if (dwLogicalDrives & (1L << i))
	{
	    szNewDrive[0] = i + 'A';

	    if (GetDriveType (szNewDrive) != DRIVE_REMOVABLE)
		break;

	    LastTest = i + 1;
	}
    }

    return LastTest;
}

#  elif defined (__TURBOC__)
static	int	_GP_GetNumberofFloppyDrives ()
{
    return ((biosequip () & 0x00c0) >> 6) + 1;
}

#  else
static	int	_GP_GetNumberofFloppyDrives ()
{
    return ((_bios_equiplist () & 0x00c0) >> 6) + 1;
}
#  endif
#endif

/*
 * Pattern Matching function
 */

static bool	_GP_MatchPattern (string, pattern)
char		*string;		/* String to match                  */
char		*pattern;		/* Pattern to match against         */
{
    register int	cur_s;		/* Current string character         */
    register int	cur_p;		/* Current pattern character        */

/* Match string */

    while (cur_p = *(pattern++))
    {
	cur_s = *(string++);		/* Load current string character    */

        switch (cur_p)			/* Switch on pattern character      */
        {
            case '[':			/* Match class of characters        */
            {
                while(1)
                {
                    if (!(cur_p = *(pattern++)))
			return 0;

                    if (cur_p == ']')
			return FALSE;

                    if (cur_s != cur_p)
                    {
                        if (*pattern == '-')
                        {
                            if(cur_p > cur_s)
                                continue;

                            if (cur_s > *(++pattern))
                                continue;
                        }
                        else
                            continue;
                    }

                    break;
                }

                while (*pattern)
                {
                    if (*(pattern++) == ']')
                        break;
                }

		break;
            }

            case '?':			/* Match any character              */
            {
                if (!cur_s)
		    return FALSE;

                break;
            }

            case '*':			/* Match any number of any character*/
            {
                string--;

                do
                {
                    if (_GP_MatchPattern (string, pattern))
			return TRUE;
                }
                while (*(string++));

		return FALSE;
            }

            case '\\':			/* Next character is non-meta       */
            {
                if (!(cur_p = *(pattern++)))
		    return FALSE;
            }

            default:			/* Match against current pattern    */
            {
                if (cur_p != cur_s)
		    return FALSE;

                break;
            }
        }
    }

    return (!*string) ? TRUE : FALSE;
}

/*
 * Local Stat function to do some additional checks
 */

static bool _GP_stat (char *FileName, struct stat *Status)
{
    int		rc;

    DISABLE_HARD_ERRORS;
    rc = stat (FileName, Status);
    ENABLE_HARD_ERRORS;

    return rc ? FALSE : TRUE;
}

/*
 * Local access function to do some additional checks
 */

static bool _GP_access (char *FileName, int mode)
{
    int		rc;

    DISABLE_HARD_ERRORS;
    rc = access (FileName, mode);
    ENABLE_HARD_ERRORS;

    return rc ? FALSE : TRUE;
}

/*
 * Test program
 */

#ifdef TEST
int main (int argc, char **argv)
{
    int		i;

    for (i = 0; i < argc; i++)
	printf ("Arg %d = <%s>\n", i, argv[i]);
    
    return 0;
}
#endif
