/*
 * MS-DOS SHELL - Data Declarations
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
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh6.c,v 2.15 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh6.c,v $
 *	Revision 2.15  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.14  1994/02/23  09:23:38  istewart
 *	Beta 233 updates
 *
 *	Revision 2.13  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.12  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.11  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.10  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.9  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.8  1993/06/14  11:00:12  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.7  1993/06/02  09:52:35  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.6  1993/02/16  16:03:15  istewart
 *	Beta 2.22 Release
 *
 *	Revision 2.5  1993/01/26  18:35:09  istewart
 *	Release 2.2 beta 0
 *
 *	Revision 2.4  1992/12/14  10:54:56  istewart
 *	BETA 215 Fixes and 2.1 Release
 *
 *	Revision 2.3  1992/11/06  10:03:44  istewart
 *	214 Beta test updates
 *
 *	Revision 2.2  1992/09/03  18:54:45  istewart
 *	Beta 213 Updates
 *
 *	Revision 2.1  1992/07/10  10:52:48  istewart
 *	211 Beta updates
 *
 *	Revision 2.0  1992/04/13  17:39:09  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include "sh.h"

#if (OS_TYPE == OS_UNIX)
#  include <sys/utsname.h>
static char	*Copy_Right1 = "%s %s SH Version 2.3 (%s) - %s (OS %s.%s)\n";
#else
static char	*Copy_Right1 = "%s %s SH Version 2.3 (%s) - %s (OS %d.%d)\n";
#endif


#if (OS_TYPE == OS_OS2)
char		*LIT_OSname = "OS2";
#  if (OS_SIZE == OS_32)
static char	*Copy_Right2 = "(32-Bit)";
#  else
static char	*Copy_Right2 = "(16-Bit)";
#  endif
#elif (OS_TYPE == OS_DOS)
char		*LIT_OSname = "MS-DOS";
#  if (OS_SIZE == OS_32)
static char	*Copy_Right2 = "(32-Bit)";
#  else
static char	*Copy_Right2 = "(16-Bit)";
#  endif
#elif (OS_TYPE == OS_NT)
char		*LIT_OSname = "MS-WNT";
static char	*Copy_Right2 = "";
#elif (OS_TYPE == OS_UNIX)
char		*LIT_OSname = "UNIX";
#endif

static char	*Copy_Right3 = "Copyright (c) Data Logic Ltd and Charles Forsyth 1990, 94\n";

#if (OS_TYPE == OS_DOS) && (OS_SIZE == OS_32)
bool		IgnoreHardErrors = FALSE;/* Hard Error Flag		*/
#endif

int		BaseOS;			/* Underlying OS		*/
char		**ParameterArray = (char **)NULL; /* Parameter array	*/
int		ParameterCount = 0;	/* # entries in parameter array	*/
int		ExitStatus;		/* Exit status			*/
bool		ExpansionErrorDetected;
				/* interactive (talking-type wireless)	*/
bool		InteractiveFlag = FALSE;
bool		ProcessingEXECCommand;	/* Exec mode			*/
bool		UseConsoleBuffer = FALSE;/* Flag from dofc to		*/
					/* GetConsoleInput		*/
int		AllowMultipleLines;	/* Allow continuation onto	*/
					/* Next line			*/
int		Current_Event = 0;	/* Current history event	*/
bool		ChangeInitLoad = FALSE;	/* Change load .ini pt.		*/
unsigned int	ShellGlobalFlags = 0;	/* Other global flags		*/
int		MaxNumberofFDs = 20;	/* Max no of file descriptors	*/

int		DisabledVariables = 0;	/* Disabled variables		*/
int		StartCursorPosition = -1;/* Start cursor position	*/

#ifndef OS_SWAPPING
unsigned int	SW_intr;		/* Interrupt flag		*/
bool		IgnoreInterrupts = FALSE;/* Ignore interrupts flag	*/
#else
					/* Swap mode			*/
int		Swap_Mode = SWAP_EXPAND | SWAP_DISK;
#endif

Break_C		*Break_List;	/* Break list for FOR/WHILE		*/
Break_C		*Return_List;	/* Return list for RETURN		*/
Break_C		*SShell_List;	/* SubShell list for EXIT		*/
bool		RestrictedShellFlag = FALSE;	/* Restricted shell	*/
				/* History processing enabled flag	*/
bool		HistoryEnabled = FALSE;

void		*FunctionTree = (void *)NULL;	/* Function tree	*/
FunctionList	*CurrentFunction = (FunctionList *)NULL;
void		*AliasTree = (void *)NULL;	/* Alias tree		*/

#if (OS_TYPE != OS_DOS)
void		*JobTree = (void *)NULL;	/* job tree		*/
bool		ExitWithJobsActive = FALSE;	/* Exit flag		*/
int		CurrentJob = 0;			/* No current		*/
int		PreviousJob = 0;		/* Previous Job		*/
#endif

#if (OS_TYPE == OS_OS2)
/*
 * Session Info
 */
char		*SessionEndQName = (char *)NULL;	/* Queue	*/

/*
 * Special flag for EMX parameters
 */

bool		EMXStyleParameters = FALSE;
#endif

/*
 * redirection
 */

Save_IO		*SSave_IO;	/* Save IO array			*/
int		NSave_IO_E = 0;	/* Number of entries in Save IO array	*/
int		MSave_IO_E = 0;	/* Max Number of entries in SSave_IO	*/
S_SubShell	*SubShells;	/* Save Vars array			*/
int		NSubShells = 0;	/* Number of entries in SubShells	*/
int		MSubShells = 0;	/* Max Number of entries in SubShells	*/
int		LastNumberBase = -1;	/* Last base entered		*/

int		InterruptTrapPending;	/* Trap pending			*/
int		Execute_stack_depth;	/* execute function recursion	*/
					/* depth			*/
void		*VariableTree = (void *)NULL;	/* Variable dictionary	*/
VariableList	*CurrentDirectory;	/* Current directory		*/
char		*LastUserPrompt;	/* Last prompt output		*/
char		*LastUserPrompt1;	/* Alternate Last prompt output	*/
char		IFS[] = "IFS";		/* Inter-field separator	*/
char		PS1[] = "PS1";		/* Prompt 1			*/
char		PS2[] = "PS2";		/* Prompt 2			*/
char		PS3[] = "PS3";		/* Prompt 3			*/
char		PS4[] = "PS4";		/* Prompt 4			*/
char		PathLiteral[] = "PATH";
char		CDPathLiteral[] = "CDPATH";
char		CurrentDirLiteral[] = ".";
char		ParentDirLiteral[] = "..";
char		PathExtsLiteral[] = "PATHEXTS";
char		HomeVariableName[] = "HOME";
char		ShellVariableName[] = "SHELL";
char		HistoryFileVariable[] = "HISTFILE";
char		HistorySizeVariable[] = "HISTSIZE";
char		*ComspecVariable= "COMSPEC";
char		*ParameterCountVariable = "#";
char		*ShellOptionsVariable = "-";
char		StatusVariable[] = "?";
char		SecondsVariable[] = "SECONDS";
char		RandomVariable[] = "RANDOM";
char		LineCountVariable[] = "LINENO";
char		*RootDirectory = "x:/";

#if (OS_TYPE != OS_DOS)
char		WinTitleVariable[] = "WINTITLE";
#endif

char		*OldPWDVariable = "OLDPWD";
char		*PWDVariable = "PWD";
char		*ENVVariable = "ENV";

#if (OS_TYPE != OS_DOS)
char		BATExtension[] = ".cmd";
#else
char		BATExtension[] = ".bat";
#endif

char		SHELLExtension[] = ".sh";
char		KSHELLExtension[] = ".ksh";
char		EXEExtension[] = ".exe";
char		COMExtension[] = ".com";
char		*NotFound = "not found";
char		*BasicErrorMessage = "%s: %s";
char		*DirectorySeparator = "/";
char		LastWordVariable[] = "_";
char		OptArgVariable[] = "OPTARG";
char		OptIndVariable[] = "OPTIND";
char		MailCheckVariable[] = "MAILCHECK";
char		FCEditVariable[] = "FCEDIT";
char		EditorVariable[] = "EDITOR";
char		VisualVariable[] = "VISUAL";
char		Trap_DEBUG[] = "~DEBUG";
char		Trap_ERR[] = "~ERR";
char		LIT_dos[] = "DOS";
char		*LIT_NewLine = "\n";
char		*LIT_BadID = "bad identifier";
char		LIT_export[] = "export";
char		LIT_exit[] = "exit";
char		LIT_exec[] = "exec";
char		LIT_done[] = "done";
char		LIT_history[] = "history";
char		LIT_REPLY[] = "REPLY";
char		LIT_LINES[] = "LINES";
char		LIT_COLUMNS[] = "COLUMNS";
char		*ListVarFormat = "%s=%s\n";
char		*Outofmemory1 = "Out of memory";
char		*LIT_Emsg = "%s: %s (%s)";
char		*LIT_2Strings = "%s %s";
char		*LIT_3Strings = "%s %s%s";
char		*LIT_SyntaxError = "Syntax error";
char	 	*LIT_BadArray = "%s: bad array value";
char 		*LIT_ArrayRange = "%s: subscript out of range";
char		*LIT_BNumber = "[%d]";
char		*LIT_Invalidfunction = "Invalid function name";
char		*LIT_AllowTTY = "SH_ALLOWTTYPIPES";
char		*LIT_IsReadonly = "is read-only";
char		LIT_Test[] = "[[";
int		MaximumColumns = 80;	/* Max columns			*/
int		MaximumLines = 25;	/* Max Lines			*/

/*
 * Fopen modes, different between IBM OS/2 2.x and the rest
 */

#ifdef __IBMC__
char		*sOpenReadMode = "r";	/* Open file in read mode	*/
char		*sOpenWriteMode = "w";	/* Open file in write mode	*/
char		*sOpenAppendMode = "w+";/* Open file in append mode	*/
char		*sOpenWriteBinaryMode = "wb";/* Open file in append mode	*/
#else
char		*sOpenReadMode = "rt";	/* Open file in read mode	*/
char		*sOpenWriteMode = "wt";	/* Open file in write mode	*/
char		*sOpenAppendMode = "wt+";/* Open file in append mode	*/
char		*sOpenWriteBinaryMode = "wb";/* Open file in append mode	*/
#endif

#if (OS_TYPE == OS_OS2)
STARTDATA	*SessionControlBlock;	/* Start a session info		*/

STARTDATA	PM_SessionControlBlock = {	/* PM session defaults	*/
    sizeof (STARTDATA),			/* Length		*/
    SSF_RELATED_CHILD,			/* Related		*/
    SSF_FGBG_FORE,			/* FgBg			*/
    SSF_TRACEOPT_NONE,			/* TraceOpt		*/
    (char *)NULL,			/* PgmTitle		*/
    (char *)NULL,			/* PgmName		*/
    (PBYTE)NULL,			/* PgmInputs		*/
    (PBYTE)NULL,			/* TermQ		*/
    (char *)1,				/* Environment		*/
    SSF_INHERTOPT_PARENT,		/* InheritOpt		*/
    SSF_TYPE_PM,			/* SessionType		*/
    (char *)NULL,			/* IconFile		*/
    0L,					/* PgmHandle		*/
    SSF_CONTROL_NOAUTOCLOSE,		/* PgmControl		*/
    0,					/* InitXPos		*/
    0,					/* InitYPos		*/
    100,				/* InitXSize		*/
    100					/* InitYSize		*/
};

STARTDATA	DOS_SessionControlBlock = {	/* DOS session defaults	*/
    sizeof (STARTDATA),			/* Length		*/
    SSF_RELATED_CHILD,			/* Related		*/
    SSF_FGBG_FORE,			/* FgBg			*/
    SSF_TRACEOPT_NONE,			/* TraceOpt		*/
    (char *)NULL,			/* PgmTitle		*/
    (char *)NULL,			/* PgmName		*/
    (PBYTE)NULL,			/* PgmInputs		*/
    (PBYTE)NULL,			/* TermQ		*/
    (char *)1,				/* Environment		*/
    SSF_INHERTOPT_PARENT,		/* InheritOpt		*/
    SSF_TYPE_VDM,			/* SessionType		*/
    (char *)NULL,			/* IconFile		*/
    0L,					/* PgmHandle		*/
    SSF_CONTROL_NOAUTOCLOSE,		/* PgmControl		*/
    0,					/* InitXPos		*/
    0,					/* InitYPos		*/
    100,				/* InitXSize		*/
    100					/* InitYSize		*/
};
#endif

/*
 * Parser information
 */

char			CurrentLexIdentifier [IDENT+1];/* Identifier	*/
Source			*source;	/* yyparse/ScanNextToken source	*/
YYSTYPE			yylval;		/* result from ScanNextToken	*/
int			yynerrs;	/* Parse error flag		*/


/*
 * Global program mode information
 */

ExeMode 	ExecProcessingMode;	/* Global Program mode		*/

/*
 * Character Types array
 */

unsigned char	CharTypes [UCHAR_MAX + 1];

/*
 * Modified getopt values
 */

int		OptionIndex = 1;	/* optind			*/
int		OptionStart;		/* start character		*/
char		*OptionArgument;	/* optarg			*/

/*
 * Device directory.  The length of this string is defined by the variable
 * LEN_DEVICE_NAME_HEADER
 */

char		*DeviceNameHeader = "/dev/";

int		MemoryAreaLevel;/* Current allocation area		*/
long		flags = 0L;	/* Command line flags			*/
char		null[] = "";	/* Null value				*/
char		ConsoleLineBuffer[LINE_MAX + 1]; /* Console line buffer	*/

/*
 * Current environment
 */

ShellFileEnvironment	e = {
    (int *)NULL,		/* Error handler			*/
    0L,				/* IO Map for this level		*/
    (char *)NULL,		/* Current line buffer			*/
    (ShellFileEnvironment *)NULL,	/* Previous Env pointer		*/
};

/*
 * Some defines to print version and release info
 */

#define str(s)		# s
#define xstr(s)		str(s)

/*
 * The only bit of code in this module prints the version number
 */

void	PrintVersionNumber (FILE *fp)
{
#if (OS_TYPE == OS_UNIX)
    char		*Copy_Right2 = null;
    char		*_osmajor = null;
    char		*_osminor = null;
    struct utsname	name;

    if (!uname (&name))
    {
	Copy_Right2 = name.machine;
	_osmajor = name.release;
	_osminor = name.version;
    }

#endif
    fprintf (fp, Copy_Right1, LIT_OSname, Copy_Right2, xstr (RELEASE),
	     __DATE__, OS_VERS_N, _osminor);
    fputs (Copy_Right3, fp);
}
