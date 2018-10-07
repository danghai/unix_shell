/*
 * MS-DOS SHELL - Main program, memory and variable management
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Charles Forsyth
 *
 * This code is based on (in part) the shell program written by Charles
 * Forsyth and is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 * When parts of the original 2.1 shell were replaced by the Lexical
 * Analsyer written by Simon J. Gerraty (for his Public Domain Korn Shell,
 * which is also based on Charles Forsyth original idea), a number of changes
 * were made to reflect the changes Simon made to the Parse output tree.  Some
 * parts of this code in this module are based on the algorithms/ideas that
 * he incorporated into his shell, in particular interfaces to the new Lexical
 * Scanner.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh1.c,v 2.19 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh1.c,v $
 *	Revision 2.19  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.18  1994/02/23  09:23:38  istewart
 *	Beta 233 updates
 *
 *	Revision 2.17  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.16  1994/01/20  14:51:43  istewart
 *	Release 2.3 Beta 1
 *
 *	Revision 2.15  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.14  1993/12/01  11:58:34  istewart
 *	Release 226 beta
 *
 *	Revision 2.13  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.12  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.11  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.10  1993/06/16  12:55:49  istewart
 *	Fix the -s and -t options so they work
 *
 *	Revision 2.9  1993/06/14  11:00:12  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.8  1993/06/02  09:52:35  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.7  1993/02/16  16:03:15  istewart
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
 *	Revision 2.1  1992/07/10  10:52:48  istewart
 *	211 Beta updates
 *
 *	Revision 2.0  1992/04/13  17:39:09  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include "sh.h"

/* OS2 or DOS, DEBUG MEMORY or normal malloc */

#ifdef OS2_DOSALLOC
#  ifdef DEBUG_MEMORY
#    define RELEASE_MEMORY(x)	if (DosFreeSeg (SELECTOROF ((x))))	\
				    fprintf(stderr, "Memory Release error\n");
#  else
#    define RELEASE_MEMORY(x)	DosFreeSeg (SELECTOROF ((x)))
#  endif

#else
#  define RELEASE_MEMORY(x)	 free ((x))
#endif

/*
 * Structure of Malloced space to allow release of space nolonger required
 * without having to know about it.
 */

typedef struct region {
    struct region	*next;
    size_t		nbytes;
    int			area;
} s_region;

static struct region	*MemoryAreaHeader = (s_region *)NULL;

/*
 * default shell, search rules
 */

#if (OS_TYPE == OS_UNIX)
static char		*shellname = "/bin/pksh";
static char		search[] = ":/bin:/usr/bin";

extern char		**environ;
#else
static char		*shellname = "c:/bin/sh.exe";
static char		search[] = ";c:/bin;c:/usr/bin";
#endif
static char		*ymail = "You have mail";
static char		*ShellNameLiteral = "sh";
static char		*ShellOptions = "D:MPRX:abc:defghijklmnopqrtsuvwxyz0";

#if (OS_TYPE != OS_DOS)
static char		DefaultWinTitle[] = "MS Shell";
#endif

#if (OS_TYPE == OS_OS2)
#  if (OS_SIZE == OS_32)
static HEV		SessionQueueSema = 0;
static HQUEUE		SessionQueueHandler = 0;
#  else
static HSEM		SessionQueueSema = 0;
static HQUEUE		SessionQueueHandler = 0;
#  endif
#endif

static char		*tilde = "~";
static char		LIT_OSmode[] = "OSMODE";
static char		LIT_SHmode[] = "SHMODE";
static char		LIT_Dollar[] = "$";
#if (OS_TYPE == OS_DOS)
static char	*NOExit = "sh: ignoring attempt to leave lowest level shell";
#endif
static bool		ProcessingDEBUGTrap = FALSE;
static bool		ProcessingERRORTrap = FALSE;
static unsigned int	ATOE_GFlags;	/* Twalk GLOBAL flags		*/
static unsigned int	ATNE_Function;	/* Twalk GLOBAL flags		*/

/*
 * Count value for counting the number entries in an array
 */

static int		Count_Array;	/* Array size			*/
static char		*Count_Name;	/* Array name			*/

/* Integer Variables */

static struct ShellVariablesInit {
    char	*Name;			/* Name of variable		*/
    int		Status;			/* Initial status		*/
    char	*CValue;
} InitialiseShellVariables[] = {
    { LIT_COLUMNS,		STATUS_INTEGER,		"80"		},
    { HistorySizeVariable,	STATUS_INTEGER,		"100"		},
    { LineCountVariable,	STATUS_INTEGER,		"1"		},
    { LIT_LINES,		STATUS_INTEGER,		"25"		},
    { OptIndVariable,		STATUS_INTEGER,		"1"		},
    { StatusVariable,		STATUS_INTEGER,		"0"		},
    { RandomVariable,		(STATUS_EXPORT | STATUS_INTEGER),
							null		},
    { SecondsVariable,		(STATUS_EXPORT | STATUS_INTEGER),
							null		},
    { LIT_OSmode,		(STATUS_EXPORT | STATUS_CANNOT_UNSET |
				 STATUS_INTEGER),	null		},
    { LIT_SHmode,		(STATUS_EXPORT | STATUS_CANNOT_UNSET |
				 STATUS_INTEGER),	null		},
    { LIT_Dollar,		(STATUS_CANNOT_UNSET | STATUS_INTEGER),
							null		},

    { LastWordVariable,		STATUS_EXPORT,		null		},
    { PathLiteral,		(STATUS_EXPORT | STATUS_CANNOT_UNSET),
							search		},
    { IFS,			(STATUS_EXPORT | STATUS_CANNOT_UNSET),
							" \t\n"		},
    { PS1,			(STATUS_EXPORT | STATUS_CANNOT_UNSET),
							"%e$ "		},
    { PS2,			(STATUS_EXPORT | STATUS_CANNOT_UNSET),
							"> "		},
    { PS3,			STATUS_EXPORT,		"#? "		},
    { PS4,			STATUS_EXPORT,		"+ "		},
    { HomeVariableName,		STATUS_EXPORT,		null		},
    { ShellVariableName,	STATUS_EXPORT,		null		},

#if (OS_TYPE != OS_DOS)
    { WinTitleVariable, 	0,			DefaultWinTitle	},
#endif

    { (char *)NULL,		0 }
};

/*
 * List of Editor variables
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
static char		*EditorVariables[] = {
    FCEditVariable, EditorVariable, VisualVariable, (char *)NULL
};
#endif

					/* Entry directory		*/
static char	Start_directory[PATH_MAX + 6] = { 0 };
static time_t	ShellStartTime = 0;	/* Load time of shell		*/
					/* Original Interrupt 24 address */
#if (OS_TYPE == OS_DOS) && !defined (__EMX__)
#if defined (__WATCOMC__)
#    define INTERRUPT_TYPE		__interrupt __far
#  else
#    define INTERRUPT_TYPE		interrupt far
#  endif
static void	(INTERRUPT_TYPE *Orig_I24_V) (void);
#endif

#ifdef SIGQUIT
static void	(*qflag)(int) = SIG_IGN;
#endif

/* Functions */

static int F_LOCAL	RunCommands (Source *);
static unsigned char * F_LOCAL	CheckClassExpression (unsigned char *, int,
						      bool);
static bool F_LOCAL	Initialise (int, char **);
static void F_LOCAL	CheckForMailArriving (void);
static void F_LOCAL	LoadGlobalVariableList (void);
static void F_LOCAL	LoadTheProfileFiles (void);
static void F_LOCAL	ConvertUnixPathToMSDOS (char *);
static void F_LOCAL	ClearUserPrompts (void);
static void F_LOCAL	SecondAndRandomEV (char *, long);
static void F_LOCAL	SetUpParameterEV (int, char **, char *);
static bool F_LOCAL	AllowedToSetVariable (VariableList *);
static void F_LOCAL	SetUpANumericValue (VariableList *, long, int);
static void F_LOCAL	ProcessErrorExit (char *, va_list);
static char * F_LOCAL	SuppressSpacesZeros (VariableList *, char *);
static void		AddToNewEnvironment (const void *, VISIT, int);
static void		AddToOldEnvironment (const void *, VISIT, int);
static void		DeleteEnvironment (const void *, VISIT, int);
static void F_LOCAL	CheckOPTIND (char *, long);
static void F_LOCAL	CreateIntegerVariables (void);
static bool F_LOCAL	ExecuteShellScript (char *);
static void 		CountEnvironment (const void *, VISIT, int);

/*
 * Process termination
 */

#if (OS_TYPE != OS_DOS)
static void F_LOCAL	CheckForTerminatedProcess (void);

#  if (OS_TYPE == OS_NT)
static void		LookUpJobs (const void *, VISIT, int);
#  endif

#else
#  define CheckForTerminatedProcess()
#endif

/*
 * No real argv interface
 */

#if (OS_TYPE == OS_UNIX)
#define Pre_Process_Argv(a,b)
#else
static void F_LOCAL	Pre_Process_Argv (char **, int *);
#endif

/*
 * OS/2 Session Queues
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL	CheckForSessionEnd (void);
static void F_LOCAL	CreateTerminationQueues (void);
#else
#  define CheckForSessionEnd()
#  define CreateTerminationQueues()
#endif

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
static void F_LOCAL	SetEditorMode (char *);
#endif

/*
 * The Program Name and Command Line, set up by stdargv.c
 */

#if (OS_TYPE != OS_UNIX)
extern char 		*_APgmName;
extern char		*_ACmdLine;
#endif

/*
 * The main program starts here
 */

void main (int argc, char **argv)
{
    int			cflag = 0;
    int			sc;
    char		*name = *argv;
					/* Load up various parts of the	*/
					/* system			*/
    bool		OptionsRflag = Initialise (argc, argv);
    bool		OptionsXflag = FALSE;	/* -x option from	*/
						/* command line		*/
    bool		Level0Shell = FALSE;	/* Level 0 (read profile)*/
    jmp_buf		ReturnPoint;
    char		*cp;
    Source		*s;
    int			fid;
    bool		TTYInput;
    bool		PIPEInput;
    bool		RootShell = FALSE;

    SetWindowName ((char *)NULL);

/* Create Parse input */

    s = pushs (SFILE);
    s->u.file = stdin;

/* Set up start time */

    ShellStartTime = time ((time_t *)NULL);

/* Preprocess options to convert two character options of the form /x to
 * -x.  Some programs!!
 */

    if (argc > 1)
	Pre_Process_Argv (argv, &argc);

/* Save the start directory for when we exit */

    S_getcwd (Start_directory, 0);

/* Process the options */

    while ((sc = GetOptions (argc, argv, ShellOptions, GETOPT_MESSAGE)) != EOF)
    {
	switch (sc)
	{
	    case '?':
		FinalExitCleanUp (1);

	    case '0':				/* Level 0 flag for DOS	*/
		Level0Shell = TRUE;
		break;

	    case 'r':				/* Restricted		*/
		OptionsRflag = TRUE;
		break;

	    case 'c':				/* Command on line	*/
		ClearUserPrompts ();
		cflag = 1;
		s->type = SSTRING;
		s->str = OptionArgument;
		SetVariableFromString ("_cString", OptionArgument);
		break;

	    case 'q':				/* No quit ints		*/
#ifdef SIGQUIT
		qflag = SIG_DFL;
#endif
		break;


	    case 'X':
		if (!GotoDirectory (OptionArgument, GetCurrentDrive ()))
		{
		    PrintErrorMessage ("%s: bad directory", OptionArgument);
		    FinalExitCleanUp (1);
		}

		break;

	    case 'x':
		OptionsXflag = TRUE;
		break;

	    case 'M':
		ShellGlobalFlags |= FLAGS_MSDOS_FORMAT;
	        break;

	    case 'D':
		AssignVariableFromString (OptionArgument, (int *)NULL);
	        break;

	    case 'R':
		RootShell = TRUE;
		ChangeInitLoad = TRUE;	/* Change load .ini pt.		*/
		break;

#if (OS_TYPE != OS_DOS)
	    case 'P':				/* Use real pipes	*/
    		ShellGlobalFlags |= FLAGS_REALPIPES;
    		break;
#endif

	    case 's':				/* standard input	*/
	    	if (cflag)
		    PrintErrorMessage ("cannot use -s and -c together");

	    case 'i':				/* Set interactive	*/
		InteractiveFlag = TRUE;

	    default:
		if (islower (sc))
		    FL_SET (sc);
	}

/* If -s, set the argv to point to -s so that the rest of the parameters
 * get used as parameters ($digit) values.
 */

        if (FL_TEST (FLAG_POSITION))
	{
	    OptionIndex--;
	    break;
	}
    }

/* Under UNIX, check for login shell */

#if (OS_TYPE == OS_UNIX)
    if (*argv[0] == '-')
	Level0Shell = TRUE;
#endif

    argv += OptionIndex;
    argc -= OptionIndex;

/* Get configuration info */

    Configure_Keys ();

/*
 * Check for terminal input.  A special case for OS/2.  If pipe input and
 * output and no arguments, set interactive flag.
 *
 * Unset the variable after!
 */

    TTYInput  = C2bool ((IS_TTY (0) && IS_TTY (1)));
    PIPEInput = C2bool ((IS_Pipe (0) && IS_Pipe (1) &&
			 (GetVariableAsString (LIT_AllowTTY, FALSE) != null)));

    UnSetVariable (LIT_AllowTTY, -1, TRUE);

    if ((s->type == SFILE) && 
	((FL_TEST (FLAG_INTERACTIVE)) || TTYInput || PIPEInput) &&
	!cflag &&
	((argc == 0) || FL_TEST (FLAG_POSITION)))
    {
	s->type = STTY;
	FL_SET (FLAG_POSITION);
	FL_SET (FLAG_INTERACTIVE);
	InteractiveFlag = TRUE;

	if  (TTYInput)
	    PrintVersionNumber (stderr);
    }

/* Root shell - check only tty devices */

    if (RootShell)
    {
	if ((s->type != STTY) || !TTYInput)
	    PrintErrorMessage ("-R not valid on non-interactive shells");

#if (OS_TYPE == OS_DOS) && !defined (__EMX__)
	Orig_I24_V = (void (INTERRUPT_TYPE *)())NULL;
#endif
    }

/*
 * Execute commands from a file? - disable prompts
 */

    if ((s->type == SFILE) && (argc > 0) && !InteractiveFlag)
    {
	ClearUserPrompts ();
	name = *argv;

	if (((fid = S_open (FALSE, s->file = name, O_RMASK)) < 0) ||
	    ((s->u.file = ReOpenFile (ReMapIOHandler (fid), sOpenReadMode))
	                == (FILE *)NULL))
	{
	    PrintErrorMessage (LIT_Emsg, "cannot open script", name,
			       strerror (errno));
	    FinalExitCleanUp (1);
	}

/* Un-map this file descriptor from the current environment so it does not
 * get closed at the wrong times
 */
	ChangeFileDescriptorStatus (fileno (s->u.file), FALSE);


#if (OS_TYPE != OS_DOS) 
	SetVariableFromString (WinTitleVariable, name);
#endif
    }

/* Setup stderr, stdout buffering */

   setvbuf (stderr, (char *)NULL, _IONBF, 0);

   if (InteractiveFlag && !IS_TTY (1))
       setvbuf (stdout, (char *)NULL, _IOLBF, BUFSIZ);

/* Set up the $- variable */

    SetShellSwitches ();

#ifdef SIGQUIT
    signal (SIGQUIT, qflag);
#endif

/* Set up signals */

    if (InteractiveFlag)
	signal (SIGTERM, TerminateSignalled);

/* Load any parameters */

    SetUpParameterEV (argc, argv, name);

/* Return point */

    if (SetErrorPoint (ReturnPoint))
	ExitTheShell (FALSE);

    signal (SIGINT, InterruptSignalled);

/* Read profile ?.  Init EMAC first for binding keys */

    if (((name != (char *)NULL) && (*name == CHAR_HYPHEN)) || Level0Shell)
    {
#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
	EMACS_Initialisation ();
#  endif
	LoadTheProfileFiles ();
    }

/*
 * Load history and configure
 */

    if (s->type == STTY)
    {
	HistoryEnabled = TRUE;
	LoadHistory ();
    }

/*
 * If the x or r flag was set on the command line, set it now after the
 * profiles have been executed.
 */

    if (OptionsXflag)
	FL_SET (FLAG_PRINT_EXECUTE);

    if (OptionsRflag)
    {
	FL_SET (FLAG_READONLY_SHELL);
	RestrictedShellFlag = TRUE;
    }

/*
 * Execute $ENV
 *
 * TOCHECK - substitute (cp, DOTILDE);
 */

    if ((cp = GetVariableAsString (ENVVariable, FALSE)) != null)
	ExecuteShellScript (cp);

/* If interactive, set up Editor modes */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
    if (InteractiveFlag)
    {
	char		**rep = EditorVariables;

	while (*rep != (char *)NULL)
	{
	    if ((cp = GetVariableAsString (*(rep++), FALSE)) != null)
	    {
		SetEditorMode (cp);
		break;
	    }
	}
    }
#endif

/*
 * Execute what ever we have to do!!
 */

    while (TRUE)
    {
	switch (SetErrorPoint (ReturnPoint))
	{
	    case TERMINATE_POINT_SET:
		RunCommands (s);		/* Drop to exit		*/

	    case TERMINATE_COMMAND:
	    default:
		ExitTheShell (FALSE);

/* Re-set TTY input.  If we reach this point, the shell is a root shell and
 * the no exit message has been displayed.  Reset the shell for input from the
 * TTY.
 */

	    case TERMINATE_SHELL:
		s->type = STTY;
		break;
	}
    }
}

/*
 * Process a script file
 */

static bool F_LOCAL ExecuteShellScript (char *name)
{
    FILE	*f = stdin;
    int		fp;
    Source	*s;

    if (strcmp (name, "-") != 0)
    {
	if ((fp = OpenForExecution (name, (char **)NULL, (int *)NULL)) < 0)
	    return FALSE;

	if ((f = ReOpenFile (fp = ReMapIOHandler (fp),
			     sOpenReadMode)) == (FILE *)NULL)
	    return FALSE;
    }

    (s = pushs (SFILE))->u.file = f;
    s->file = name;

    RunCommands (s);

    if (f != stdin)
	CloseFile (f);

    return TRUE;
}

/*
 * run the commands from the input source, returning status.
 */

static int F_LOCAL RunCommands (Source *src)
{
    int		i;
    jmp_buf	ReturnPoint;
    C_Op	*t = (C_Op *)NULL;
    bool	wastty;
    int		EntryMemoryLevel = MemoryAreaLevel + 1;

    CreateNewEnvironment ();
    e.ErrorReturnPoint = (ErrorPoint)NULL;
    e.line = GetAllocatedSpace (LINE_MAX);

/*
 * Run until the end
 */

    while (TRUE)
    {

/* Initialise space */

	MemoryAreaLevel = EntryMemoryLevel;
	ReleaseMemoryArea (MemoryAreaLevel);
	SW_intr = 0;
	ProcessingEXECCommand = FALSE;

	if (src->next == NULL)
	    src->echo = C2bool (FL_TEST (FLAG_ECHO_INPUT));

/*
 * Set up a few things for console input - cursor, mail prompt etc
 */

	if ((wastty = C2bool (src->type == STTY)))
	{
	    PositionCursorInColumnZero ();
	    CheckForMailArriving ();
	    CheckForTerminatedProcess ();
	    CloseAllHandlers ();	/* Clean up any open shell files */
	}

	LastUserPrompt = PS1;
	FlushStreams ();			/* Clear output */

/* Set execute function recursive level and the SubShell count to zero */

	Execute_stack_depth = 0;

/* Set up Redirection IO (Saved) array and SubShell Environment information */

	NSave_IO_E = 0;		/* Number of entries		*/
	MSave_IO_E = 0;		/* Max Number of entries	*/
	NSubShells = 0;		/* Number of entries		*/
	MSubShells = 0;		/* Max Number of entries	*/
	CurrentFunction = (FunctionList *)NULL;
	CurrentFunction = (FunctionList *)NULL;
	ProcessingDEBUGTrap = FALSE;
	ProcessingERRORTrap = FALSE;
	Break_List = (Break_C *)NULL;
	Return_List = (Break_C *)NULL;
	SShell_List = (Break_C *)NULL;
	ProcessingEXECCommand = FALSE;

/* Get the line and process it */

	if (SetErrorPoint (ReturnPoint) ||
	    ((t = BuildParseTree (src)) == (C_Op *)NULL) || SW_intr)
	{
	    ScrapHereList ();

	    if ((!InteractiveFlag && SW_intr) || FL_TEST (FLAG_ONE_COMMAND))
		ExitTheShell (FALSE);

/* Go round again */

	    src->str = null;
	    SW_intr = 0;
	    continue;
	}

/* Ok - reset some variables and then execute the command tree */

	SW_intr = 0;
	ProcessingEXECCommand = FALSE;
	FlushHistoryBuffer ();		/* Save history			*/

/* Check for exit */

	if ((t != NULL) && (t->type == TEOF))
	{
	    if (wastty && (ShellGlobalFlags & FLAGS_IGNOREEOF))
	    {
		PrintWarningMessage ("Use `exit'");
		src->type = STTY;
		continue;
	    }

	    else
		break;
	}

/* Execute the parse tree */

	if ((SetErrorPoint (ReturnPoint) == 0) &&
	    ((!FL_TEST (FLAG_NO_EXECUTE)) || (src->type == STTY)))
	    ExecuteParseTree (t, NOPIPE, NOPIPE, 0);

/* Make sure the I/O and environment are back at level 0 and then clear them */

	e.ErrorReturnPoint = (ErrorPoint)NULL;
	Execute_stack_depth = 0;
	ClearExtendedLineFile ();

	if (NSubShells != 0)
	    DeleteGlobalVariableList ();

	if (NSave_IO_E)
	    RestoreStandardIO (0, TRUE);

	if (MSubShells)
	    ReleaseMemoryCell ((void *)SubShells);

	if (MSave_IO_E)
	    ReleaseMemoryCell ((void *)SSave_IO);

    /* Check for interrupts */

	if ((!InteractiveFlag && SW_intr) || FL_TEST (FLAG_ONE_COMMAND))
	{
	    ProcessingEXECCommand = FALSE;
	    ExitTheShell (FALSE);
	}

/* Run any traps that are required */

	if ((i = InterruptTrapPending) != 0)
	{
	    InterruptTrapPending = 0;
	    RunTrapCommand (i);
	}
    }

/*
 * Terminate the current environment
 */

    QuitCurrentEnvironment ();
    return ExitStatus;
}

/*
 * Set up the value of $-
 */

void SetShellSwitches (void)
{
    char	*cp, c;
    char	m['z' - 'a' + 2];

    for (cp = m, c = 'a'; c <= 'z'; ++c)
    {
	if (FL_TEST (c))
	    *(cp++) = c;
    }

    if (ShellGlobalFlags & FLAGS_MSDOS_FORMAT)
	*(cp++) = 'M';

    *cp = 0;
    SetVariableFromString (ShellOptionsVariable, m);
}

/*
 * Terminate current environment with an error
 */

void TerminateCurrentEnvironment (int TValue)
{
    FlushStreams ();			/* Clear output */

    if (e.ErrorReturnPoint != (ErrorPoint)NULL)
	ExitErrorPoint (TValue);

    /* NOTREACHED */
}

/*
 * Exit the shell
 */

void ExitTheShell (bool ReturnRequired)
{
    FlushStreams ();			/* Clear output */

    if (ProcessingEXECCommand)
	TerminateCurrentEnvironment (TERMINATE_COMMAND);

#if (OS_TYPE == OS_DOS) && !defined (__EMX__)
    if (Orig_I24_V == (void (INTERRUPT_TYPE *)())NULL)
    {
	feputs (NOExit);

	if (!ReturnRequired)
	    TerminateCurrentEnvironment (TERMINATE_SHELL);
    }
#endif

/* Clean up */

    ScrapHereList ();
    FreeAllHereDocuments (1);

/* Trap zero on exit */

    RunTrapCommand (0);

/* Dump history on exit */

    DumpHistory ();

    CloseAllHandlers ();

/* Clear swap file if necessary */

    ClearSwapFile ();

/* If this is a command only - restore the directory because DOS doesn't
 * and the user might expect it
 */

    if (*Start_directory)
	RestoreCurrentDirectory (Start_directory);

/* If this happens during startup - we restart */

#if (OS_TYPE == OS_DOS) && !defined (__EMX__)
    if (Orig_I24_V == (void (INTERRUPT_TYPE *)())NULL)
	return;
#endif

/*
 * Clean up any Here Document files left in the function tree
 */

    DeleteAllFunctions ();

/* Exit - hurray */

    FinalExitCleanUp (ExitStatus);

/* NOTREACHED */
}

/*
 * Output warning message
 */

int PrintWarningMessage (char *fmt, ...)
{
    va_list	ap;

    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    feputc (CHAR_NEW_LINE);
    ExitStatus = -1;

/* If leave on error - exit */

    if (FL_TEST (FLAG_EXIT_ON_ERROR))
	ExitTheShell (FALSE);

    va_end (ap);
    return 1;
}

/*
 * Shell error message
 */

void ShellErrorMessage (char *fmt, ...)
{
    va_list	ap;

/* Error message processing */

    if (source->file == (char *)NULL)
	feputs ("sh: ");

    else
	fprintf (stderr, "%s: at line %d, ", source->file, source->line);

    va_start (ap, fmt);
    ProcessErrorExit (fmt, ap);
    va_end (ap);
}

/*
 * Output error message
 */

void PrintErrorMessage (char *fmt, ...)
{
    va_list	ap;

/* Error message processing */

    va_start (ap, fmt);
    ProcessErrorExit (fmt, ap);
    va_end (ap);
}

/*
 * Common processing for PrintErrorMessage and ShellError Message
 */

static void F_LOCAL ProcessErrorExit (char *fmt, va_list ap)
{
    vfprintf (stderr, fmt, ap);
    feputc (CHAR_NEW_LINE);

    ExitStatus = -1;

    if (FL_TEST (FLAG_EXIT_ON_ERROR))
	ExitTheShell (FALSE);

/* Error processing */

    if (FL_TEST (FLAG_NO_EXECUTE))
	return;

/* If not interactive - exit */

    if (!InteractiveFlag)
	ExitTheShell (FALSE);

    if (e.ErrorReturnPoint != (ErrorPoint)NULL)
	ExitErrorPoint (TERMINATE_COMMAND);

/* CloseAllHandlers (); Removed - caused problems.  There may be problems
 * remaining with files left open?
 */
}

/*
 * Create or delete a new environment.  If f is set, delete the environment
 */

void CreateNewEnvironment (void)
{
    ShellFileEnvironment	*ep;

/* Create a new environment */

    if ((ep = (ShellFileEnvironment *)
		GetAllocatedSpace (sizeof (ShellFileEnvironment)))
		== (ShellFileEnvironment *)NULL)
    {
	while (e.PreviousEnvironment)
	    QuitCurrentEnvironment ();

	TerminateCurrentEnvironment (TERMINATE_COMMAND);
    }

    *ep = e;
    e.PreviousEnvironment = ep;
    e.IOMap = 0L;
    e.OpenStreams = (Word_B *)NULL;
}

/*
 * Exit the current environment successfully
 */

void QuitCurrentEnvironment (void)
{
    ShellFileEnvironment	*ep;
    unsigned long		FdMap;
    Word_B			*wb;
    int				NEntries;
    int				i;

/* Restore old environment, delete the space and close any files opened in
 * this environment
 */

    if ((ep = e.PreviousEnvironment) != (ShellFileEnvironment *)NULL)
    {

/* Close opened streams */

	wb = e.OpenStreams;
	NEntries = WordBlockSize (wb);

	for (i = 0; i < NEntries; i++)
	{
	    if (wb->w_words[i] != (char *)NULL)
		fclose ((FILE *)wb->w_words[i]);
	}

/* Get the files used in this environment to close */

	FdMap = e.IOMap;
	e = *ep;

	ReleaseMemoryCell ((void *)ep);

	for (i = 0; i < 32; i++)
	{
	    if (FdMap & (1L << i))
		S_close (i + FDBASE, TRUE);
	}
    }
}

/*
 * Convert binary to ascii
 */

char *IntegerToString (int n)
{
    static char		nt[10];

    sprintf (nt, "%u", n);
    return nt;
}

/*
 * SIGINT interrupt processing
 */

void InterruptSignalled (int signo)
{
/* Restore signal processing and set SIGINT detected flag */

    signal (signo, InterruptSignalled);

#if SIGNALDEBUG
    fprintf (stderr, "Interrupt %d detected\n", signo); fflush (stderr);
#endif

/* Under OS/2, if the Ignore Interrupts flag is set, ignore them.  To do
 * with starting OS/2 programs in sh3.c
 */

#if (OS_TYPE != OS_DOS)
    if (IgnoreInterrupts)
        return;
#endif

/* Set interrupt detected */

    SW_intr = 1;

/* Zap the swap file, just in case it got corrupted */

    ClearSwapFile ();

/* Are we talking to the user?  Yes - Abandon processing */

    if (InteractiveFlag)
	TerminateCurrentEnvironment (TERMINATE_COMMAND);

/* No - exit */

    else
    {
	ProcessingEXECCommand = FALSE;
	ExitStatus = 1;
	ExitTheShell (FALSE);
    }
}

/*
 * Grap some space and check for an error
 */

void *GetAllocatedSpace (size_t n)
{
    void	*cp;

    if ((cp = AllocateMemoryCell (n)) == (void *)NULL)
	PrintErrorMessage (BasicErrorMessage, ShellNameLiteral, Outofmemory1);

    return cp;
}

/*
 * Re-allocate some space
 */

void	*ReAllocateSpace (void *OldSpace, size_t NewSize)
{
    void	*NewSpace;

    if ((NewSpace = GetAllocatedSpace (NewSize)) == (void *)NULL)
        return NewSpace;

    if (OldSpace != (void *)NULL)
    {
	size_t	OldSize = ((s_region *)((char *)OldSpace -
					sizeof (s_region)))->nbytes;

	SetMemoryAreaNumber (NewSpace, GetMemoryAreaNumber (OldSpace));
	memcpy (NewSpace, OldSpace, OldSize);
	ReleaseMemoryCell (OldSpace);
    }

    return NewSpace;
}


/*
 * Duplicate a memory Area
 */

void		*DuplicateMemoryCell (void *cell)
{
    void	*new;
    size_t	len = ((s_region *)((char *)cell - sizeof (s_region)))->nbytes;

    if ((new = AllocateMemoryCell (len)) == (void *)NULL)
	PrintErrorMessage (BasicErrorMessage, ShellNameLiteral, Outofmemory1);

    else
	memcpy (new, cell, len);

    return new;
}


/*
 * Get memory Area size
 */

size_t		GetMemoryCellSize (void *cell)
{
    return ((s_region *)((char *)cell - sizeof (s_region)))->nbytes;
}

/*
 * Save a string in a given area
 */

char *StringSave (char *s)
{
    char	*cp;

    if ((cp = GetAllocatedSpace (strlen (s) + 1)) != (char *)NULL)
    {
	SetMemoryAreaNumber ((void *)cp, 0);
	return strcpy (cp, s);
    }

    return null;
}

/*
 * Duplicate at current Memory level
 */

char *StringCopy (char *s)
{
    char	*cp;

    if ((cp = GetAllocatedSpace (strlen (s) + 1)) != (char *)NULL)
	return strcpy (cp, s);

    return null;
}

/*
 * trap handling - Save signal number and restore signal processing
 */

void TerminateSignalled (int i)
{
    if (i == SIGINT)		/* Need this because swapper sets it	*/
    {
	SW_intr = 0;

/* Zap the swap file, just in case it got corrupted */

	ClearSwapFile ();
    }

    InterruptTrapPending = i;
    signal (i, TerminateSignalled);
}

/*
 * Execute a trap command
 *
 *  0 - exit trap
 * -1 - Debug Trap
 * -2 - Error Trap
 */

void RunTrapCommand (int i)
{
    Source	*s;
    char	*trapstr;
    char	tval[10];
    char	*tvalp = tval;

/* Check for special values and recursion */

    if (i == -1)
    {
	tvalp = Trap_DEBUG;

	if (ProcessingDEBUGTrap)
	    return;

	ProcessingDEBUGTrap = TRUE;
    }

/* Error trap */

    else if (i == -2)
    {
	tvalp = Trap_ERR;

	if (ProcessingERRORTrap)
	    return;

	ProcessingERRORTrap = TRUE;
    }

    else
	sprintf (tval, "~%d", i);

    if ((trapstr = GetVariableAsString (tvalp, FALSE)) == null)
	return;

/* If signal zero, save a copy of the trap value and then delete the trap */

    if (i == 0)
    {
	trapstr = StringCopy (trapstr);
	UnSetVariable (tval, -1, TRUE);
    }

    (s = pushs (SSTRING))->str = trapstr;
    RunACommand (s, (char **)NULL);

    ProcessingDEBUGTrap = FALSE;
    ProcessingERRORTrap = FALSE;
}

/*
 * Find the given name in the dictionary and return its value.  If the name was
 * not previously there, enter it now and return a null value.
 */

VariableList	*LookUpVariable (char *name,	/* Variable name	*/
				 int  Index,	/* Array Index		*/
				 bool cflag)	/* Create flag		*/
{
    VariableList		*vp;
    VariableList		**vpp;
    int				c;
    static VariableList		dummy;
    void			(*save_signal)(int);

/* Set up the dummy variable */

    memset (&dummy, 0, sizeof (VariableList));
    dummy.name = name;
    dummy.status = STATUS_READONLY;
    dummy.value = null;

/* If digit string - use the dummy to return the value */

    if (isdigit (*name))
    {
	for (c = 0; isdigit (*name) && (c < 1000); name++)
	    c = c * 10 + *name - '0';

	c += Index;
	dummy.value = (c <= ParameterCount) ? ParameterArray[c] : null;
	return &dummy;
    }

/* Look up in list */

    dummy.index = Index;
    vpp = (VariableList **)tfind (&dummy, &VariableTree, SearchVariable);

/* If we found it, return it */

    if (vpp != (VariableList **)NULL)
    {
        vp = *vpp;

/* Special processing for SECONDS and RANDOM */

	if (!strcmp (name, SecondsVariable) &&
	    !(DisabledVariables & DISABLE_SECONDS))
	    SetUpANumericValue (vp, time ((time_t *)NULL) - ShellStartTime, 10);

	else if (!strcmp (name, RandomVariable) &&
		 !(DisabledVariables & DISABLE_RANDOM))
	    SetUpANumericValue (vp, (long)rand(), 10);

	return vp;
    }

/* If we don't want to create it, return a dummy */

    dummy.status |= STATUS_NOEXISTANT;

    if (!cflag)
	return &dummy;

/* Create a new variable.  If no memory, use the dummy */

    dummy.name = null;

    if ((vp = (VariableList *)GetAllocatedSpace (sizeof (VariableList)))
		== (VariableList *)NULL)
	return &dummy;

    if ((vp->name = StringCopy (name)) == null)
    {
	ReleaseMemoryCell ((void *)vp);
	return &dummy;
    }

/* Set values */

    vp->value = null;
    vp->index = Index;
    vp->status = NSubShells ? 0 : STATUS_GLOBAL;

/* Save signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Add to the tree */

    if (tsearch (vp, &VariableTree, SearchVariable) == (void *)NULL)
    {
	ReleaseMemoryCell ((void *)vp->name);
	ReleaseMemoryCell ((void *)vp);
        vp = &dummy;
    }

/* OK Added OK - set up memory */

    else
    {
	SetMemoryAreaNumber ((void *)vp, 0);
	SetMemoryAreaNumber ((void *)vp->name, 0);
    }

/* Restore signals */

    signal (SIGINT, save_signal);

    return vp;
}

/*
 * Count the number of entries in an Array Variable
 */

int	CountVariableArraySize (char *name)
{
    if (isdigit (*name))
	return ParameterCount;

    Count_Array = 0;
    Count_Name = name;
    twalk (VariableTree, CountEnvironment);
    return Count_Array;
}

/*
 * TWALK - Count the Environment Variables in an array
 */

static void CountEnvironment (const void *key, VISIT visit, int level)
{
    if (((visit == postorder) || (visit == leaf)) &&
       (strcmp (Count_Name, (*(VariableList **)key)->name) == 0))
	Count_Array++;
}

/*
 * TFIND & TSEARCH - Search the VARIABLE TREE for an entry
 */

int	SearchVariable (const void *key1, const void *key2)
{
    int			diff;

    if ((diff = strcmp (((VariableList *)key1)->name,
			((VariableList *)key2)->name)) != 0)
	return diff;

    return ((VariableList *)key1)->index - ((VariableList *)key2)->index;
}

/*
 * Execute an assignment.  If a valid assignment, load it into the variable
 * list.
 *
 * If value is not NULL, assign it.  Otherwise don't
 */

bool	AssignVariableFromString (char *String,	/* The assignment string */
				  int  *Index)	/* Index value returned	*/
{
    char	*cp;
    long	value = 0;

/* Ignore if not valid environment variable - check alpha and equals */

    if (!GetVariableName (String, &value, &cp, (bool*)NULL) ||
	(*cp != CHAR_ASSIGN))
    {
	if (value == -1)
	    PrintErrorMessage (LIT_BadArray, String);

	return FALSE;
    }

/* Change the = to a end of string */

    *(cp++) = 0;

/* Assign the value */

    SetVariableArrayFromString (String, (int)value, cp);

/* Return the index */

    if (Index != (int *)NULL)
	*Index = (int)value;

    return TRUE;
}

/*
 * Get variable name and index
 *
 * String either ends in a null or assignment
 */

bool	GetVariableName (char *String,	/* The original string		*/
			 long *Index,	/* Array index value found	*/
			 char **Value,	/* Pointer to the value		*/
			 bool *Array)	/* Array detected flag		*/
{
    char	*cp, *sp;
    char	EndName;

    *Index = 0;

/* Ignore if not valid environment variable - check alpha and equals */

    if (((EndName = IsValidVariableName (String)) != CHAR_ASSIGN) &&
	(EndName != CHAR_OPEN_BRACKETS) && EndName)
	return FALSE;

    if ((cp = strchr (String, CHAR_ASSIGN)) == (char *)NULL)
	cp = &String[strlen (String)];

    if (Array != (bool *)NULL)
	*Array = C2bool (EndName == CHAR_OPEN_BRACKETS);

/* Check for valid array */

    if (EndName == CHAR_OPEN_BRACKETS)
    {
	if ((Index == (long *)NULL) || (*(cp - 1) != CHAR_CLOSE_BRACKETS))
	    return FALSE;

/* Terminate the name and remove the trailing bracket */

	*(sp = strchr (String, CHAR_OPEN_BRACKETS)) = 0;
	*(cp - 1) = 0;

	if ((!ConvertNumericValue (sp + 1, Index, 10)) ||
	    (*Index < 0) || (*Index > INT_MAX))
	{
	    *Index = -1;
	    return FALSE;
	}
    }

/* Return pointer to null or assignment */

    *Value = cp;
    return TRUE;
}
/*
 * Duplicate the Variable List for a Subshell
 *
 * Create a new Var_list environment for a Sub Shell
 */

int CreateGlobalVariableList (unsigned int Function)
{
    int			i;
    S_SubShell		*sp;

    for (sp = SubShells, i = 0; (i < NSubShells) &&
			       (SubShells[i].depth < Execute_stack_depth);
	 i++);

/* If depth is greater or equal to the Execute_stack_depth - we should panic
 * as this should not happen.  However, for the moment, I'll ignore it
 */

    if (NSubShells == MSubShells)
    {
	sp = (S_SubShell *)ReAllocateSpace ((MSubShells == 0) ? (void *)NULL
							      : SubShells,
					    (MSubShells + SSAVE_IO_SIZE) *
						sizeof (S_SubShell));
/* Check for error */

	if (sp == (S_SubShell *)NULL)
	    return -1;

	SetMemoryAreaNumber ((void *)sp, 0);
	SubShells = sp;
	MSubShells += SSAVE_IO_SIZE;
    }

/* Save the depth and the old Variable Tree value */

    sp = &SubShells[NSubShells++];
    sp->OldVariableTree = VariableTree;
    sp->depth  = Execute_stack_depth;
    sp->GFlags = ShellGlobalFlags | Function;
    sp->Eflags = flags;
    VariableTree = (void *)NULL;

/* Duplicate the old Variable list */

    ATNE_Function = Function;
    twalk (sp->OldVariableTree, AddToNewEnvironment);

/* Reset global values */

    LoadGlobalVariableList ();
    return 0;
}

/*
 * TWALK - add to new environment
 */

static void AddToNewEnvironment (const void *key, VISIT visit, int level)
{
    VariableList	*vp = *(VariableList **)key;
    VariableList	*vp1;

    if ((visit == postorder) || (visit == leaf))
    {

/* For functions, do not copy the traps */

	if (ATNE_Function && (*vp->name == CHAR_TILDE) && vp->name[1])
	    return;

/* Create a new entry */

	vp1 = LookUpVariable (vp->name, vp->index, TRUE);

	if ((!(vp->status & STATUS_INTEGER)) && (vp->value != null))
	    vp1->value = StringSave (vp->value);

/* Copy some flags */

	vp1->status = vp->status;
	vp1->nvalue = vp->nvalue;
	vp1->base = vp->base;
	vp1->width = vp->width;
    }
}

/*
 * Delete a SubShell environment and restore the original
 */

void DeleteGlobalVariableList (void)
{
    int			j;
    S_SubShell		*sp;
    VariableList	*vp;
    void		(*save_signal)(int);

    for (j = NSubShells; j > 0; j--)
    {
       sp = &SubShells[j - 1];

       if (sp->depth < Execute_stack_depth)
	   break;

/* Reduce number of entries */

	--NSubShells;

/* Disable signals */

	save_signal = signal (SIGINT, SIG_IGN);

/* Restore the previous level information */

	vp = VariableTree;
	VariableTree = sp->OldVariableTree;
	ShellGlobalFlags = (unsigned int)(sp->GFlags & ~FLAGS_FUNCTION);
	flags = sp->Eflags;

/* Release the space */

	ATOE_GFlags = sp->GFlags;

	twalk (vp, AddToOldEnvironment);
	twalk (vp, DeleteEnvironment);

/* Restore signals */

	signal (SIGINT, save_signal);

	LoadGlobalVariableList ();
    }
}

/*
 * TWALK - delete old environment tree
 */

static void DeleteEnvironment (const void *key, VISIT visit, int level)
{
    VariableList	*vp = *(VariableList **)key;

    if ((visit == endorder) || (visit == leaf))
    {
        if (vp->value == null)
            ReleaseMemoryCell ((void *)vp->value);

        ReleaseMemoryCell ((void *)vp->name);
        ReleaseMemoryCell ((void *)vp);
    }
}

/*
 * TWALK - Transfer Current Environment to the Old one
 */

static void AddToOldEnvironment (const void *key, VISIT visit, int level)
{
    VariableList	*vp = *(VariableList **)key;
    VariableList	*vp1;

    if ((visit == postorder) || (visit == leaf))
    {

/* Skip local variables and traps */

	if ((ATOE_GFlags & FLAGS_FUNCTION) && (!(vp->status & STATUS_LOCAL)) &&
	    (((*vp->name != CHAR_TILDE) || !vp->name[1])))
	{

/* Get the entry in the old variable list and update it with the new
 * parameters
 */
	    vp1 = LookUpVariable (vp->name, vp->index, TRUE);

	    if (vp1->value != null)
		ReleaseMemoryCell ((void *)vp1->value);

	    vp1->value = vp->value;
	    vp->value = null;		/* Stop releaseing this as its tx */

	    vp1->status = vp->status;
	    vp1->nvalue = vp->nvalue;
	    vp1->base   = vp->base;
	    vp1->width  = vp->width;
	}
    }
}

/*
 * Load GLobal Var List values
 */

static void F_LOCAL LoadGlobalVariableList (void)
{
    VariableList	*cifs = LookUpVariable (IFS, 0, TRUE);

    CurrentDirectory = LookUpVariable (tilde, 0, TRUE);
    RestoreCurrentDirectory (CurrentDirectory->value);
    SetCharacterTypes (cifs->value, C_IFS);
}

/*
 * Match a pattern as in sh(1).  Enhancement to handle prefix processing
 *
 * IgnoreCase - ignore case on comparisions.
 * end - end of match in 'string'.
 * mode - mode for match processing - see GM_ flags in sh.h
 *
 * pattern character are prefixed with MAGIC by expand.
 */

bool GeneralPatternMatch (char		*string,	/* String	*/
			  unsigned char *pattern,	/* Pattern	*/
			  bool		IgnoreCase,	/* Ignorecase	*/
			  char		**end,		/* End of match	*/
			  int		mode)		/* Mode		*/
{
    int		string_c, pattern_c;
    char	*save_end;

    if ((string == (char *)NULL) || (pattern == (unsigned char *)NULL))
	return FALSE;

    while ((pattern_c = *(pattern++)) != 0)
    {
	string_c = *(string++);

	if (pattern_c != CHAR_MAGIC)
	{
	    if (IgnoreCase)
	    {
		string_c = tolower (string_c);
		pattern_c = tolower (pattern_c);
	    }

	    if (string_c != pattern_c)
		return FALSE;

	    continue;
	}

/* Magic characters */

	switch (*(pattern++))
	{
	    case CHAR_OPEN_BRACKETS:	/* Class expression		*/
		if ((!string_c) ||
		    ((pattern = CheckClassExpression (pattern, string_c,
						     IgnoreCase)) ==
				(unsigned char *)NULL))
		    return FALSE;

		break;

	    case CHAR_MATCH_ANY:	/* Match any character		*/
		if (string_c == 0)
		    return FALSE;

		break;

	    case CHAR_MATCH_ALL:	/* Match as many as possible	*/
		--string;
		save_end = (char *)NULL;

		do
		{
		    if (!*pattern ||
			GeneralPatternMatch (string, pattern, IgnoreCase, end,
					     mode))
		    {
			if (mode == GM_LONGEST)
			    save_end = *end;

			else
			    return TRUE;
		    }

		} while (*(string++));

		if (end != (char **)NULL)
		    *end = save_end;

		return C2bool (save_end != (char *)NULL);

	    default:		/* Match				*/
		if ((unsigned)string_c != pattern[-1])
		    return FALSE;

		break;
	}
    }

    if (end != (char **)NULL)
    {
	*end = string;
	return TRUE;
    }

    return C2bool (*string == 0);
}

/*
 * Process a class expression - []
 */

static unsigned char * F_LOCAL CheckClassExpression (
				unsigned char	*pattern,
				int		string_c, /* Match char*/
				bool		IgnoreCase)/* Ic flag	*/
{
    int		llimit_c, ulimit_c;
    bool	not = FALSE;
    bool	found;

/* Exclusive or inclusive class */

    if ((*pattern == CHAR_MAGIC) &&
	((*(pattern + 1) == CHAR_NOT) || (*(pattern + 1) == '!')))
    {
	pattern += 2;
	not = TRUE;
    }

    found = not;

/* Process the pattern */

    do
    {
	if (*pattern == CHAR_MAGIC)
	    pattern++;

	if (!*pattern)
	    return (unsigned char *)NULL;

/* Get the next character in class, converting to lower case if necessary */

	llimit_c = IgnoreCase ? tolower (*pattern) : *pattern;

/* If this is a range, get the end of range character */

	if ((*(pattern + 1) == CHAR_MATCH_RANGE) &&
	    (*(pattern + 2) != CHAR_CLOSE_BRACKETS))
	{
	    ulimit_c = IgnoreCase ? tolower (*(pattern + 2)) : *(pattern + 2);
	    pattern++;
	}

	else
	    ulimit_c = llimit_c;

/* Is the current character in the class? */

	if ((llimit_c <= string_c) && (string_c <= ulimit_c))
	    found = C2bool (!not);

    } while (*(++pattern) != CHAR_CLOSE_BRACKETS);

    return found ? pattern + 1 : (unsigned char *)NULL;
}

/*
 * Suffix processing - find the longest/shortest suffix.
 */

bool SuffixPatternMatch (char *string,	/* String to match		*/
			 char *pattern, /* Pattern to match against	*/
			 char **start,	/* Start position		*/
			 int  mode)	/* Match mode			*/
{
    char	*save_start = (char *)NULL;

/* Scan the string, looking for a match to the end */

    while (*string)
    {
	if (GeneralPatternMatch (string, (unsigned char *)pattern, FALSE,
				 (char **)NULL, GM_ALL))
	{

/* If longest, stop here */

	    if (mode == GM_LONGEST)
	    {
		*start = string;
		return TRUE;
	    }

/* Save the start of the shortest string so far and continue */

	    save_start = string;
	}

	++string;
    }

    return C2bool ((*start = save_start) != (char *)NULL);
}

/*
 * Get a string in a malloced area
 */

char *AllocateMemoryCell (size_t nbytes)
{
    s_region		*np;
    void		(*save_signal)(int);
#ifdef OS2_DOSALLOC
    SEL			sel;
#endif

    if (nbytes == 0)
	abort ();	/* silly and defeats the algorithm */

/* Grab some space */

#ifdef OS2_DOSALLOC
    if (DosAllocSeg (nbytes + sizeof (s_region), &sel, SEG_NONSHARED))
    {
	errno = ENOMEM;
	return (char *)NULL;
    }

    np = (s_region *)MAKEP (sel, 0);
    memset (np, 0, nbytes + sizeof (s_region));

#else
    if ((np = (s_region *)calloc (nbytes + sizeof (s_region), 1))
		== (s_region *)NULL)
    {
	errno = ENOMEM;
        return (char *)NULL;
    }
#endif

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Link into chain */

    np->next = MemoryAreaHeader;
    np->area = MemoryAreaLevel;
    np->nbytes = nbytes;

    MemoryAreaHeader = np;

/* Restore signals */

    signal (SIGINT, save_signal);

    return ((char *)np) + sizeof (s_region);
}

/*
 * Release a array of strings
 */

void	ReleaseAList (char **list)
{
    char	**ap = list;

    while (*ap != (char *)NULL)
	ReleaseMemoryCell (*(ap++));

    ReleaseMemoryCell (list);

}

/*
 * Free a string in a malloced area
 */

void ReleaseMemoryCell (void *s)
{
    s_region		*cp = MemoryAreaHeader;
    s_region		*lp = (s_region *)NULL;
    s_region		*sp = (s_region *)((char *)s - sizeof (s_region));
    void		(*save_signal)(int);

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Find the string in the chain */

    if (s != (char *)NULL)
    {
	while (cp != (s_region *)NULL)
	{
	    if (cp != sp)
	    {
		lp = cp;
		cp = cp->next;
		continue;
	    }

/* First in chain ? */

	    else if (lp == (s_region *)NULL)
		MemoryAreaHeader = cp->next;

/* Delete the current entry and relink */

	    else
		lp->next = cp->next;

	    RELEASE_MEMORY (cp);
	    break;
	}
    }

/* Restore signals */

    signal (SIGINT, save_signal);
}

/*
 * Check for memory leaks with a dump
 */

#ifdef DEBUG_MEMORY
void DumpMemoryCells (int status)
{
    s_region		*cp = MemoryAreaHeader;
    size_t		i;
    char		buffer[17];
    char		*sp;

/* Find the string in the chain */

    while (cp != (s_region *)NULL)
    {
	fprintf (stderr, "Segment 0x%.8lx Area %5d Length %5d Link 0x%.8lx\n",
		 cp, cp->area, cp->nbytes, cp->next);

	memset (buffer, CHAR_SPACE, 17);
	buffer[16] = 0;

	sp = ((char *)cp) + sizeof (s_region);

	for (i = 0; i < (((cp->nbytes - 1)/16) + 1) * 16; i++)
	{
	    if (i >= cp->nbytes)
	    {
		feputs ("   ");
		buffer [i % 16] = CHAR_SPACE;
	    }

	    else
	    {
		fprintf (stderr, "%.2x ", *sp & 0x0ff);
		buffer [i % 16] = (char)(isprint (*sp) ? *sp : CHAR_PERIOD);
	    }

	    if (i % 16 == 15)
		fprintf (stderr, "    [%s]\n", buffer);

	    sp++;
	}

	feputc (CHAR_NEW_LINE);
	cp = cp->next;
    }
#undef exit
    exit (status);
#define exit(x)		DumpMemoryCells (x)
}
#endif

/*
 * Autodelete space nolonger required.  Ie. Free all the strings in a malloced
 * area
 */

void ReleaseMemoryArea (int a)
{
    s_region		*cp = MemoryAreaHeader;
    s_region		*lp = (s_region *)NULL;
    void		(*save_signal)(int);

/* Release the Here documents first */

    FreeAllHereDocuments (a);

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

    while (cp != (s_region *)NULL)
    {

/* Is the area number less than that specified - yes, continue */

	if (cp->area < a)
	{
	    lp = cp;
	    cp = cp->next;
	}

/* OK - delete the area.  Is it the first in chain ?  Yes, delete, relink
 * and update start location
 */

	else if (lp == (s_region *)NULL)
	{
	    lp = cp;
	    cp = cp->next;
	    MemoryAreaHeader = cp;

	    RELEASE_MEMORY (lp);
	    lp = (s_region *)NULL;
	}

/* Not first, delete the current entry and relink */

	else
	{
	    lp->next = cp->next;
	    RELEASE_MEMORY (cp);
	    cp = lp->next;
	}
    }

/* Restore signals */

    signal (SIGINT, save_signal);
}

/*
 * Set the area number for a malloced string.  This allows autodeletion of
 * space that is nolonger required.
 */

void SetMemoryAreaNumber (void *cp, int a)
{
    s_region	*sp = (s_region *)((char *)cp - sizeof (s_region));

    if (cp != (void *)NULL)
	sp->area = a;
}

/*
 * Get the area number for a malloced string
 */

int GetMemoryAreaNumber (void *cp)
{
    s_region	*sp = (s_region *)((char *)cp - sizeof (s_region));

    return sp->area;
}

/* Output one of the Prompt.  We save the prompt for the history part of
 * the program
 */

void OutputUserPrompt (char *s)
{
    struct tm		*tm;
    time_t		xtime = time ((time_t *)NULL);
    int			i;
    char		buf[PATH_MAX + 4];

    if (LastUserPrompt != (char *)NULL)
    {
	LastUserPrompt = s;		/* Save the Last prompt id	*/
	s = GetVariableAsString (s, TRUE); /* Get the string value	*/

	if (LastUserPrompt == PS1)
	    s = substitute (s, 0);
    }

    else
	s = LastUserPrompt1;

    tm = localtime (&xtime);

    while (*s)
    {

/* If a format character, process it */

	if (*s == CHAR_FORMAT)
	{
	    s++;
	    *s = (char)tolower(*s);

	    if (*s == CHAR_FORMAT)
		fputchar (CHAR_FORMAT);

	    else
	    {
		*buf = 0;

		switch (*(s++))
		{
		    case 'e':		    /* Current event number */
			if (HistoryEnabled)
			    sprintf (buf, "%d", Current_Event + 1);

			break;

		    case 't':		    /* time	    */
			sprintf (buf,"%.2d:%.2d", tm->tm_hour, tm->tm_min);
			break;

		    case 'd':		    /* date	    */
			sprintf (buf, "%.3s %.2d-%.2d-%.2d",
				 &"SunMonTueWedThuFriSat"[tm->tm_wday * 3],
				 tm->tm_mday, tm->tm_mon + 1,
				 tm->tm_year % 100);
			break;

		    case 'p':		    /* directory    */
		    case 'n':		    /* default drive */
			strcpy (buf, CurrentDirectory->value);

			if (*(s - 1) == 'n')
			    buf[1] = 0;

			break;

#if (OS_TYPE != OS_UNIX)
		    case 'v':		    /* version	    */
			sprintf (buf, "%s %.2d:%.2d", LIT_OSname,
				OS_VERS_N, _osminor);
			break;
#endif
		}

/* Output the string */

		foputs (buf);
	    }
	}

/* Escaped character ? */

	else if (*s == CHAR_META)
	{
	    ++s;
	    if ((i = ProcessOutputMetaCharacters (&s)) == -1)
		i = 0;

	    fputchar ((char)i);
	}

	else
	    fputchar (*(s++));
    }

    FlushStreams ();			/* Clear output */
}

/*
 * Get the current path in UNIX format and save it in the environment
 * variable $~
 */

void GetCurrentDirectoryPath (void)
{
    char	ldir[PATH_MAX + 6];
    char	*CurrentPWDValue;		/* Current directory	*/

    S_getcwd (ldir, 0);

/* Save in environment */

    SetVariableFromString (tilde, ldir);
    CurrentDirectory = LookUpVariable (tilde, 0, TRUE);

/* If we have changed directory, set PWD and OLDPWD */

    if (strcmp (CurrentPWDValue = GetVariableAsString (PWDVariable, FALSE),
		ldir))
    {
	SetVariableFromString (OldPWDVariable, CurrentPWDValue);
	SetVariableFromString (PWDVariable, ldir);
    }
}

/*
 * Initialise the shell and Patch up various parts of the system for the
 * shell.  At the moment, we modify the ctype table so that _ is an upper
 * case character.
 */

static bool F_LOCAL Initialise (int argc, char **argv)
{
    char	*s, *s1;
    char	**ap;
    char	*name = *argv;
    bool	OptionsRflag = FALSE;
#if (OS_TYPE == OS_NT)
    bool	Level0 = FALSE;
    int		sc;
#endif

/*
 * Determine the base OS if we can!
 */

#if (OS_TYPE == OS_NT)
    BaseOS = BASE_OS_NT;	/* Default to NT - no other */
#elif (OS_TYPE == OS_UNIX)
    BaseOS = BASE_OS_UNIX;	/* Default to UNIX - no other */
#elif (OS_TYPE == OS_OS2)
    BaseOS = BASE_OS_OS2;	/* Default to OS2 - no other */
#elif (OS_TYPE == OS_DOS)

    BaseOS = BASE_OS_DOS;	/* Default to DOS */

    if (_osmajor > 10)		/* Rather crude check */
	BaseOS = BASE_OS_OS2;

/* Know to stop NT ? */

    else
    {
        union REGS	r;
	r.h.ah = 0x16;
	r.h.al = 0;

	SystemInterrupt (0x2f, &r, &r);

	if (r.h.al > 0)
	    BaseOS = BASE_OS_WIN;
	
	else
	{
	    r.x.REG_AX = 0x3306;
	    SystemInterrupt (0x21, &r, &r);

	    if ((r.x.REG_AX == 0x3306) && (r.x.REG_BX == 0x3205))
		BaseOS = BASE_OS_NT;
	}
    }
#endif

/*
 * For MSDOS: Get original interrupt 24 address and set up our new interrupt
 *	      24 address.
 * For OS/2 : Set noignore interrupts.
 */

#if (OS_TYPE == OS_DOS)  && !defined (__EMX__)
    Orig_I24_V = GetInterruptVector (0x24);
#  if (OS_SIZE == OS_16)
    SetInterruptVector (0x24, SW_Int24);
#  else
    _harderr (HardErrorHandler);
#  endif
#elif (OS_TYPE != OS_DOS)
    IgnoreInterrupts = FALSE;
#endif

/* Create terminations queues - OS/2 only */

    CreateTerminationQueues ();

/* Set up character maps */

    InitialiseCharacterTypes ();

/* Create the integer variables, in case they are loaded from the
 * environment
 */
    CreateIntegerVariables ();

/* Initialise Path Extension lists */

    BuildExtensionLists ();

/* For NT, we need to know now if this is a level 0 shell because NT has a
 * nasty habit of being case in-sensitive!  Not only for file names, but
 * also environment variables.  So scan the command line for the -0 option!
 */

#if (OS_TYPE == OS_NT)
    while ((sc = GetOptions (argc, argv, ShellOptions, 0)) != EOF)
    {
	if (sc == '0')
	{
	    Level0 = TRUE;
	    break;
	}
    }

    ResetGetOptions ();
#endif

/* Load the environment into our structures */

    if ((ap = environ) != NOWORDS)
    {
	for (ap = environ; *ap != (char *)NULL; ap++)
	{

/* Set up any variables.  Note there is an assumption that
 * AssignVariableFromString sets the equals sign to 0, hiding the value;
 */
	    if (!strncmp ("SECONDS=", *ap, 8))
		continue;

/* NT is case in-sensitive - what a PAIN! */

#if (OS_TYPE == OS_NT)
	    if (Level0)
	    {
		s = *ap;
		
		while (*s && (*s != CHAR_ASSIGN))
		{
		    *s = toupper (*s);
		    s++;
		}
	    }
#endif
	    if (AssignVariableFromString (*ap, (int *)NULL))
		SetVariableStatus (*ap, STATUS_EXPORT);
	}
    }

/* Change COMSPEC to unix format for execution */

    PATH_TO_UNIX (GetVariableAsString (ComspecVariable, FALSE));
    SetVariableStatus (ComspecVariable, STATUS_CONVERT_MSDOS);

/* Zap all files */

    CloseAllHandlers ();
    MemoryAreaLevel = 1;

/* Get the current directory */

    GetCurrentDirectoryPath ();

/* Initialise the getopts command */

    ResetGetoptsValues (TRUE);

/* Set up SHELL variable.  First check for a restricted shell.  Check the
 * restricted shell
 */

    SetVariableFromString (LastWordVariable, name);

/* For OS/2, we prefer the full path name and not that in argv[0].  Save a
 * copy of the string
 */

#if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_NT)
    PATH_TO_UNIX ((name = GetAllocatedSpace (strlen (_APgmName) + 4)));
    SetMemoryAreaNumber ((void *)name, 0);
    strcpy (name, _APgmName);
#endif

/* Has the program name got a .exe extension - Yes probably DOS 3+.  So
 * save it as the Shell name.  Under OS/2, make sure we use .exe 
 */

    if (GetVariableAsString (ShellVariableName, FALSE) == null)
    {
#if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_NT)
	if ((s1 = strrchr (name, CHAR_PERIOD)) == (char *)NULL)
	    strcat (name, EXEExtension);

	SetVariableFromString (ShellVariableName, name);

	ReleaseMemoryCell (name);
#elif (OS_TYPE == OS_DOS)
	if (((s1 = strrchr (name, CHAR_PERIOD)) != (char *)NULL) &&
	    (stricmp (s1 + 1, EXEExtension + 1) == 0))
	    SetVariableFromString (ShellVariableName, PATH_TO_UNIX (name));
#elif (OS_TYPE == OS_UNIX)
	SetVariableFromString (ShellVariableName,
				(*name == '-') ? name + 1 : name);
#endif
    }

/* Default if necessary */

    if (GetVariableAsString (ShellVariableName, FALSE) == null)
	SetVariableFromString (ShellVariableName, shellname);

    PATH_TO_UNIX (s1 = GetVariableAsString (ShellVariableName, FALSE));

/* Check for restricted shell */

    if ((s = FindLastPathCharacter (s1)) == (char *)NULL)
	s = s1;

    else
	s++;

    if (*s == 'r')
	OptionsRflag = TRUE;

/* Set up home directory */

    if (GetVariableAsString (HomeVariableName, FALSE) == null)
    {
        if ((s = GetVariableAsString ("INIT", FALSE)) == null)
            s = CurrentDirectory->value;

	SetVariableFromString (HomeVariableName, s);
    }

/* Set up OS Mode */

#if defined (__TURBOC__) 
    SetVariableFromNumeric (LIT_OSmode, 0L);
#elif (__WATCOMC__) 
    SetVariableFromNumeric (LIT_OSmode, (long)OS_TYPE - 1);
#elif (OS_TYPE == OS_UNIX)
    SetVariableFromNumeric (LIT_OSmode, 3L);
#else
    SetVariableFromNumeric (LIT_OSmode, (long)_osmode);
#endif

#if (OS_SIZE == OS_32)
    SetVariableFromNumeric (LIT_SHmode, 32L);
#else
    SetVariableFromNumeric (LIT_SHmode, 16L);
#endif

/* Set up history file location */

    SetVariableFromNumeric (LIT_Dollar, (long)getpid ());

    LoadGlobalVariableList ();
    PATH_TO_UNIX (GetVariableAsString (PathLiteral, FALSE));
    PATH_TO_UNIX (GetVariableAsString (CDPathLiteral, FALSE));

    return OptionsRflag;
}

/*
 * Mail Check processing.  Every $MAILCHECK seconds, we check either $MAIL
 * or $MAILPATH to see if any file has changed its modification time since
 * we last looked.  In $MAILCHECK, the files are separated by semi-colon (;).
 * If the filename contains a %, the string following the % is the message
 * to display if the file has changed.
 */

static void F_LOCAL CheckForMailArriving (void)
{
    int			delay = (int)GetVariableAsNumeric (MailCheckVariable);
    char		*mail = GetVariableAsString ("MAIL", FALSE);
    char		*mailp = GetVariableAsString ("MAILPATH", FALSE);
    static time_t	last = 0L;
    time_t		current = time ((time_t *)NULL);
    struct stat		st;
    char		*cp, *sp, *ap;

/* Have we waited long enough */

    if (((current - last) < delay) || (DisabledVariables & DISABLE_MAILCHECK))
	return;

/* Yes - Check $MAILPATH.  If it is defined, process it.  Otherwise, use
 * $MAIL
 */

    if (mailp != null)
    {

/* Check MAILPATH */

	sp = mailp;

/* Look for the next separator */

	while ((cp = strchr (sp, CHAR_PATH_SEPARATOR)) != (char *)NULL)
	{
	    *cp = 0;

/* % in string ? */

	    if ((ap = strchr (sp, CHAR_FORMAT)) != (char *)NULL)
		*ap = 0;

/* Check the file name */

	    if ((S_stat (sp, &st)) && (st.st_mtime > last) && st.st_size)
	    {
		feputs ((ap != (char *)NULL) ? ap + 1 : ymail);
		feputc (CHAR_NEW_LINE);
	    }

/* Restore the % */

	    if (ap != (char *)NULL)
		*ap = CHAR_FORMAT;

/* Restore the semi-colon and find the next one */

	    *cp = CHAR_PATH_SEPARATOR;
	    sp = cp + 1;
	}
    }

/* Just check MAIL */

    else if ((mail != null) && (S_stat (mail, &st)) &&
	     (st.st_mtime > last) && st.st_size)
    {
	feputs (ymail);
	feputc (CHAR_NEW_LINE);
    }

/* Save the last check time */

    last = current;
}

/*
 * Preprocess Argv to get handle of options in the format /x
 *
 * Some programs invoke the shell using / instead of - to mark the options.
 * We need to convert to -.  Also /c is a special case.  The rest of the
 * command line is the command to execute.  So, we get the command line
 * from the original buffer instead of argv array.
 */

#if (OS_TYPE != OS_UNIX)
static void F_LOCAL Pre_Process_Argv (char **argv, int *argc1)
{
    int		argc = 1;
    char	*ocl = _ACmdLine;
    int		i;

/* Check for these options */

    while ((*++argv != (char *)NULL) && (strlen (*argv) == 2) &&
	   (**argv == '/'))
    {
	argc++;
	*strlwr (*argv) = CHAR_SWITCH;

/* Get the original information from the command line */

	if ((*argv)[1] == 'c')
	{

/*
 * In some case under OS/2, we get /c, but with EMX style.  So all we need
 * to do is change the / to a - and return.
 */

#  if (OS_TYPE == OS_OS2)
	    if (EMXStyleParameters)
	        return;
#  endif

/*
 * Otherwise, parse the command line again, looking for the /c
 */

	    while ((*ocl != '/') && (*(ocl + 1) != 'c') &&
		   (*ocl) && (*ocl != CHAR_RETURN))
		++ocl;

	    if (*ocl != '/')
		continue;

/* Find the start of the string */

	    ocl += 2;

	    while (isspace (*ocl) && (*ocl != CHAR_RETURN))
		++ocl;

	    if (*ocl == CHAR_RETURN)
		continue;

/* Found the start.  Set up next parameter and ignore the rest */

	    if (*(argv + 1) == (char *)NULL)
		continue;

	    argc++;

/* Remove quotes from string, if they are there */

	    if ((*ocl == CHAR_DOUBLE_QUOTE) &&
		(ocl[i = (strlen (ocl) - 1)] == CHAR_DOUBLE_QUOTE))
	    {
		ocl[i] = 0;
		ocl++;
	    }

/* Set up new argument array */

	    *(argv + 1) = ocl;
	    *(argv + 2) = (char *)NULL;
	    *argc1 = argc;

	    if ((ocl = strchr (ocl, CHAR_RETURN)) != (char *)NULL)
		*ocl = 0;

	    return;
	}
    }
}
#endif

/*
 * Convert path format to/from UNIX
 */

char *ConvertPathToFormat (char *path, char in, char out)
{
#if (OS_TYPE == OS_UNIX)
    return path;
#else
    char	*s = path;

    while ((path = strchr (path, in)) != (char *)NULL)
	*path = out;

    return s;
#endif
}

/* Load profiles onto I/O Stack */

static void F_LOCAL LoadTheProfileFiles (void)
{
    char	*name;
    char	*Pname;

    if ((Pname = GetVariableAsString ("ETCPROFILE", FALSE)) == null)
    {
	Pname = "x:/etc/profile";
	*Pname = GetDriveLetter (GetRootDiskDrive ());
    }

    InteractiveFlag = TRUE;
    ExecuteShellScript (Pname);

/*
 * Check $HOME format.  If in DOS format, mark and convert to UNIX
 */

    if (strchr (name = GetVariableAsString (HomeVariableName, FALSE),
		CHAR_DOS_PATH) != (char *)NULL)
    {
	PATH_TO_UNIX (name);
	SetVariableStatus (HomeVariableName, STATUS_CONVERT_MSDOS);
    }

    name = BuildFileName ("profile"); 	/* Set up home profile */
    ExecuteShellScript (name);
    ReleaseMemoryCell ((void *)name);
}

/*
 * Convert Unix PATH to MSDOS PATH
 */

static void F_LOCAL ConvertUnixPathToMSDOS (char *cp)
{
    char	*scp = cp;
    int		colon = 0;

/* If there is a semi-colon or a backslash, we assume this is a DOS format
 * path
 */

    if ((strchr (cp, CHAR_PATH_SEPARATOR) != (char *)NULL) ||
	(strchr (cp, CHAR_DOS_PATH) != (char *)NULL))
	return;

/* Count the number of colons */

    while ((cp = strchr (cp, CHAR_COLON)) != (char *)NULL)
    {
	++colon;
	++cp;
    }

/* If there are no colons or there is one colon as the second character, it
 * is probably an MSDOS path
 */

    cp = scp;
    if ((colon == 0) || ((colon == 1) && (*(cp + 1) == CHAR_COLON)))
	return;

/* Otherwise, convert all colons to semis */

    while ((cp = strchr (cp, CHAR_COLON)) != (char *)NULL)
	*(cp++) = CHAR_PATH_SEPARATOR;
}

/* Generate a file name from a directory and name.  Return null if an error
 * occurs or some malloced space containing the file name otherwise
 */

char *BuildFileName (char *name)
{
    char	*dir = GetVariableAsString (HomeVariableName, FALSE);
    char	*cp;

/* Get some space */

    if ((cp = AllocateMemoryCell (strlen (dir) + strlen (name) + 2))
		== (char *)NULL)
	return null;

/* Addend the directory and a / if the directory does not end in one */

    strcpy (cp, dir);

    if (!IsPathCharacter (cp[strlen (cp) - 1]))
	strcat (cp, DirectorySeparator);

/* Append the file name */

    return strcat (cp, name);
}

/* Clear prompts */

static void F_LOCAL ClearUserPrompts (void)
{
    ClearVariableStatus (PS1, STATUS_EXPORT);
    ClearVariableStatus (PS2, STATUS_EXPORT);
    ClearVariableStatus (PS3, STATUS_EXPORT);
    SetVariableFromString (PS1, null);
    SetVariableFromString (PS2, null);
    SetVariableFromString (PS3, null);
}

/* Process setting of SECONDS and RANDOM environment variables */

static void F_LOCAL SecondAndRandomEV (char *name, long val)
{
    if (!strcmp (name, SecondsVariable) &&
	!(DisabledVariables & DISABLE_SECONDS))
	ShellStartTime = time ((time_t *)NULL) - val;

    else if (!strcmp (name, RandomVariable) &&
	     !(DisabledVariables & DISABLE_RANDOM))
	srand ((int)val);
}

/*
 * Set up the Window name.  Give up if it does not work.
 */

#if (OS_TYPE == OS_OS2)
void	SetWindowName (char *title)
{
    HSWITCH		hswitch;
    SWCNTRL		swctl;
    char		*cp;

    if ((!(hswitch = WinQuerySwitchHandle (0, getpid ()))) ||
	(WinQuerySwitchEntry (hswitch, &swctl)))
	return;

    if (title != (char *)NULL)
        cp = title;

    else if ((DisabledVariables & DISABLE_WINTITLE) ||
	     ((cp = GetVariableAsString (WinTitleVariable, FALSE)) == null))
	cp = DefaultWinTitle;

    strncpy (swctl.szSwtitle, cp, sizeof (swctl.szSwtitle));
    swctl.szSwtitle[sizeof (swctl.szSwtitle) - 1] = 0;
    WinChangeSwitchEntry (hswitch, &swctl);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
void	SetWindowName (char *title)
{
    char		*cp;

    if (title != (char *)NULL)
        cp = title;

    else if ((DisabledVariables & DISABLE_WINTITLE) ||
	     ((cp = GetVariableAsString (WinTitleVariable, FALSE)) == null))
	cp = DefaultWinTitle;

    SetConsoleTitle (cp);
}
#endif

/*
 * In OS/2, check for terminated processes
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL CheckForTerminatedProcess (void)
{
    RESULTCODES		rescResults;
    PID			pidProcess;
    char		*s;
    OSCALL_RET		rc;

#  if (OS_TYPE == OS_OS2)
   CheckForSessionEnd ();
#  endif

/* Check for tasks terminating */

    while (TRUE)
    {
	while ((rc = DosCwait (DCWA_PROCESSTREE, DCWW_NOWAIT, &rescResults,
			       &pidProcess, 0)) == ERROR_INTERRUPT)
	    continue;

/* Ignore errors */

	if (rc)
	    return;

/* Remove the job */

	DeleteJob (pidProcess);

	switch (rescResults.codeTerminate)
	{
	    case TC_EXIT:
		s = "Normal Exit";
		break;

	    case TC_HARDERROR:
		s = "Hard error";
		break;

	    case TC_TRAP:
		s = "Trapped";
		break;

	    case TC_KILLPROCESS:
		s = "Killed";
		break;

	    default:
		s = "Unknown";
		break;

	}

	fprintf (stderr, "Process %d terminated - %s (%d)\n", pidProcess, s,
		 rescResults.codeTerminate);
    }
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static void F_LOCAL CheckForTerminatedProcess (void)
{
    twalk (JobTree, LookUpJobs);
}

/*
 * Walk the NT job tree looking for terminated processes
 */

static void LookUpJobs (const void *key, VISIT visit, int level)
{
    JobList	*job = *(JobList **)key;
    HANDLE	hp;
    DWORD	res;
    DWORD	ExitCode;

    if ((visit == postorder) || (visit == leaf))
    {
	if ((hp = OpenProcess (PROCESS_ALL_ACCESS, (BOOL)TRUE,
			       job->pid)) == NULL)
	{
	    PrintWarningMessage ("sh: Cannot open process %d\n%s", job->pid,
				 GetOSSystemErrorMessage (GetLastError ()));
	    DeleteJob (job->pid);
	}

/* Wait for the object to exit */

	else if ((res = WaitForSingleObject (hp, 0)) == WAIT_OBJECT_0)
	{
	    DeleteJob (job->pid);

	    if (!GetExitCodeProcess (hp, &ExitCode))
		PrintWarningMessage (
			"sh: Cannot get termination code for process %d\n%s",
			job->pid, GetOSSystemErrorMessage (GetLastError ()));
	    else
		fprintf (stderr, "Process %d terminated (%ld)\n", job->pid,
			 ExitCode);
	}

/* Failed?  - error */

	else if (res == WAIT_FAILED)
	{
	    PrintWarningMessage ("sh: Cannot wait for process %d\n%s",
				 job->pid,
				 GetOSSystemErrorMessage (GetLastError ()));

	    DeleteJob (job->pid);
	}

	CloseHandle (hp);
    }
}
#endif

/* UNIX version */

#if (OS_TYPE == OS_UNIX)
static void F_LOCAL CheckForTerminatedProcess (void)
{
    fputs ("UNIX: CheckForTerminatedProcess NI\n", stderr);
}
#endif


/*
 * Check for end of a Session
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL	CheckForSessionEnd (void)
{
    OSCALL_PARAM	DataLength;
    OSCALL_RET		rc;
    BYTE		bElemPriority;
    struct SessionEnd
    {
        unsigned short	SessionId;
	unsigned short	ExitCode;
    }			*DataAddress;

#  if (OS_SIZE == OS_32)
    REQUESTDATA		Request;
#  else
    QUEUERESULT		Request;
#  endif

/* Check for sessions terminating */

    while ((rc = DosReadQueue (SessionQueueHandler, &Request, &DataLength,
    		       	       (PVOID *)&DataAddress,
    		       	       0, DCWW_NOWAIT, &bElemPriority,
			       SessionQueueSema)) == NO_ERROR)
    {
	DeleteJobBySession (DataAddress->SessionId);

	fprintf (stderr, "Session %d terminated - Normal Exit (%d)\n",
		 DataAddress->SessionId, DataAddress->ExitCode);

#  if (OS_SIZE == OS_32)
	DosFreeMem (DataAddress);
#  else
	DosFreeSeg (SELECTOROF ((DataAddress)));
#  endif
    }
}
#endif

/*
 * Set up the Parameter Environment variables
 */

static void F_LOCAL SetUpParameterEV (int argc, char **argv, char *name)
{
    Word_B	*wb = (Word_B *)NULL;
    char	*Value;
    int		i;

    if ((Value = StringSave (name)) == null)
    {
	fprintf (stderr, BasicErrorMessage, ShellNameLiteral, Outofmemory1);
	return;
    }

    wb = AddParameter (Value, wb, ShellNameLiteral);

    for (i = 1; i < argc; ++i)
    {
	if ((!AssignVariableFromString (argv[i], (int *)NULL)) &&
	    (wb != (Word_B *)NULL))
	{
	    if ((Value = StringSave (argv[i])) != null)
		wb = AddParameter (Value, wb, ShellNameLiteral);

	    else
	    {
		fprintf (stderr, BasicErrorMessage, ShellNameLiteral,
			 Outofmemory1);
		return;
	    }
	}
    }

    if (wb != (Word_B *)NULL)
	wb = AddParameter ((char *)NULL, wb, ShellNameLiteral);
}

/*
 * Update the Seconds and Random variables
 */

void HandleSECONDandRANDOM (void)
{
    if (!(DisabledVariables & DISABLE_SECONDS))
	LookUpVariable (SecondsVariable, 0, TRUE);

    if (!(DisabledVariables & DISABLE_RANDOM))
	LookUpVariable (RandomVariable, 0, TRUE);
}

/*
 * Set the status of an environment variable
 */

void SetVariableStatus (char *name,		/* Variable name	*/
			int  flag)		/* New status		*/
{
    VariableList	*vp = LookUpVariable (name, 0, TRUE);

    if (IS_VariableFC ((int)*vp->name))	/* not an internal symbol ($# etc) */
	vp->status |= flag;
}

/*
 * Array version - only 0 is exported
 */

void SetVariableArrayStatus (char *name,	/* Variable name	*/
			     int  index,	/* Array index		*/
			     int  flag)		/* New status		*/
{
    VariableList	*vp = LookUpVariable (name, index, TRUE);

    if (IS_VariableFC ((int)*vp->name))	/* not an internal symbol ($# etc) */
    {
	vp->status |= flag;

	if (index)
	    vp->status &= ~STATUS_EXPORT;
    }
}

/*
 * Set the status of an environment variable
 */

void ClearVariableStatus (char *name, int flag)
{
    VariableList	*vp = LookUpVariable (name, 0, TRUE);

    if (IS_VariableFC ((int)*vp->name))	/* not an internal symbol ($# etc) */
	vp->status &= ~flag;
}

/*
 * Check allowed to set variable
 */

static bool F_LOCAL AllowedToSetVariable (VariableList *vp)
{
    if (vp->status & STATUS_READONLY)
    {
	ShellErrorMessage (LIT_2Strings, vp->name, LIT_IsReadonly);
	return FALSE;
    }

/* Check for $PATH, $SHELL or $ENV reset in restricted shell */

    if ((!strcmp (vp->name, PathLiteral) || !strcmp (vp->name, ENVVariable) ||
	 !strcmp (vp->name, ShellVariableName)) &&
	CheckForRestrictedShell (PathLiteral))
	return FALSE;

    return TRUE;
}

/*
 * Set up a variable from a string
 */

void SetVariableFromString (char *name, char *val)
{
    SetVariableArrayFromString (name, 0, val);
}

/*
 * Array index version
 */

void SetVariableArrayFromString (char *name, int Index, char *val)
{
    VariableList	*vp = LookUpVariable (name, Index, TRUE);
    char		*xp = null;
    long		nval;

/* Check if allowed to set variable */

    if (!AllowedToSetVariable (vp))
	return;

/* If we change the PATH to a new value, we need to untrack some aliases */

    if (!strcmp (name, PathLiteral) && strcmp (vp->value, val))
	UnTrackAllAliases ();

    CheckOPTIND (name, atol (val));

/* Save the new value */

    if ((!(vp->status & STATUS_INTEGER)) && (val != null) && strlen (val) &&
	((xp = StringSave (val = SuppressSpacesZeros (vp, val))) == null))
	    return;

/* Free the old value if appropriate */

    if (vp->value != null)
	ReleaseMemoryCell ((void *)vp->value);

    vp->value = null;

/* String value? */

    if (!(vp->status & STATUS_INTEGER))
    {
	vp->value = xp;

	if (!vp->width)
	    vp->width = strlen (val);
    }

/* No - Number value */

    else if (!ValidMathsExpression (val, &nval))
    {
	SecondAndRandomEV (name, nval);
	SetUpANumericValue (vp, nval, -1);
    }
    

/* Check to see if it should be exported */

    if (FL_TEST (FLAG_ALL_EXPORT))
	vp->status |= STATUS_EXPORT;

/* Convert UNIX to DOS for PATH variable */

    if (!strcmp (name, PathLiteral))
	ConvertUnixPathToMSDOS (GetVariableAsString (PathLiteral, FALSE));

    else if (!strcmp (name, CDPathLiteral))
	ConvertUnixPathToMSDOS (GetVariableAsString (CDPathLiteral, FALSE));

    else if (!strcmp (name, PathExtsLiteral))
	BuildExtensionLists ();

/* Set up IFS characters */

    else if (!strcmp (name, IFS))
	SetCharacterTypes (val, C_IFS);

/* Check for title change */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
    else if (!strcmp (name, FCEditVariable))
	SetEditorMode (val);

    else if (!strcmp (name, EditorVariable))
	SetEditorMode (val);

    else if (!strcmp (name, VisualVariable))
	SetEditorMode (val);
#endif

#if (OS_TYPE != OS_DOS)
    else if ((!(DisabledVariables & DISABLE_WINTITLE)) &&
	     (!strcmp (name, WinTitleVariable)))
	SetWindowName ((char *)NULL);
#endif
}

/*
 * Set a variable from a numeric
 */

void		SetVariableFromNumeric (char *name, long value)
{
    SetVariableArrayFromNumeric (name, 0, value);
}

/* Array version */

void		SetVariableArrayFromNumeric (char *name, int Index, long value)
{
    VariableList	*vp = LookUpVariable (name, Index, TRUE);
    char		NumericBuffer[20];

/* Check if allowed to set variable */

    if (!AllowedToSetVariable (vp))
	return;

    CheckOPTIND (name, value);

    if (!(vp->status & STATUS_INTEGER))
    {
	sprintf (NumericBuffer, "%ld", value);
	SetVariableFromString (name, NumericBuffer);
    }

/* Save the integer value */

    else
	SetUpANumericValue (vp, value, -1);

    SecondAndRandomEV (name, value);
}

/*
 * Get variable as a numeric
 */

long		GetVariableAsNumeric (char *name)
{
    return GetVariableArrayAsNumeric (name, 0);
}

/* Array version */

long		GetVariableArrayAsNumeric (char *name, int Index)
{
    VariableList	*vp = LookUpVariable (name, Index, FALSE);

    if (vp->status & STATUS_INTEGER)
	return vp->nvalue;

    else
	return atol (vp->value);
}

/*
 * Get variable as a formatted string
 */

char		*GetVariableAsString (char *name, bool Format)
{
    return GetVariableArrayAsString (name, 0, Format);
}

/*
 * Indexed version
 */

char		*GetVariableArrayAsString (char *name, int Index, bool Format)
{
    VariableList	*vp = LookUpVariable (name, Index, FALSE);
    char		*Value = vp->value;
    char		*xp;
    size_t		len;
    char		*NumericBuffer;

    if (vp->status & STATUS_INTEGER)
    {
	if ((NumericBuffer = GetAllocatedSpace (40)) == (char *)NULL)
	    return null;

	if (vp->base != 10)
	{
	    sprintf (NumericBuffer, LIT_BNumber, vp->base);
	    xp = NumericBuffer + strlen (NumericBuffer);
	}

	else
	    xp = NumericBuffer;

        ltoa (vp->nvalue, xp, vp->base);
	return NumericBuffer;
    }

/* Handle a string variable, if no formating required, return it */

    if (!Format)
	return vp->value;

/* Left justify ? */

    if (vp->status & STATUS_LEFT_JUSTIFY)
    {
	xp = SuppressSpacesZeros (vp, Value);

	if ((Value = GetAllocatedSpace (vp->width + 1)) == (char *)NULL)
	    return null;

	memset (Value, CHAR_SPACE, vp->width);
	Value[vp->width] = 0;

	if ((len = strlen (xp)) > vp->width)
	    len = vp->width;

	memcpy (Value, xp, len);
    }

/* Right justify ? */

    else if (vp->status & (STATUS_RIGHT_JUSTIFY | STATUS_ZERO_FILL))
    {
	if ((xp = GetAllocatedSpace (vp->width + 1)) == (char *)NULL)
	    return null;

	if ((len = strlen (Value)) < vp->width)
	{
	    memset (xp, ((vp->status & STATUS_ZERO_FILL) &&
			 (isdigit (*Value))) ? '0' : CHAR_SPACE, vp->width);

	    memcpy (xp + (vp->width - len), Value, len);
	}

	else
	    memcpy (xp, Value + vp->width - len, vp->width);

	*(xp + vp->width) = 0;
	Value = xp;
    }

/* Handle upper and lower case conversions */

    if (vp->status & STATUS_LOWER_CASE)
	Value = strlwr (StringCopy (Value));

    if (vp->status & STATUS_UPPER_CASE)
	Value = strupr (StringCopy (Value));

    return Value;
}

/*
 * Set up a numeric value
 */

static void F_LOCAL SetUpANumericValue (VariableList *vp, long value, int base)
{
    vp->nvalue = value;
    vp->status |= STATUS_INTEGER;

    if (vp->base == 0)
	vp->base = (base > 1) ? base
			      : ((LastNumberBase != -1) ? LastNumberBase
							: 10);

    if (vp->value != null)
	ReleaseMemoryCell ((void *)vp->value);

    vp->value = null;
}

/*
 * Suppress leading spaces and zeros
 */

static char * F_LOCAL SuppressSpacesZeros (VariableList *vp, char *value)
{
    if (vp->status & STATUS_LEFT_JUSTIFY)
    {
	while (*value == CHAR_SPACE)
	    value++;

	if (vp->status & STATUS_ZERO_FILL)
	{
	    while (*value == '0')
		value++;
	}
    }

    return value;
}

/*
 * Check to see if a reset of CheckOPTIND has occured
 */

static void F_LOCAL CheckOPTIND (char *name, long value)
{
    if ((value == 1) && (!(DisabledVariables & DISABLE_OPTIND)) &&
	(strcmp (OptIndVariable, name) == 0))
	ResetGetoptsValues (FALSE);
}

/*
 * Initialise the Integer variables by creating them
 */

static void F_LOCAL CreateIntegerVariables (void)
{
    struct ShellVariablesInit	*wp = InitialiseShellVariables;

    while  (wp->Name != (char *)NULL)
    {
	SetVariableStatus (wp->Name, wp->Status);

	if (wp->CValue != null)
	    SetVariableFromString (wp->Name, wp->CValue);

	wp++;
    }
}

/*
 * Close up and exit
 */

void	FinalExitCleanUp (int status)
{
#if (OS_TYPE == OS_OS)
#  if (OS_SIZE == OS_32)
    DosCloseEventSem (SessionQueueSema);
    DosCloseQueue (SessionQueueHandler);
#  else
    DosCloseSem (SessionQueueSema);
    DosCloseQueue (SessionQueueHandler);
#  endif
#endif

    exit (status);
}

/*
 * Create Session termination semaphores and queues.  Also get the number
 * file handlers.
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL CreateTerminationQueues (void)
{
    int			count = 0;
    static char		Name[25];
    OSCALL_RET		rc;
#  if (OS_SIZE == OS_32)
    LONG		ReqCount = 0;		/* Increment		*/
    ULONG		CurMaxFH;		/* Available File handlers */

    DosSetRelMaxFH (&ReqCount, &CurMaxFH);
    MaxNumberofFDs = min (CurMaxFH, 32 + FDBASE);
#  endif

/* Create semaphore for queue */

    while (TRUE)
    {
#  if (OS_SIZE == OS_32)
        sprintf (Name, "\\SEM32\\SHELL\\%.5d", count++);
#  else
        sprintf (Name, "\\SEM\\SHELL\\%.5d", count++);
#  endif

#  if (OS_SIZE == OS_32)
	if ((rc = DosCreateEventSem (Name, &SessionQueueSema,
				     DC_SEM_SHARED, TRUE)) == NO_ERROR)
	    break;
#  else
	if ((rc = DosCreateSem (CSEM_PUBLIC, &SessionQueueSema,
				Name)) == NO_ERROR)
	{
	    DosSemClear (SessionQueueSema);
	    break;
	}
#  endif

/* Check for error */

#  if (OS_SIZE == OS_32)
	if (rc != ERROR_DUPLICATE_NAME)
#  else
	if (rc != ERROR_ALREADY_EXISTS)
#  endif
	{
	    SessionQueueSema = 0;
	    PrintErrorMessage ("DosCreateSem: Cannot create semaphore\n%s",
	    		       GetOSSystemErrorMessage (rc));
	}
    }

/* Create the queue */

    count = 0;

    while (TRUE)
    {
#  if (OS_SIZE == OS_32)
        sprintf (Name, "\\QUEUES\\SHELL\\%.5d", count++);
#  else
        sprintf (Name, "\\QUEUES\\SHQ%.5d", count++);
#  endif

	if ((rc = DosCreateQueue (&SessionQueueHandler,
#ifdef QUE_CONVERT_ADDRESS
				  QUE_FIFO | QUE_CONVERT_ADDRESS,
#else
				  QUE_FIFO,
#endif

				  Name)) == NO_ERROR)
	    break;

/* Check for error */

	if (rc != ERROR_QUE_DUPLICATE)
	{
	    SessionQueueHandler = 0;
	    PrintErrorMessage ("DosCreateQueue: Cannot create queue\n%s",
	    		       GetOSSystemErrorMessage (rc));
	}
    }

    SessionEndQName = Name;
}
#endif

/*
 * Set up Edit mode
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
static void F_LOCAL SetEditorMode (char *ed)
{
    char	*rcp;

    if ((rcp = strrchr (ed, '/')) != (char *)NULL)
	ed = rcp + 1;

#  ifdef FLAGS_EMACS
    if (strstr (ed, "emacs"))
    {
	ShellGlobalFlags &= ~FLAGS_EDITORS;
	ShellGlobalFlags |= FLAGS_EMACS;
    }
#  endif

#  ifdef FLAGS_VI
    if (strstr (ed, "vi"))
    {
	ShellGlobalFlags &= ~FLAGS_EDITORS;
	ShellGlobalFlags |= FLAGS_VI;
    }
#  endif

#  ifdef FLAGS_GMACS
    if (strstr (ed, "gmacs"))
    {
	ShellGlobalFlags &= ~FLAGS_EDITORS;
	ShellGlobalFlags |= FLAGS_GMACS;
    }
#  endif
}
#endif
