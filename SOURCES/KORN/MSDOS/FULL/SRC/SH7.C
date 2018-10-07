/*
 * MS-DOS SHELL - Internal Command Processing
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Charles Forsyth
 *
 * This code is based on (in part) the shell program written by Charles
 * Forsyth and is subject to the following copyright restrictions.  The
 * code for the test (dotest) command was based on code written by
 * Erik Baalbergen.  The following copyright conditions apply:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh7.c,v 2.18 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh7.c,v $
 *	Revision 2.18  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.17  1994/02/23  09:23:38  istewart
 *	Beta 233 updates
 *
 *	Revision 2.16  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.15  1994/01/20  14:51:43  istewart
 *	Release 2.3 Beta 1
 *
 *	Revision 2.14  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.13  1993/12/01  11:58:34  istewart
 *	Release 226 beta
 *
 *	Revision 2.12  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.11  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.10  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
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
 *	Revision 2.0  1992/05/07  21:33:35  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#if defined (__EMX__)
#  include <sys/wait.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <stdarg.h>
#include <time.h>
#include "sh.h"

#define	SECS		60L
#define	MINS		3600L

#if (OS_TYPE == OS_OS2)
#  if defined (__EMX__)
#    define  PGM_TITLE_TYPE	unsigned char
#  else
#    define  PGM_TITLE_TYPE	char
#  endif
#endif
/* Definitions for echo and print */

#define ECHO_ESCAPE	0x01
#define ECHO_NO_EOL	0x02
#define ECHO_HISTORY	0x04

/* Definitions for test */

#define END_OF_INPUT	0
#define FILE_READABLE	1
#define FILE_WRITABLE	2
#define FILE_REGULAR	3
#define FILE_DIRECTRY	4
#define FILE_NONZERO	5
#define FILE_TERMINAL	6
#define STRING_ZERO	7
#define STRING_NONZERO	8
#define STRING_EQUAL	9
#define STRING_NOTEQUAL	10
#define NUMBER_EQUAL	11
#define NUMBER_NOTEQUAL	12
#define NUMBER_EQ_GREAT	13
#define NUMBER_GREATER	14
#define NUMBER_EQ_LESS	15
#define NUMBER_LESS	16
#define UNARY_NOT	17
#define BINARY_AND	18
#define BINARY_OR	19
#define LPAREN		20
#define RPAREN		21
#define OPERAND		22
#define FILE_EXECUTABLE	23
#define FILE_USER	24
#define FILE_GROUP	25
#define FILE_TEXT	26
#define FILE_BLOCK	27
#define FILE_CHARACTER	28
#define FILE_FIFO	29
#define FILE_NEWER	30
#define FILE_OLDER	31
#define STRING_LESS	32
#define STRING_GREATER	33
#define FILE_EXISTS	34
#define TEST_OPTION	35
#define FILE_SYMBOLIC	36
#define FILE_OWNER	37
#define FILE_GROUPER	38
#define FILE_SOCKET	39
#define FILE_EQUAL	40

#define UNARY_OP	1
#define BINARY_OP	2
#define B_UNARY_OP	3
#define B_BINARY_OP	4
#define PAREN		5

/* This is the list of operators and the conversion values */

static struct TestOperator {
    char	*OperatorName;
    short 	OperatorID;
    short 	OperatorType;
} ListOfTestOperators[] = {

/* These two entries are modified depending on the test program. The
 * alternative values are shown in the following comment.
 */

    {"-a",	FILE_EXISTS,		UNARY_OP},
    /* {"-a",	BINARY_AND,		B_BINARY_OP}, */
    {"-o",	TEST_OPTION,		UNARY_OP},
    /* {"-o",	BINARY_OR,		B_BINARY_OP}, */

/* Add new entries after here */

    {"-r",	FILE_READABLE,		UNARY_OP},
    {"-w",	FILE_WRITABLE,		UNARY_OP},
    {"-x",	FILE_EXECUTABLE,	UNARY_OP},
    {"-f",	FILE_REGULAR,		UNARY_OP},
    {"-d",	FILE_DIRECTRY,		UNARY_OP},
    {"-s",	FILE_NONZERO,		UNARY_OP},
    {"-t",	FILE_TERMINAL,		UNARY_OP},
    {"-z",	STRING_ZERO,		UNARY_OP},
    {"-n",	STRING_NONZERO,		UNARY_OP},
    {"=",	STRING_EQUAL,		BINARY_OP},
    {"!=",	STRING_NOTEQUAL,	BINARY_OP},
    {"<",	STRING_LESS,		BINARY_OP},
    {">",	STRING_GREATER,		BINARY_OP},
    {"-eq",	NUMBER_EQUAL,		BINARY_OP},
    {"-ne",	NUMBER_NOTEQUAL,	BINARY_OP},
    {"-ge",	NUMBER_EQ_GREAT,	BINARY_OP},
    {"-gt",	NUMBER_GREATER,		BINARY_OP},
    {"-le",	NUMBER_EQ_LESS,		BINARY_OP},
    {"-lt",	NUMBER_LESS,		BINARY_OP},
    {"!",	UNARY_NOT,		B_UNARY_OP},
    {"(",	LPAREN,			PAREN},
    {")",	RPAREN,			PAREN},
    {"&&",	BINARY_AND,		B_BINARY_OP},
    {"||",	BINARY_OR,		B_BINARY_OP},
    {"-c",	FILE_CHARACTER,		UNARY_OP},
    {"-b",	FILE_BLOCK,		UNARY_OP},
    {"-u",	FILE_USER,		UNARY_OP},
    {"-g",	FILE_GROUP,		UNARY_OP},
    {"-k",	FILE_TEXT,		UNARY_OP},
    {"-p",	FILE_FIFO,		UNARY_OP},
    {"-h",	FILE_SYMBOLIC,		UNARY_OP},
    {"-L",	FILE_SYMBOLIC,		UNARY_OP},
    {"-O",	FILE_OWNER,		UNARY_OP},
    {"-G",	FILE_GROUPER,		UNARY_OP},
    {"-S",	FILE_SOCKET,		UNARY_OP},
    {"-nt",	FILE_NEWER,		BINARY_OP},
    {"-ot",	FILE_OLDER,		BINARY_OP},
    {"-ef",	FILE_EQUAL,		BINARY_OP},
    {(char *)NULL,	0,		0}
};

/*
 * -o values for set
 */

static struct SetOptions {
    char		*OptionName;		/* Option name		*/
    unsigned int	FlagValue;		/* Option flag		*/
    bool		HasOptionValue;		/* Has -x value		*/
} SetOptions[] = {
    { "alternation",	FLAGS_ALTERNATION,	FALSE },
    { "allexport",	'a',			TRUE },

#ifdef FLAGS_BREAK_SWITCH
    { "break",		FLAGS_BREAK_SWITCH,	FALSE },
#endif

#ifdef FLAGS_EMACS
    { "emacs",		FLAGS_EMACS,		FALSE },
#endif

    { "errexit",	'e',			TRUE },

#ifdef FLAGS_GMACS
    { "gmacs",		FLAGS_GMACS,		FALSE },
#endif

#if (OS_TYPE == OS_NT) || (OS_TYPE == OS_OS2)
    { "ignorecase",	FLAGS_NOCASE,		FALSE },
#endif

    { "ignoreeof",	FLAGS_IGNOREEOF,	FALSE },
    { "keyword",	'k',			TRUE },
    { "markdirs",	FLAGS_MARKDIRECTORY,	FALSE },
    { "msdos",		FLAGS_MSDOS_FORMAT,	FALSE },
    { "noclobber",	FLAGS_NOCLOBER,		FALSE },
    { "noexec",		'n',			TRUE },
    { "noglob",		'f',			TRUE },
    { "nounset",	'u',			TRUE },

#ifdef FLAGS_SET_OS2
    { "os2",		FLAGS_SET_OS2,		FALSE },
#endif

    { "privileged",	'p',			TRUE },

#if (OS_TYPE == OS_NT) || (OS_TYPE == OS_OS2)
    { "realpipes",	FLAGS_REALPIPES,	FALSE },
#endif

    { "trackall",	'h',			TRUE },

#ifdef FLAGS_VI
    { "vi",		FLAGS_VI,		FALSE },
#endif

    { "verbose",	'v',			TRUE },
    { "verify",		FLAGS_VERIFY_SWITCH,	FALSE },

#ifdef FLAGS_SET_NT
    { "winnt",		FLAGS_SET_NT,		FALSE },
#endif

    { "xtrace",		'x',			TRUE },
    { (char *)NULL,	0,	FALSE }
};

/*
 * Getopts values
 */

static GetoptsIndex	GetOptsIndex = { 1, 1 };

/*
 * Signal Name structure
 *
 * Note that the first two entries are constructed such that the character
 * before the name is a ~.
 */

#define MAX_TRAP_SIGNALS	ARRAY_SIZE (TrapSignalList)
#define SIG_SPECIAL		-1		/* Error or trap */
#define SIG_NO_MAP		-2		/* No DOS mapping */

#if (OS_TYPE == OS_UNIX)
#define MAX_SIG_MAP		NSIG
#else
#define MAX_SIG_MAP		ARRAY_SIZE (UnixToDosSignals)
#endif

/*
 * Signal name to number mapping
 */

static struct TrapSignalList {
    char	*name;
    int		signo;
} TrapSignalList [] = {
    { Trap_DEBUG + 1,		SIG_SPECIAL },
    { Trap_ERR + 1,		SIG_SPECIAL },
    { LIT_exit,			0 },
    { "SIGINT",			SIGINT },
    { "SIGFPE",			SIGFPE },
    { "SIGILL",			SIGILL },
    { "SIGSEGV",		SIGSEGV },
    { "SIGABRT",		SIGABRT },

#ifdef SIGTERM
    { "SIGTERM",		SIGTERM },
#endif

#ifdef SIGBREAK
    { "SIGBREAK",		SIGBREAK },
#endif

#ifdef SIGUSR1
    { "SIGUSR1",		SIGUSR1 },
#endif

#ifdef SIGUSR2
    { "SIGUSR2",		SIGUSR2 },
#endif

#ifdef SIGUSR3
    { "SIGUSR3",		SIGUSR3 },
#endif

#ifdef SIGIDIVZ
    { "SIGIDIVZ",		SIGIDIVZ },
#endif

#ifdef SIGIOVFL
    { "SIGIOVFL",		SIGIOVFL },
#endif

#if (OS_TYPE == OS_UNIX)
    { "SIGHUP",			SIGHUP },
    { "SIGQUIT",		SIGQUIT },
    { "SIGTRAP",		SIGTRAP },
    { "SIGIOT",			SIGIOT },
    { "SIGEMT",			SIGEMT },
    { "SIGKILL",		SIGKILL },
    { "SIGBUS",			SIGBUS },
    { "SIGSYS",			SIGSYS },
    { "SIGPIPE",		SIGPIPE },
    { "SIGALRM",		SIGALRM },
    { "SIGTERM",		SIGTERM },
    { "SIGUSR1",		SIGUSR1 },
    { "SIGUSR2",		SIGUSR2 },
    { "SIGCLD",			SIGCLD },
    { "SIGPWR",			SIGPWR },
    { "SIGWINCH",		SIGWINCH },
    { "SIGURG",			SIGURG },
    { "SIGPOLL",		SIGPOLL },
    { "SIGIO",			SIGIO },
    { "SIGSTOP",		SIGSTOP },
    { "SIGTSTP",		SIGTSTP },
    { "SIGCONT",		SIGCONT },
    { "SIGTTIN",		SIGTTIN },
    { "SIGTTOU",		SIGTTOU },
    { "SIGVTALRM",		SIGVTALRM },
    { "SIGPROF",		SIGPROF },
    { "SIGXCPU",		SIGXCPU },
    { "SIGXFSZ",		SIGXFSZ },
#endif
};

/*
 * UNIX to DOS signal number mapping.  We only have 15 mappings because
 * only the fdirst 15 signal numbers are common
 */

#if (OS_TYPE != OS_UNIX)
static int	UnixToDosSignals [] = {
    0,			/* 0 - On exit				*/
    SIG_NO_MAP,		/* 1 - hangup				*/
    SIGINT,		/* 2 - interrupt (DEL)			*/
    SIG_NO_MAP,		/* 3 - quit (ASCII FS)			*/
    SIGILL,		/* 4 - illegal instruction		*/
    SIG_NO_MAP,		/* 5 - trace trap			*/
    SIG_NO_MAP,		/* 6 - IOT instruction			*/
    SIG_NO_MAP,		/* 7 - EMT instruction			*/
    SIGFPE,		/* 8 - floating point exception		*/
    SIG_NO_MAP,		/* 9 - kill				*/
    SIG_NO_MAP,		/* 10 - bus error			*/
    SIGSEGV,		/* 11 - segmentation violation		*/
    SIG_NO_MAP,		/* 12 - bad argument to system call	*/
    SIG_NO_MAP,		/* 13 - write on a pipe with no reader	*/
    SIG_NO_MAP,		/* 14 - alarm clock			*/
    SIGTERM		/* 15 - software termination signal	*/
};
#endif


/*
 * General Functions
 */

static int		DeleteAllVariables (const void *, const void *);
static int F_LOCAL	doOutofMemory (char *);
static int F_LOCAL	TestProcessNextExpression (int);
static int F_LOCAL	TestANDExpression (int);
static int F_LOCAL	TestPrimaryExpression (int);
static int F_LOCAL	TestUnaryOperand (int);
static int F_LOCAL	TestBinaryOperand (void);
static int F_LOCAL	TestLexicalAnalysis (void);
static struct TestOperator *
	   F_LOCAL	TestLookupOperator (char *);
static long F_LOCAL	GetNumberForTest (char *);
static void F_LOCAL	TestSyntaxError (void);
static void F_LOCAL	SetVerifyStatus (bool);
static void F_LOCAL	WhenceLocation (bool, char *, char *);
static void F_LOCAL	WhenceType (char *);
static int F_LOCAL	PrintOptionSettings (void);
static int F_LOCAL	CheckFAccess (char *, int);
static int F_LOCAL	CheckFType (char *, mode_t);
static int F_LOCAL	CheckFMode (char *, mode_t);
static int F_LOCAL	CheckFSize (char *);
static int F_LOCAL	CheckForFD (char *);

#if (OS_TYPE != OS_UNIX)
static OSCALL_RET F_LOCAL OS_GetFHAttributes (int, OSCALL_PARAM *);
#endif

#if (OS_TYPE == OS_DOS)
static void F_LOCAL	SetBreakStatus (bool);
#else
#  define SetBreakStatus(a)
#endif

static bool F_LOCAL	CheckPhysLogical (char *, bool *);
static char * F_LOCAL	GetPhysicalPath (char *, bool);
static bool F_LOCAL	ReadALine (int, char *, bool, bool, int *);
static bool F_LOCAL	WriteOutLine (int);
static bool F_LOCAL	ChangeOptionValue (char *, bool);
static void F_LOCAL	SetClearFlag (int, bool);
static void F_LOCAL	RemoveVariable (char *, int);
static int F_LOCAL	BreakContinueProcessing (char *, int);
static int F_LOCAL	SetUpNewParameterVariables (char **, int, int, char *);
static int F_LOCAL	UsageError (char *);
static void F_LOCAL 	PrintEntry (VariableList *, bool, unsigned int);
static int F_LOCAL 	UpdateVariableStatus (char **, unsigned int);
static int F_LOCAL	TypesetVariables (char **);
static int F_LOCAL	ListAllVariables (unsigned int, bool);
static int F_LOCAL	HandleFunction (char *);
static int F_LOCAL	TestOptionValue (char *, bool);
static int F_LOCAL	GetUnitNumber (char *);
static struct SetOptions * F_LOCAL LookUpOptionValue (char *);
static void		DisplayVariableEntry (const void *, VISIT, int);
static struct TrapSignalList * F_LOCAL LookupSignalName (char *);

#if (OS_TYPE != OS_DOS)
static bool F_LOCAL	ConvertJobToPid (char *, PID *);

#  if (OS_TYPE == OS_OS2)
static void F_LOCAL	DisplayStartData (STARTDATA *);
#  endif

#  if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32)
APIRET	 		DosQFileMode (PSZ, PULONG);

#    if !defined (__EMX__)
#      define Dos32FlagProcess		DosFlagProcess
#      pragma linkage (DosFlagProcess, far16 pascal)
#    else
USHORT	_THUNK_FUNCTION (Dos16FlagProcess) ();
#    endif

extern USHORT		Dos32FlagProcess (PID, USHORT, USHORT, USHORT);

#  endif

#  if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_16)
#      define Dos32FlagProcess		DosFlagProcess
#  endif

#  if (OS_TYPE == OS_NT)
int			 DosQFileMode (char *, DWORD *);
#  endif
#endif

/*
 * Builtin Commands
 */

static int	doexport (int, char **);
static int	doreadonly (int, char **);
static int	domsdos (int, char **);
static int	dotypeset (int, char **);
static int	dounalias (int, char **);
static int	doalias (int, char **);
static int	dolabel (int, char **);
static int	dofalse (int, char **);
static int	dochdir (int, char **);
static int	dodrive (int, char **);
static int	doshift (int, char **);
static int	doumask (int, char **);
static int	dodot (int, char **);
static int	doecho (int, char **);
static int	dolet (int, char **);
static int	doshellinfo (int, char **);

#if (OS_TYPE == OS_OS2)
static int	dostart (int, char **);
#endif

#if (OS_TYPE != OS_DOS)
static int	dodetach (int, char **);
static int	dokill (int, char **);
static int	dojobs (int, char **);
static int	dowait (int, char **);
#endif

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
static int	dobind (int, char **);
#endif

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32) && !defined (__WATCOMC__)
static int	dotimes (int, char **);
#endif

#if (OS_TYPE == OS_NT) || (OS_TYPE == OS_UNIX)
static int	dotimes (int, char **);
#endif

static int	dogetopts (int, char **);
static int	dopwd (int, char **);
static int	doswap (int, char **);
static int	dounset (int, char **);
static int	dowhence (int, char **);
static int	dofc (int, char **);
static int	dotest (int, char **);
static int	dover (int, char **);
static int	doread (int, char **);
static int	doeval (int, char **);
static int	dotrap (int, char **);
static int	dobuiltin (int, char **);
static int	dobreak (int, char **);
static int	docontinue (int, char **);
static int	doexit (int, char **);
static int	doreturn (int, char **);
static int	doset (int, char **);
static int	dofunctions (int, char **);
static int	dohistory (int, char **);

/*
 * TWALK global values for DisplayVariable
 */

static unsigned int	DVE_Mask;
static bool 		DVE_PrintValue;

/*
 * Local data structure for test command
 */

static char			**TestArgumentList;
static struct TestOperator	*CurrentTestOperator;
static jmp_buf			TestErrorReturn;
static char			*TestProgram;
static bool			NewTestProgram;

/*
 * Static structure for typeset
 */

static struct TypesetValues {
    unsigned int	Flags_On;
    unsigned int	Flags_Off;
    int			Base;
    int			Width;
} TypesetValues;

/*
 * Current position in Getoption string
 */

static int	GetOptionPosition = 1;		/* Current position	*/
static int	BadOptionValue = 0;		/* The bad option value	*/

/*
 * Common strings
 */

static char	*DoubleHypen = "--";
static char	*TypeSetUsage = "typeset [ [ [+|-][Hflprtux] ] [+|-][LRZi[n]] [ name [=value] ...]";
static char	*NotBuiltinCommand = "not a builtin";
static char	*NotAnAlias = "%s: %s is not an alias";
static char	*NotValidAlias = "Invalid alias name";
static char	*Reply_Array[] = {LIT_REPLY, (char *)NULL};
static char	*BadDrive = "%c: bad drive";
static char	*ShellInternalCommand = "is a shell internal command";
static char	*FCTooLong = "fc: command line too long";
static char	LIT_alias[] = "alias";
static char	LIT_print[] = "print";
static char	LIT_read[] = "read";
static char	LIT_shift[] = "shift";
static char	LIT_break[] = "break";
static char	LIT_builtin[] = "builtin";
static char	LIT_devfd[] = "/dev/fd/";
static char	*BuiltinUsage = "builtin [ -ads ] [ commands ]";
static char	*WhenceUsage = "whence [ -pvt ] [ commands ]";
static char	LIT_continue[] = "continue";
static char	LIT_type[] = "type";
static char	LIT_unalias[] = "unalias";
static char	LIT_unfunction[] = "unfunction";
static char	*LIT_bun = "bad unit number";
static char	*HistoryUsage = "history [ -iedsl ] [ number ]";
static char	*ReturnUsage = "return [ value ]";

#if (OS_TYPE == OS_OS2)
static char	*StartUsage =
"start -O [dos | pm] [ -hHfWPFxibID ] [ -c [ vilsna ]] [ -t title ]\n                   [ -e string ]\n       start [ -dfWPFibCISxhH ] [ -c [ vilsna ]] [ -t title ] [ -e string ]\n                   [ -X directory ] [ args.. ]\n       start -A sessionId";
static char	*Start_NoSession = "start: Cannot switch to session %lu\n%s";
#endif

#if (OS_TYPE != OS_DOS)
static char	*JobUsage = "jobs [-lp] [ -P pid]";
static char	*KillUsage = "kill [ [-l] | [ [-sig] [ process id | %job number ] ... ] ]";
#endif

/*
 * Disable variables mapping
 */

struct	DisableVariableMap {
    char	*name;
    int		flag;
} DisableVariableMap [] = {
    { MailCheckVariable,	DISABLE_MAILCHECK },
    { OptArgVariable,		DISABLE_OPTARG },
    { OptIndVariable,		DISABLE_OPTIND },
    { SecondsVariable,		DISABLE_SECONDS },
    { RandomVariable,		DISABLE_RANDOM },
    { LastWordVariable,		DISABLE_LASTWORD },
    { LineCountVariable,	DISABLE_LINECOUNT },

#if (OS_TYPE != OS_DOS)
    { WinTitleVariable,		DISABLE_WINTITLE },
#endif
    { (char *)NULL,		0 },
};

/*
 * built-in commands: : and true
 */

static int dolabel (int argc, char **argv)
{
    return 0;
}

static int dofalse (int argc, char **argv)
{
    return 1;
}

/*
 * Getopts - split arguments.  getopts optstring name [ arg ... ]
 */

static int dogetopts (int argc, char **argv)
{
    char	**Arguments;
    char	*OptionString;
    int		result;
    char	SResult[3];
    char	BadResult[2];
    int		Mode = GETOPT_MESSAGE | GETOPT_PLUS;

    if (argc < 3)
	return UsageError ("getopts optstring name [ arg ... ]");

    memset (SResult, 0, 3);

/*
 * A leading : in optstring causes getopts to store the letter of an
 * invalid option in OPTARG, and to set name to ? for an unknown option and
 * to : when a required option is missing.
 */

    if (*(OptionString = argv[1]) == ':')
    {
	OptionString++;
	Mode = GETOPT_PLUS;
    }

/*
 * Use positional parameters
 */

    if (argc == 3)
    {
	argc = ParameterCount + 1;
	Arguments = ParameterArray;
    }

/* No - use supplied */

    else
    {
	Arguments = &argv[2];		/* Adjust pointers		*/
	argc -= 2;
    }

/*
 * Get the value of OPTIND and initialise the getopt function
 */

    if (!(DisabledVariables & DISABLE_OPTIND))
	OptionIndex = (int)GetVariableAsNumeric (OptIndVariable);

    else
	OptionIndex = GetOptsIndex.Index;

/* Initialise the other values */

    GetOptionPosition = GetOptsIndex.SubIndex;
    OptionArgument = (char *)NULL;

    result = GetOptions (argc, Arguments, OptionString, Mode);

/* Save new positions */

    SaveGetoptsValues (OptionIndex, GetOptionPosition);

/* Check for EOF */

    if (result == EOF)
	return 1;

/* Set up result string */

    *SResult = (char)result;

/* Did we get an error.  Yes.  If message output, don't put value
 * in OPTARG
 */

    if (result == '?')
    {
	if (Mode & GETOPT_MESSAGE)
	    OptionArgument = (char *)NULL;

/* Error, set up values in optarg and the result */

	else
	{
	    SResult[0] = (char)((OptionArgument == (char *)NULL) ? '?' : ':');
	    *(OptionArgument = BadResult) = (char)BadOptionValue;
	    *(OptionArgument + 1) = 0;
	}
    }

/* If the argument started with a +, tell them */

    else if (OptionStart == '+')
    {
      SResult[1] = (char)result;
      SResult[0] = '+';
    }

/* If we got an argument, set the various shell variables */

    if ((OptionArgument != (char *)NULL) &&
	(!(DisabledVariables & DISABLE_OPTARG)))
	SetVariableFromString (OptArgVariable, OptionArgument);

    SetVariableFromString (argv[2], SResult);
    return 0;
}

/*
 * Reset the getopts values
 */

void	ResetGetoptsValues (bool Variable)
{
    if (Variable && (!(DisabledVariables & DISABLE_OPTIND)))
	SetVariableFromNumeric (OptIndVariable, 1L);

    GetOptsIndex.Index = 1;
    GetOptsIndex.SubIndex = 1;
}

/*
 * Save the new Getopts values
 */

void	SaveGetoptsValues (int Index, int Position)
{
    if (!(DisabledVariables & DISABLE_OPTIND))
	SetVariableFromNumeric (OptIndVariable, (long)Index);

    GetOptsIndex.Index = Index;
    GetOptsIndex.SubIndex = Position;
}

/*
 * Get the current Getopts values
 */

void	GetGetoptsValues (GetoptsIndex *values)
{
    values->Index = GetOptsIndex.Index;
    values->SubIndex = GetOptsIndex.SubIndex;
}

/*
 * Echo the parameters:  echo [ -n ] parameters
 */

static int doecho (int argc, char **argv)
{
    int		flags = ECHO_ESCAPE;
    int		fid = 1;
    char	*ip;			/* Input pointer		*/
    char	*op;
    int		c, c1;
    int		R_flag = GETOPT_PRINT;	/* Enable -n test		*/

    ResetGetOptions ();			/* Reset GetOptions		*/

/* Echo or print? */

    if (strcmp (*argv, LIT_print) == 0)
    {
	R_flag = 0;			/* Reset		*/

	while ((c = GetOptions (argc, argv, "Rnprsu:", R_flag)) != EOF)
	{
	    switch (c)
	    {
		case 'R':
		    R_flag = GETOPT_PRINT;
		    flags &= ~ECHO_ESCAPE;
		    break;

		case 'n':
		    flags = ECHO_NO_EOL;
		    break;

		case 'r':
		    flags &= ~ECHO_ESCAPE;
		    break;

		case 's':
		    flags |= ECHO_HISTORY;
		    break;

		case 'p':
		    break;

		case 'u':
		    if ((fid = GetUnitNumber (LIT_print)) == -1)
			return 1;

		    break;

		default:
		    return UsageError ("print [ -Rpnrsu[unit] ] ...");
	    }
	}
    }

    if ((OptionIndex < argc) && (R_flag == GETOPT_PRINT) &&
	(!strcmp (argv[OptionIndex], "-n")))
    {
	flags |= ECHO_NO_EOL;
	++OptionIndex;
    }

    argv += OptionIndex;

/* Clear the history buffer so we can use it */

    FlushHistoryBuffer ();
    op = ConsoleLineBuffer;

/* Process the arguments.  Process \ as a special if necessary */

    while ((ip = *(argv++)) != NOWORD)
    {

/* Process the character */

	while ((c = (int)(*(ip++))))
	{

/* If echo too big - disable history save */

	    if ((op - ConsoleLineBuffer) > LINE_MAX - 4)
	    {
		*op = 0;

		if (!WriteOutLine (fid))
		    return 1;

		op = ConsoleLineBuffer;

		if (flags & ECHO_HISTORY)
		    fprintf (stderr, BasicErrorMessage,
			     "Line too long for history", LIT_print);

		flags &= ~ECHO_HISTORY;
	    }

	    if ((flags & ECHO_ESCAPE) && (c == CHAR_META))
	    {
		c1 = *ip;

		if ((c = ProcessOutputMetaCharacters (&ip)) == -1)
		{
		    flags |= ECHO_NO_EOL;
		    continue;
		}

/* If unchanged - output backslash */

		else if ((c == c1) && (c != CHAR_META))
		    *(op++) = CHAR_META;
	    }

	    *(op++) = (char)c;
	}

/* End of string - check to see if a space if required */

	if (*argv != NOWORD)
	    *(op++) = CHAR_SPACE;
    }

/* Is EOL required ? */

    if (!(flags & ECHO_NO_EOL))
    {
#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32)
	if (IS_Console (fid))
	    *(op++) = CHAR_RETURN;
#endif
	*(op++) = CHAR_NEW_LINE;
    }

    *op = 0;

    if (!WriteOutLine (fid))
        return 1;

/* Save history */

    if (flags & ECHO_HISTORY)
    {
	CleanUpBuffer (op - ConsoleLineBuffer, ConsoleLineBuffer, 0x1a);
	AddHistory (FALSE);
    }

    return 0;
}

/*
 * Write out the current line
 */

static bool F_LOCAL WriteOutLine (int fid)
{
    size_t	Len = strlen (ConsoleLineBuffer);

    if (write (fid, ConsoleLineBuffer, Len) != (int)Len)
    {
        PrintWarningMessage ("print: write error on unit %d\n", fid);
	return FALSE;
    }

    return TRUE;

}

/*
 * Display the current version: ver
 */

static int dover (int argc, char **argv)
{
    PrintVersionNumber (stdout);
    return 0;
}

#ifdef OS_SWAPPING
static char	*swap_device[] = {"disk", "extend", "expand"};
#endif

/*
 * Modify swapping information: swap [ options ]
 */

static int doswap (int argc, char **argv)
{
#if (OS_TYPE != OS_DOS)
    printf ("Swapping not available on %s", LIT_OSname);
#elif !defined (OS_SWAPPING)
    puts ("Swapping not available in 32-bit mode");
#else
    int		n = 1;
    char	*cp;

/* Display current values ? */

    if (argv[1] == NOWORD)
    {
	if (Swap_Mode == SWAP_OFF)
	    puts ("Swapping disabled");

	else
	{
	    int		j;

	    foputs ("Swap devices: ");

	    for (j = 0, n = 1; j < 3; ++j, n <<= 1)
	    {
		if (Swap_Mode & n)
		{
		    printf ("%s ", swap_device[j]);

		    if (n == SWAP_EXTEND)
			printf ("(0x%.6lx) ", SW_EMstart);
		}
	    }

	    fputchar (CHAR_NEW_LINE);
	}

	return 0;
    }

/* Set up new values */

    Swap_Mode = SWAP_OFF;
    ClearSwapFile ();

    while ((cp = argv[n++]) != NOWORD)
    {
	if (strcmp (cp, "off") == 0)
	    Swap_Mode = SWAP_OFF;

	else if (strcmp (cp, "on") == 0)
	    Swap_Mode = SWAP_DISK | SWAP_EXPAND | SWAP_EXTEND;

/* Scan for valid arguments */

	else
	{
	    int		j, k;

	    for (j = 0, k = 1; j < 3; ++j, k <<= 1)
	    {
		if (strcmp (cp, swap_device[j]) == 0)
		{
		    Swap_Mode |= k;

/* If extended memory, they can specify the start address as a hex number */

		    if (k == SWAP_EXTEND)
		    {
			char	*sp;
			long	start;

/* Check for not changed */

			if ((sp = argv[n]) == NOWORD)
			    break;

/* Convert hex number */

			if (!ConvertNumericValue (sp, &start, 16))
			    break;

/* Set used and saved new value */

			SW_EMstart = start;
			++n;

			if ((SW_EMstart < 0x100000L) ||
			    (SW_EMstart > 0xf00000L))
			    SW_EMstart = 0x100000L;

			printf ("Extend memory start set to 0x%.6lx\n",
				  SW_EMstart);
		    }

		    break;
		}
	    }

	    if (j == 3)
		return UsageError ("swap [ on | off | disk | expand | extend [ address ] ] ...");
	}
    }

#endif
    return 0;
}

/*
 * Output the current path: pwd [drives]
 */

static int dopwd (int argc, char **argv)
{
    int			i;
    int			Start = 1;
    int			RetVal = 0;
    char		*sp;
    char		ndrive;
    char		ldir[PATH_MAX + 6];
    bool		Physical = FALSE;

    if ((argc > 1) && CheckPhysLogical (argv[1], &Physical))
	Start++;

/* Print the current directories on the selected drive */

    i = Start;

    while ((sp = argv[i++]) != NOWORD)
    {

/* Select the drive and get the path */

	while ((ndrive = *(sp++)))
	{
	    errno = 0;

	    if (!S_getcwd (ldir, GetDriveNumber (ndrive)) || errno)
		RetVal = PrintWarningMessage (BadDrive, ndrive);

	    else if (Physical)
		printf ("%c: path is %s\n", tolower (ndrive),
			GetPhysicalPath (ldir, TRUE));

	    else
		puts (ldir);
	}
    }

/* Print the current directory */

    if (argv[Start] == NOWORD)
    {
	strcpy (ldir, CurrentDirectory->value);
	puts ((Physical) ? GetPhysicalPath (ldir, TRUE) : ldir);
    }

    return RetVal;
}

/*
 * Unset a variable: unset [ flag ] [ variable name... ]
 * Delete a function: unfunction <names ...>
 */

static int dounset (int argc, char **argv)
{
    int			n = 1;
    bool		Fnc = FALSE;
    FunctionList 	*fp;
    char		*cp;
    int			i;

/* -f, functions */

    if (strcmp (*argv, LIT_unfunction) == 0)
	Fnc = TRUE;

    else if ((argc > 1) && (strcmp (argv[1], "-f") == 0))
    {
	n++;
	Fnc = TRUE;
    }

/* Unset the variables, flags or functions */

    while ((cp = argv[n++]) != NOWORD)
    {
	if (!Fnc)
	{
	    UnSetVariable (cp, -1, FALSE);

	    for (i = 0; DisableVariableMap[i].name != (char *)NULL; i++)
	    {
		if (strcmp (DisableVariableMap[i].name, cp) == 0)
		{
		    DisabledVariables |= DisableVariableMap[i].flag;
		    break;
		}
	    }
	}

	else if ((fp = LookUpFunction (cp, TRUE)) != (FunctionList *)NULL)
	    DeleteFunction (fp->tree);
    }

    return 0;
}

/* Delete a variable.  If all is set, system variables can be deleted.
 * This is used to delete the trap functions
 */

void	UnSetVariable (char *cp,	/* Variable name		*/
		       int  OIndex,	/* Index preprocessed value	*/
		       bool all)	/* Any variable			*/
{
    long		Index;
    char		*term;
    bool		ArrayDetected;

/* Unset a flag */

    if (*cp == '-')
    {
	while (*(++cp) != 0)
	{
	    if (islower (*cp))
		FL_CLEAR (*cp);
	}

	SetShellSwitches ();
	return;
    }

/* Ok - unset a variable and not a local value */

    if (!all && !(IS_VariableFC ((int)*cp)))
	return;

/* Check in list */

    if (!GetVariableName (cp, &Index, &term, &ArrayDetected) || *term)
	return;

    if (OIndex != -1)
	Index = OIndex;

/*  Delete a specific entry or all? */

    if (!((ArrayDetected || (OIndex != -1))))
	Index = -1;

    RemoveVariable (cp, (int)Index);
}

/*
 * Delete a variable
 *
 * Index = -1 implies all references
 */

static void F_LOCAL RemoveVariable (char *name,	/* Variable name	*/
				    int  Index)	/* Array index		*/
{
    VariableList	**vpp;
    VariableList	*vp;
    VariableList	dummy;
    void		(*save_signal)(int);

    while (TRUE)
    {
	dummy.name = name;
	dummy.index = (int)Index;

	vpp = (VariableList **)tfind (&dummy, &VariableTree,
				      DeleteAllVariables);

/* If not found, Ignore unset request */

	if (vpp == (VariableList **)NULL)
	    return;

/* Error if read-only */

	vp = *vpp;

	if (vp->status & (STATUS_READONLY | STATUS_CANNOT_UNSET))
	{
	    PrintWarningMessage ("unset: %s %s", vp->name,
				 (vp->status & STATUS_CANNOT_UNSET)
				    ? "cannot be unset" : LIT_IsReadonly);
	    return;
	}

/* Disable signals */

	save_signal = signal (SIGINT, SIG_IGN);

/* Delete it */

	dummy.index = vp->index;
	tdelete (&dummy, &VariableTree, SearchVariable);
	ReleaseMemoryCell ((void *)vp->name);

	if (vp->value != null)
	    ReleaseMemoryCell ((void *)vp->value);

	ReleaseMemoryCell ((void *)vp);

/* Restore signals */

	signal (SIGINT, save_signal);
    }
}

/*
 * TFIND - Search the VARIABLE TREE for an entry to delete
 */

static int DeleteAllVariables (const void *key1, const void *key2)
{
    int			diff;

    if ((diff = strcmp (((VariableList *)key1)->name,
			((VariableList *)key2)->name)) != 0)
	return diff;

    if (((VariableList *)key1)->index == -1)
        return 0;

    return ((VariableList *)key1)->index - ((VariableList *)key2)->index;
}

/*
 * Execute a test: test <arguments>
 */

static int dotest (int argc, char **argv)
{
    int		st = 0;
    char	*End;

    NewTestProgram = (bool)(strcmp (TestProgram = *argv, LIT_Test) == 0);

/* Note that -a and -o change meaning if [[ ... ]] is used */

    if (NewTestProgram)
    {
	End = "]]";
	ListOfTestOperators[0].OperatorID = FILE_EXISTS;
	ListOfTestOperators[0].OperatorType = UNARY_OP;
	ListOfTestOperators[1].OperatorID = TEST_OPTION;
	ListOfTestOperators[1].OperatorType = UNARY_OP;
    }

    else
    {
	End = "]";
	ListOfTestOperators[0].OperatorID = BINARY_AND;
	ListOfTestOperators[0].OperatorType = B_BINARY_OP;
	ListOfTestOperators[1].OperatorID = BINARY_OR;
	ListOfTestOperators[1].OperatorType = B_BINARY_OP;
    }

/* Check out */

    CurrentTestOperator = (struct TestOperator *)NULL;

/* If [ <arguments> ] or [[ <arguments> ]] form, check for end ] or ]] and
 * remove it
 */

    if (NewTestProgram || (strcmp (*argv, "[") == 0))
    {
	while (argv[++st] != NOWORD)
	    ;

	if (strcmp (argv[--st], End) != 0)
	    return PrintWarningMessage ("%s: missing '%s'", TestProgram, End);

	else
	    argv[st] = NOWORD;
    }

/* Check for null expression */

    if (*(TestArgumentList = &argv[1]) == NOWORD)
	return 1;

/* Set abort address */

    if (setjmp (TestErrorReturn))
	return 1;

    st = !TestProcessNextExpression (TestLexicalAnalysis ());

    if (*(TestArgumentList + 1) != NOWORD)
	TestSyntaxError ();

    return (st);
}

/*
 * Process next test expression
 */

static int F_LOCAL TestProcessNextExpression (int n)
{
    int		res;

    if (n == END_OF_INPUT)
	TestSyntaxError ();

    res = TestANDExpression (n);

    TestArgumentList++;

    if (TestLexicalAnalysis () == BINARY_OR)
    {
	TestArgumentList++;

	return TestProcessNextExpression (TestLexicalAnalysis ()) || res;
    }

    TestArgumentList--;
    return res;
}

/*
 * Binary expression ( a AND b)
 */

static int F_LOCAL TestANDExpression (int n)
{
    int res;

    if (n == END_OF_INPUT)
	TestSyntaxError ();

    res = TestPrimaryExpression (n);
    TestArgumentList++;

    if (TestLexicalAnalysis () == BINARY_AND)
    {
	TestArgumentList++;
	return TestANDExpression (TestLexicalAnalysis ()) && res;
    }

    TestArgumentList--;
    return res;
}

/*
 * Handle Primary expression
 */

static int F_LOCAL TestPrimaryExpression (int n)
{
    int			res;

    switch (n)
    {
	case END_OF_INPUT:
	    TestSyntaxError ();

    	case UNARY_NOT:
	    TestArgumentList++;
	    return !TestPrimaryExpression (TestLexicalAnalysis ());

	case LPAREN:
	    TestArgumentList++;
	    res = TestProcessNextExpression (TestLexicalAnalysis ());

	    TestArgumentList++;
	    if (TestLexicalAnalysis () != RPAREN)
		TestSyntaxError ();

	    return res;

/* Operand */

	case OPERAND:
	    return TestBinaryOperand ();

/* unary expression */

	default:
	    if ((CurrentTestOperator->OperatorType != UNARY_OP) ||
		(*++TestArgumentList == 0))
		TestSyntaxError ();

	    return TestUnaryOperand (n);
    }
}

/*
 * Handle a Binary Operand
 */

static int F_LOCAL TestBinaryOperand (void)
{
    char		*opnd1, *opnd2;
    struct stat		s, s1;
    short		op;

    opnd1 = *(TestArgumentList++);
    (void) TestLexicalAnalysis ();

    if ((CurrentTestOperator != (struct TestOperator *)NULL) &&
	(CurrentTestOperator->OperatorType == BINARY_OP))
    {
	op = CurrentTestOperator->OperatorID;

	TestArgumentList++;

	if ((opnd2 = *TestArgumentList) == NOWORD)
	    TestSyntaxError ();

	switch (op)
	{

/* String lengths */

	    case STRING_EQUAL:
		return strcmp (opnd1, opnd2) == 0;

	    case STRING_NOTEQUAL:
		return strcmp (opnd1, opnd2) != 0;

	    case STRING_LESS:
		return strcmp (opnd1, opnd2) < 0;

	    case STRING_GREATER:
		return strcmp (opnd1, opnd2) > 0;

/* Numeric comparisions */

	    case NUMBER_EQUAL:
		return GetNumberForTest (opnd1) == GetNumberForTest (opnd2);

	    case NUMBER_NOTEQUAL:
		return GetNumberForTest (opnd1) != GetNumberForTest (opnd2);

	    case NUMBER_EQ_GREAT:
		return GetNumberForTest (opnd1) >= GetNumberForTest (opnd2);

	    case NUMBER_GREATER:
		return GetNumberForTest (opnd1) > GetNumberForTest (opnd2);

	    case NUMBER_EQ_LESS:
		return GetNumberForTest (opnd1) <= GetNumberForTest (opnd2);

	    case NUMBER_LESS:
		return GetNumberForTest (opnd1) < GetNumberForTest (opnd2);

/* Older and Newer - if file not found - set to current time */

	    case FILE_NEWER:
	    case FILE_OLDER:
		if (!S_stat (opnd1, &s))
		    return 0;

		if (!S_stat (opnd2, &s1))
		    s1.st_mtime = 0L;

		return (op == FILE_NEWER) ? (s.st_mtime > s1.st_mtime)
					  : (s.st_mtime < s1.st_mtime);

/*
 * Equals - difficult on DOS.  So just do want UNIX says, but first compare
 * the absolute path names
 */

	    case FILE_EQUAL:
	    {
#if (OS_TYPE != OS_UNIX)
		char	*a_opnd1;
		char	*a_opnd2;
		int	res;
#endif

		if ((!S_stat (opnd1, &s)) || (!S_stat (opnd2, &s1)))
		    return 0;

#if (OS_TYPE == OS_UNIX)
		return ((s.st_dev == s1.st_dev) && (s.st_ino == s1.st_ino));
#else

		a_opnd1 = AllocateMemoryCell (FFNAME_MAX);
		a_opnd2 = AllocateMemoryCell (FFNAME_MAX);

		if ((a_opnd1 == (char *)NULL) || (a_opnd1 == (char *)NULL))
		{
		    doOutofMemory (LIT_Test);
		    return 0;
		}

		GenerateFullExecutablePath (strcpy (a_opnd1, opnd1));
		GenerateFullExecutablePath (strcpy (a_opnd2, opnd2));

/* If this is OS/2, we need to decide if to check for HPFS */

#  if (OS_TYPE != OS_DOS)
		res = ((!IsHPFSFileSystem (a_opnd1)) ||
		       (ShellGlobalFlags & FLAGS_NOCASE))
			    ? stricmp (a_opnd1, a_opnd2)
			    : strcmp (a_opnd1, a_opnd2);
#  else
		res = stricmp (a_opnd1, a_opnd2);
#  endif

		ReleaseMemoryCell (a_opnd1);
		ReleaseMemoryCell (a_opnd2);

		return (res == 0) ? 1
				  : ((s.st_dev == s1.st_dev) &&
				     (s.st_ino == s1.st_ino));
#endif
	    }
	}
    }

    TestArgumentList--;
    return strlen (opnd1) != 0;
}

/*
 * Handle a Unary Operand
 */

static int F_LOCAL TestUnaryOperand (int n)
{
    switch (n)
    {
	case STRING_ZERO:
	    return strlen (*TestArgumentList) == 0;

	case STRING_NONZERO:
	    return strlen (*TestArgumentList) != 0;

	case TEST_OPTION:
	    return TestOptionValue (*TestArgumentList, TRUE) != 0;

/* File functions */

	case FILE_EXISTS:
	    return CheckFAccess (*TestArgumentList, F_OK);

	case FILE_READABLE:
	    return CheckFAccess (*TestArgumentList, R_OK);

	case FILE_WRITABLE:
	    return CheckFAccess (*TestArgumentList, W_OK);

	case FILE_EXECUTABLE:
	    return CheckFAccess (*TestArgumentList, X_OK);

	case FILE_REGULAR:
	    return CheckFType (*TestArgumentList, S_IFREG);

	case FILE_DIRECTRY:
	    return CheckFType (*TestArgumentList, S_IFDIR);

	case FILE_NONZERO:
	    return CheckFSize (*TestArgumentList);

	case FILE_TERMINAL:
	    return IS_TTY ((int)GetNumberForTest (*TestArgumentList));

/* The following have no meaning on MSDOS or OS/2.  So we always return
 * fail for compatability
 */

#if (OS_TYPE == OS_UNIX)
	case FILE_USER:
	    return CheckFMode (*TestArgumentList, S_ISUID);

	case FILE_GROUP:
	    return CheckFMode (*TestArgumentList, S_ISGID);

	case FILE_TEXT:
	    return CheckFMode (*TestArgumentList, S_ISVTX);
#else
	case FILE_USER:
	    return CheckFMode (*TestArgumentList, OS_FILE_HIDDEN);

	case FILE_GROUP:
	    return CheckFMode (*TestArgumentList, OS_FILE_SYSTEM);

	case FILE_TEXT:
	    return CheckFMode (*TestArgumentList, OS_FILE_ARCHIVED);
#endif

	case FILE_BLOCK:
	    return CheckFType (*TestArgumentList, S_IFBLK);

	case FILE_CHARACTER:
	    return CheckFType (*TestArgumentList, S_IFCHR);

#if (OS_TYPE == OS_UNIX)
	case FILE_FIFO:
	    return CheckFType (*TestArgumentList, S_IFIFO);

	case FILE_SYMBOLIC:
	    return CheckFType (*TestArgumentList, S_IFLNK);

	case FILE_SOCKET:
	    return CheckFType (*TestArgumentList, S_IFSOCK);
#else
	case FILE_FIFO:
	case FILE_SYMBOLIC:
	case FILE_SOCKET:
	    return 0;
#endif

/* Under MSDOS and OS/2, we always own the file.  Not necessarily true on
 * networked versions.  But there is no common way of finding out
 */

#if (OS_TYPE == OS_UNIX)
	case FILE_OWNER:
	    return CheckFOwner (*TestArgumentList);

	case FILE_GROUPER:
	    return CheckFGroup (*TestArgumentList);
#else
	case FILE_OWNER:
	case FILE_GROUPER:
	    return 1;
#endif
    }

    return 0;
}

/* Operator or Operand ? */

static int F_LOCAL TestLexicalAnalysis (void)
{
    struct TestOperator		*op;
    char			*s = *TestArgumentList;

    if (s == NOWORD)
	return END_OF_INPUT;

/* This is a real pain.  If the current string is a unary operator and the
 * next string is a binary operator, assume the current string is a parameter
 * to the binary operator and not a unary operator.
 */

    if ((CurrentTestOperator = TestLookupOperator (s))
			     != (struct TestOperator *)NULL)
    {
	s = *(TestArgumentList + 1);

	if ((CurrentTestOperator->OperatorType != UNARY_OP) ||
	    (s == (char *)NULL) ||
	    ((op = TestLookupOperator (s)) == (struct TestOperator *)NULL) ||
	    (op->OperatorType != BINARY_OP))
	    return CurrentTestOperator->OperatorID;
    }

    CurrentTestOperator = (struct TestOperator *)NULL;
    return OPERAND;
}

/*
 * Look up the string and test for operator
 */

static struct TestOperator * F_LOCAL TestLookupOperator (char *s)
{
    struct TestOperator		*op = ListOfTestOperators;

    while (op->OperatorName)
    {
	if (strcmp (s, op->OperatorName) == 0)
	    return op;

	op++;
    }

    return (struct TestOperator *)NULL;
}

/*
 * Get a long numeric value
 */

static long F_LOCAL GetNumberForTest (char *s)
{
    long	l;

    if (!ConvertNumericValue (s, &l, 10))
	TestSyntaxError ();

    return l;
}

/*
 * test syntax error - abort
 */

static void F_LOCAL TestSyntaxError (void)
{
    PrintWarningMessage (BasicErrorMessage, TestProgram, LIT_SyntaxError);
    longjmp (TestErrorReturn, 1);
}

/*
 * Select a new drive: x:
 *
 * Select the drive, get the current directory and check that we have
 * actually selected the drive
 */

static int dodrive (int argc, char **argv)
{
    unsigned int	ndrive = GetDriveNumber (**argv);
    char		ldir[PATH_MAX + 6];
    bool		bad = FALSE;

    if (argc != 1)
	return UsageError ("drive:");

    if (SetCurrentDrive (ndrive) == -1)
	bad = TRUE;

    else if (!S_getcwd (ldir, ndrive))
	bad = TRUE;
    
    else
    {
	GetCurrentDirectoryPath ();

	if (ndrive != GetCurrentDrive ())
	    bad = TRUE;
    }

    if (bad)
	return PrintWarningMessage (BadDrive, **argv);

    else
	return 0;
}

/*
 * Check for -L or -P switch
 */

static bool F_LOCAL CheckPhysLogical (char *string, bool *physical)
{
    if (strcmp (string, "-L") == 0)
    {
	*physical = FALSE;
	return TRUE;
    }

    if (strcmp (string, "-P") == 0)
    {
	*physical = TRUE;
	return TRUE;
    }

    return FALSE;
}

/*
 * Select a new directory:
 *
 * cd [ directory ]			Select new directory
 * cd - 				Select previous directory
 * cd <search string> <new string>	Select directory based on current
 * cd					Select Home directory
 * cd [-L | -P]				Use Logical or physical mapping
 *					sort of symbolic links
 */

static int dochdir (int argc, char **argv)
{
    char		*NewDirectory;	/* Original new directory	*/
    char		*CNDirectory;	/* New directory		*/
    char		*cp;		/* In CDPATH Pointer		*/
    char		*directory;
    bool		first = TRUE;
    unsigned int	Length;
    unsigned int	cdrive;
    bool		Physical = FALSE;
    int			Start = 1;
    int			dcount;

/* If restricted shell - illegal */

    if (CheckForRestrictedShell ("cd"))
	return 1;

    if ((argc > 1) && CheckPhysLogical (argv[1], &Physical))
	Start = 2;

    if (argc - Start > 2)
	return UsageError ("cd [ -P | -L ] [ directory | - | search replace ]");

/* Use default ? */

    if (((NewDirectory = argv[Start]) == NOWORD) &&
	((NewDirectory = GetVariableAsString (HomeVariableName,
					      FALSE)) == null))
	return PrintWarningMessage ("cd: no home directory");

    if ((directory = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	return doOutofMemory ("cd");

    if ((strcmp (NewDirectory, ShellOptionsVariable) == 0) &&
	((NewDirectory = GetVariableAsString (OldPWDVariable, FALSE)) == null))
	return PrintWarningMessage ("cd: no previous directory");

/* Check for substitue */

    if ((argv[Start] != NOWORD) && (argv[Start + 1] != NOWORD))
    {
	if ((cp = strstr (CurrentDirectory->value, argv[Start]))
		== (char *)NULL)
	    return PrintWarningMessage ("cd: string not in pwd: %s",
					argv[Start]);

	if (strlen (CurrentDirectory->value) - strlen (argv[Start]) +
	    strlen (argv[Start + 1]) >= (size_t)FFNAME_MAX)
	    return PrintWarningMessage ("cd: new directory string too long: %s",
					argv[Start]);
/* Do the substitution */

	Length = cp - CurrentDirectory->value;
	strncpy (NewDirectory, CurrentDirectory->value, Length);
	strcpy (NewDirectory + Length, argv[Start + 1]);
	strcat (NewDirectory,
		CurrentDirectory->value + strlen (argv[Start]) + Length);
    }

/* Save the current drive */

    cdrive = GetCurrentDrive ();

/* Remove trailing / */

    Length = strlen (NewDirectory) - 1;

    if (IsPathCharacter (NewDirectory[Length]) &&
	(!((!Length) || ((Length == 2) && IsDriveCharacter (NewDirectory[1])))))
	NewDirectory[Length] = 0;

/*
 * Special case for . and .., it seems
 */

    if ((strcmp (NewDirectory, CurrentDirLiteral) == 0) &&
	S_chdir (NewDirectory))
    {
	GetCurrentDirectoryPath ();
	return  0;
    }

    if (strcmp (NewDirectory, ParentDirLiteral) == 0)
    {
	if (S_chdir (NewDirectory))
	{
	    GetCurrentDirectoryPath ();
	    return  0;
	}

/* If we are in the root directory, .. does not move you! */

	dcount = 0;
	cp = CurrentDirectory->value;

	while ((cp = strchr (cp, CHAR_UNIX_DIRECTORY)) != (char *)NULL)
	{
	    cp++;

	    if (++dcount == 2)
	        break;
	}

	if (dcount == 1)
	{
	    GetCurrentDirectoryPath ();
	    return  0;
	}
    }


/* Scan for the directory.  If there is not a / or : at start, use the
 * CDPATH variable
 */

    cp = (IsPathCharacter (*NewDirectory) ||
          IsDriveCharacter (*(NewDirectory + 1)))
		? null : GetVariableAsString (CDPathLiteral, FALSE);

    do
    {
	cp = BuildNextFullPathName (cp, NewDirectory, directory);

/* Check for new disk drive */

	if (Physical && (GetPhysicalPath (directory, FALSE) == (char *)NULL))
	    return doOutofMemory ("cd");

	CNDirectory = directory;

/* Was the change successful? */

	if (GotoDirectory (CNDirectory, cdrive))
	{

/* OK - reset the current directory (in the shell) and display the new
 * path if appropriate
 */

	    GetCurrentDirectoryPath ();

	    if (!first)
		puts (CurrentDirectory->value);

	    return 0;
	}

	first = FALSE;

    } while (cp != (char *)NULL);

/* Restore our original drive and restore directory info */

    GetCurrentDirectoryPath ();

    return PrintWarningMessage (BasicErrorMessage, NewDirectory,
				"bad directory");
}

/*
 * Execute a shift command: shift [ n ]
 */

static int doshift (int argc, char **argv)
{
    int		n;
    char	*Nvalue = argv[1];

    if (argc > 2)
	return UsageError ("shift [ count ]");

    n = (Nvalue != NOWORD) ? GetNumericValue (Nvalue) : 1;

    if (n < 0)
	return PrintWarningMessage (LIT_Emsg, LIT_shift, "bad shift value",
				    Nvalue);

    if (ParameterCount < n)
	return PrintWarningMessage (BasicErrorMessage, LIT_shift,
				    "nothing to shift");

    return SetUpNewParameterVariables (ParameterArray, n + 1,
				       ParameterCount + 1, LIT_shift);
}

/*
 * Execute a umask command: umask [ n ]
 */

static int doumask (int argc, char **argv)
{
    int		i;
    char	*cp;
    long	value;

    if (argc > 2)
	return UsageError ("umask [ new mask ]");

    if ((cp = argv[1]) == NOWORD)
    {
	umask ((i = umask (0)));
	printf ("%o\n", i);
    }

    else if (!ConvertNumericValue (cp, &value, 8))
	return PrintWarningMessage ("umask: bad mask (%s)", cp);

#if (__OS2__)
    else if (umask ((int)value) == 0xffff)
	return PrintWarningMessage ("umask: bad value (%s)", cp);
#else
    umask ((int)value);
#endif

    return 0;
}

/*
 * Execute an exec command: exec [ arguments ]
 */

int	doexec (C_Op *t)
{
    int		i = 0;
    jmp_buf	ex;
    ErrorPoint	OldERP;

/* Shift arguments */

    do
    {
	t->args[i] = t->args[i + 1];
	i++;

    } while (t->args[i] != NOWORD);

/* Left the I/O as it is */

    if (i == 1)
	return RestoreStandardIO (0, FALSE);

    ProcessingEXECCommand = TRUE;
    OldERP = e.ErrorReturnPoint;

/* Set execute function recursive level to zero */

    Execute_stack_depth = 0;
    t->ioact = (IO_Actions **)NULL;

    if (SetErrorPoint (ex) == 0)
	ExecuteParseTree (t, NOPIPE, NOPIPE, EXEC_WITHOUT_FORK);

/* Clear the extended file if an interrupt happened */

    ClearExtendedLineFile ();
    e.ErrorReturnPoint = OldERP;
    ProcessingEXECCommand = FALSE;
    return 1;
}

/*
 * Execute a script in the current shell: . <filename>
 */

static int dodot (int argc, char **argv)
{
    int		i;
    char	*sp;
    char	*cp;
    char	*l_path;
    Source	*s;
    FILE	*Fp;

    if ((cp = argv[1]) == NOWORD)
	return 0;

/* Get some space */

    if ((l_path = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	return doOutofMemory (".");

/* Save the current drive */

    sp = ((FindPathCharacter (cp) != (char *)NULL) || IsDriveCharacter (*(cp + 1)))
	  ? null
	  : GetVariableAsString (PathLiteral, FALSE);

    do
    {
	sp = BuildNextFullPathName (sp, cp, l_path);

	if ((i = OpenForExecution (l_path, (char **)NULL, (int *)NULL)) >= 0)
	{
	    if ((Fp = ReOpenFile (ReMapIOHandler (i),
	    			  sOpenReadMode)) == (FILE *)NULL)
		return PrintWarningMessage ("Cannot remap file");

	    (s = pushs (SFILE))->u.file = Fp;
	    s->file = cp;

	    RunACommand (s, &argv[1]);
	    S_fclose (Fp, TRUE);

	    return (int)GetVariableAsNumeric (StatusVariable);
	}

    } while (sp != (char *)NULL);

    return PrintWarningMessage (BasicErrorMessage, cp, NotFound);
}

/*
 * Read from standard input into a variable list
 *
 * read [-prs] [-u unit] [ variable list ]
 */

static int doread (int argc, char **argv)
{
    char	*cp, *op;
    int		i;
    int		Unit = STDIN_FILENO;
    bool	EndOfInputDetected = FALSE;
    int		PreviousCharacter = 0;
    bool	SaveMode = FALSE;
    bool	RawMode = FALSE;
    char	*Prompt = (char *)NULL;
    char	*NewBuffer;
    int		eofc;

    if ((NewBuffer = AllocateMemoryCell (LINE_MAX + 2)) == (char *)NULL)
	return doOutofMemory (LIT_read);

/* Check for variable name.  If not defined, use $REPLY */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((i = GetOptions (argc, argv, "prsu:", 0)) != EOF)
    {
	switch (i)
	{
	    case 'p':			/* Clean up process		*/
		break;

	    case 'r':			/* Raw Mode			*/
		RawMode = TRUE;
		break;

	    case 's':			/* Save a command		*/
		SaveMode = TRUE;
		break;

	    case 'u':			/* Specify input unit		*/
		if ((Unit = GetUnitNumber (LIT_read)) == -1)
		    return 2;

	    default:
		return UsageError ("read [ -prs ] [ -u unit ] [ name?prompt ] [ name ... ]");
	}
    }

    argv += OptionIndex;
    argc -= OptionIndex;

    if (!argc)
	argv = Reply_Array;

/* Get the prompt and write it */

    LastUserPrompt = null;

    if ((Prompt = strchr (argv[0], '?')) != (char *)NULL)
    {
	*(Prompt++) = 0;
	LastUserPrompt1 = Prompt;
	LastUserPrompt = (char *)NULL;
    }

/* Check we have a name */

    if (!strlen (argv[0]))
	argv = Reply_Array;

/* Check for valid names */

    for (i = 0; argv[i] != (char *)NULL; i++)
    {
	if (IsValidVariableName (argv[i]))
	{
	    PrintErrorMessage (BasicErrorMessage, argv[i], LIT_BadID);
	    return 1;
	}
    }

/* Clear the history buffer */

    FlushHistoryBuffer ();

/* Read the line from the device */

    if (ReadALine (Unit, Prompt, SaveMode, FALSE, &eofc))
	return 1;

    cp = ConsoleLineBuffer;

/* For each variable, read the data until a white space is detected */

    while (*argv != NOWORD)
    {

/* Read in until end of line, file or a field separator is detected */

	op = (char *)NULL;

	while (!EndOfInputDetected && *cp)
	{

/* End of file */

	    if (*cp == (char)eofc)
		return 1;

/* Check for Newline or IFS character */

	    if ((*cp == CHAR_NEW_LINE) ||
		((argv[1] != NOWORD) &&
		 strchr (GetVariableAsString (IFS, FALSE),
			 *cp) != (char *)NULL))
	    {
		if (*cp != CHAR_NEW_LINE)
		     ;

		else if ((PreviousCharacter != CHAR_META) || (RawMode))
		    EndOfInputDetected = TRUE;

/* Handle continuation line */

		else if (ReadALine (Unit, Prompt, SaveMode, TRUE, &eofc))
		    return 1;

		else
		{
		    cp = ConsoleLineBuffer;
		    PreviousCharacter = 0;
		    continue;
		}

		break;
	    }

/* Save the current character */

	    if (op == (char *)NULL)
		op = NewBuffer;

	    *(op++) = *cp;
	    PreviousCharacter = *(cp++);
	}

/* Skip over terminating character */

	if (*cp)
	    ++cp;

/* Check for null string */

	if (op == (char *)NULL)
	    op = null;

/* Terminate the string */

	else
	{
	    *op = 0;

	    if (!strlen (NewBuffer))
		continue;

	    else
		op = NewBuffer;
	}

/* Save the string value */

	SetVariableFromString (*(argv++), op);
    }

    ReleaseMemoryCell ((void *)NewBuffer);

    return 0;
}

/*
 * Read a line from either the console or a file for the read command.
 * The line is returned in the ConsoleLineBuffer.
 */

static bool F_LOCAL ReadALine (int  Unit,	/* Unit to read from	*/
			       char *Prompt,	/* User prompt		*/
			       bool SaveMode,	/* Save in history	*/
			       bool Append,	/* Append to history	*/
			       int *eofc)	/* EOF character	*/
{
    int		NumberBytesRead;
    char	*cp;
    int		x;

/* Generate the prompt */

    if ((Prompt != (char *)NULL) && (!IS_Console (Unit)) &&
	(!IS_TTY (Unit) || (write (Unit, Prompt, strlen (Prompt)) == -1)))
	feputs (Prompt);

/* Read the line */

    *eofc = 0x1a;

    if (IS_Console (Unit))
    {
	*eofc = GetEOFKey ();

	if (!GetConsoleInput ())		/* get input	*/
	    strcpy (ConsoleLineBuffer, LIT_NewLine);
    }

    else
    {
	NumberBytesRead = 0;
	cp =  ConsoleLineBuffer;

	while (NumberBytesRead++ < LINE_MAX)
	{
	    if ((x = read (Unit, cp, 1)) == -1)
		return TRUE;

/* EOF detected as first character on line */

	    if ((NumberBytesRead == 1) && ((!x) || (*cp == (char)*eofc)))
		return TRUE;

/* End read if NEWLINE or EOF character detected */

	    if ((!x) || (*cp == CHAR_NEW_LINE) || (*cp == (char)*eofc))
		break;

	    cp++;
	}

/* Terminate the line the same in all cases */

	*(cp++) = CHAR_NEW_LINE;
	*cp = 0;
    }

/* Save the history.  Clean it up first */

    if (SaveMode)
    {
	char	save;

	save = CleanUpBuffer (strlen (ConsoleLineBuffer), ConsoleLineBuffer,
			      *eofc);
	AddHistory (Append);
	ConsoleLineBuffer[strlen(ConsoleLineBuffer)] = save;
    }

    return FALSE;
}

/*
 * Evaluate an expression: eval <expression>
 */

static int doeval (int argc, char **argv)
{
    Source	*s;

    (s = pushs (SWORDS))->u.strv = argv + 1;
    return RunACommand (s, (char **)NULL);
}

/*
 * Map signals.  
 *
 * Numeric values are assumed to be UNIX - map to appropriate DOS signal.
 * Character values are just mapped to the DOS signal
 */

static struct TrapSignalList * F_LOCAL LookupSignalName (char *name)
{
    static struct TrapSignalList	Numeric = {NULL, 0};
    int				n;

    if (isdigit (*name))
    {
	if (((n = GetNumericValue (name)) < 0) || (n >= MAX_SIG_MAP))
	    return (struct TrapSignalList *)NULL;

#if (OS_TYPE == OS_UNIX)
	Numeric.signo = n;
#else
	Numeric.signo = UnixToDosSignals [n];
#endif
	return &Numeric;
    }

/* Check the character names */

    for (n = 0; n < MAX_TRAP_SIGNALS; n++)
    {
        if (stricmp (name, TrapSignalList[n].name) == 0)
	    return &TrapSignalList[n];
    }

    return (struct TrapSignalList *)NULL;
}

/*
 * Execute a trap: trap [ number... ] [ command ]
 */

static int dotrap (int argc, char **argv)
{
    int				i;
    bool			SetSignal;
    char			tval[10];
    char			*cp;
    struct TrapSignalList	*SignalInfo;
    void			(*NewSignalFunc)(int);

    if ((argc == 2) && (strcmp (argv[1], "-l") == 0))
    {
	for (i = 3; i < MAX_TRAP_SIGNALS; i++)
	    puts (TrapSignalList[i].name);

	return 0;
    }

/* Display active traps ? */

    if (argc < 2)
    {

/* Display trap - look up each trap and print those we find */

	for (i = 0; i < NSIG; i++)
	{
	    sprintf (tval, "~%d", i);

	    if ((cp = GetVariableAsString (tval, FALSE)) != null)
		printf ("%u: %s\n", i, cp);
	}

	if ((cp = GetVariableAsString (Trap_DEBUG, FALSE)) != null)
	    printf (BasicErrorMessage, &Trap_DEBUG[1], cp);

	if ((cp = GetVariableAsString (Trap_ERR, FALSE)) != null)
	    printf (BasicErrorMessage, &Trap_ERR[1], cp);

	return 0;
    }

/* Check to see if signal re-set */

    SetSignal = C2bool(LookupSignalName (argv[1]) ==
				(struct TrapSignalList *)NULL);

    for (i = SetSignal ? 2 : 1; argv[i] != NOWORD; ++i)
    {
	if ((SignalInfo = LookupSignalName (argv[i]))
			== (struct TrapSignalList *)NULL)
	    return PrintWarningMessage ("trap: bad signal number - %s",
	    				argv[i]);

/* Check for no UNIX to DOS mapping */

	if (SignalInfo->signo == SIG_NO_MAP)
	{
	    if (!FL_TEST (FLAG_WARNING))
		PrintWarningMessage ("trap: No UNIX to DOS signal map for %s",
				     argv[i]);

	    continue;
	}

/* Check for ERR or DEBUG.  cp points to the variable name */

	if (SignalInfo->signo == SIG_SPECIAL)
	    cp = (SignalInfo->name) - 1;

/* Generate the variable name for a numeric value */

	else
	    sprintf (cp = tval, "~%d", SignalInfo->signo);

/* Remove the old processing */

	RemoveVariable (cp, 0);

/* Default to new function of ignore! */

	NewSignalFunc = SIG_DFL;

/* Re-define signal processing */

	if (SetSignal)
	{
	    if (*argv[1] != 0)
	    {
		SetVariableFromString (cp, argv[1]);
		NewSignalFunc = TerminateSignalled;
	    }

	    else
		NewSignalFunc = SIG_IGN;
	}

/* Clear signal processing */

	else if (InteractiveFlag)
	{
	    if (SignalInfo->signo == SIGINT)
		NewSignalFunc = InterruptSignalled;

#ifdef SIGQUIT
	    else 
		NewSignalFunc = (SignalInfo->signo == SIGQUIT)
					? SIG_IGN : SIG_DFL;
#endif
	}

/* Set up new signal function */

	if (SignalInfo->signo > 0)
	    signal (SignalInfo->signo, NewSignalFunc);

    }

    return 0;
}

/*
 * BREAK and CONTINUE processing: break/continue [ n ]
 */

static int dobreak (int argc, char **argv)
{
    if (argc > 2)
	return UsageError ("break [ count ]");

    return BreakContinueProcessing (argv[1], BC_BREAK);
}

static int docontinue (int argc, char **argv)
{
    if (argc > 2)
	return UsageError ("continue [ count ]");

    return BreakContinueProcessing (argv[1], BC_CONTINUE);
}

static int F_LOCAL	BreakContinueProcessing (char *NumberOfLevels,
						 int Type)
{
    Break_C	*Break_Loc;
    int		LevelNumber;
    char	*cType = (Type == BC_BREAK) ? LIT_break : LIT_continue;


    LevelNumber = (NumberOfLevels == (char *)NULL)
			? 1 : GetNumericValue (NumberOfLevels);

    if (LevelNumber < 0)
	return PrintWarningMessage (LIT_Emsg, cType, "bad level number",
				    NumberOfLevels);

/* If level is invalid - clear all levels */

    if (LevelNumber <= 0)
	LevelNumber = 999;

/* Move down the stack */

    do
    {
	if ((Break_Loc = Break_List) == (Break_C *)NULL)
	    break;

	Break_List = Break_Loc->NextExitLevel;

    } while (--LevelNumber);

/* Check level */

    if (LevelNumber)
	return PrintWarningMessage (BasicErrorMessage, cType, "bad level");

    longjmp (Break_Loc->CurrentReturnPoint, Type);

/* NOTREACHED */
    return 1;
}

/*
 * Exit function: exit [ status ]
 */

static int doexit (int argc, char **argv)
{
    Break_C	*SShell_Loc = SShell_List;

    ProcessingEXECCommand = FALSE;

    if (argc > 2)
	return UsageError ("exit [ status ]");

/* Set up error codes */

    ExitStatus = (argv[1] != NOWORD) ? GetNumericValue (argv[1]) : 0;

    SetVariableFromNumeric (StatusVariable, (long)abs (ExitStatus));

/* Are we in a subshell.  Yes - do a longjmp instead of an exit */

    if (SShell_Loc != (Break_C *)NULL)
    {
	SShell_List = SShell_Loc->NextExitLevel;
	longjmp (SShell_Loc->CurrentReturnPoint, ExitStatus);
    }

/*
 * Check for active jobs
 */

#if (OS_TYPE != OS_DOS)
    if (!ExitWithJobsActive)
    {
	if (NumberOfActiveJobs () && InteractiveFlag)
	{
	    feputs ("You have running jobs.\n");
	    ExitWithJobsActive = TRUE;
	    return 0;
	}
    }
#endif

    ExitTheShell (FALSE);
    return ExitStatus;
}

/*
 * Function return: return [ status ]
 *
 * Set exit value and return via a long jmp
 */

static int doreturn (int argc, char **argv)
{
    Break_C	*Return_Loc = Return_List;
    int		Retval = 0;

    if ((argc > 2) || 
	((argc == 2) && ((Retval = GetNumericValue (argv[1])) == -1)))
	return UsageError (ReturnUsage);

    SetVariableFromNumeric (StatusVariable, (long) abs (Retval));

/* If the return address is defined - return to it.  Otherwise, return
 * the value
 */

    if (Return_Loc != (Break_C *)NULL)
    {
	Return_List = Return_Loc->NextExitLevel;
	longjmp (Return_Loc->CurrentReturnPoint, 1);
    }

    return Retval;
}

/*
 * Set function:  set [ -/+flags ] [ parameters... ]
 */

static int doset (int argc, char **argv)
{
    int			i;

/* Display ? */

    if (argc < 2)
	return ListAllVariables (0xffff, TRUE);

/* Set/Unset a flag ? */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((i = GetOptions (argc, argv, "VMA:abcdefghijklmno:pqrstuvwxyz",
			    GETOPT_PLUS | GETOPT_AMISSING)) != EOF)
    {
	switch (i)
	{
	    case '?':			/* Unknown */
		if (BadOptionValue != 'o')
		    return UsageError ("set [ [-|+][switches] ] [ [-|+]o option ] [ parameter=value ] args");
		
		return PrintOptionSettings ();

	    case 'r':
		return PrintWarningMessage ("set: r switch cannot be changed");

	    case 'o':
		if ((!ChangeInitialisationValue (OptionArgument,
					        OptionStart == '-')) &&
		    (!ChangeOptionValue (OptionArgument,
					 (bool)(OptionStart == '-'))))
		    return PrintWarningMessage ("set: -o bad option (%s)",
						OptionArgument);

		break;

	    case 'A':
		if (OptionStart == '-')		/* If -, remove all values */
		    RemoveVariable (OptionArgument, -1);

		i = 0;

		while (argv[OptionIndex] != NOWORD)
		    SetVariableArrayFromString (OptionArgument, i++,
						argv[OptionIndex++]);

		return 0;

	    case 'M':
		if (OptionStart == '-')		/* If -, remove all values */
		    ShellGlobalFlags |= FLAGS_MSDOS_FORMAT;

		else
		    ShellGlobalFlags &= ~FLAGS_MSDOS_FORMAT;

	        break;
	    
	    case 'V':
		SetVerifyStatus (C2bool (OptionStart == '-'));
		break;

	    default:
		if ((i == 'e') && InteractiveFlag)
		    continue;

		SetClearFlag (i, (bool)(OptionStart == '-'));
	}
    }

    SetShellSwitches ();

/* Check for --, ++, - and +, which we skip */

    if ((OptionIndex != argc) &&
	((!strcmp (argv[OptionIndex], DoubleHypen))		||
	 (!strcmp (argv[OptionIndex], ShellOptionsVariable))	||
	 (!strcmp (argv[OptionIndex], "+"))))
	OptionIndex++;

/* Set up parameters ? */

    if (OptionIndex != argc)
    {
	ResetGetoptsValues (TRUE);
	return SetUpNewParameterVariables (argv, OptionIndex, argc, "set");
    }

    else
	return 0;
}


/*
 * Print the list of functions: functions [ names ]
 */

static int dofunctions (int argc, char **argv)
{
    FunctionList	*fp;
    int			i;

    if (argc < 2)
	return PrintAllFunctions ();

    for (i = 1; argv[i] != NOWORD; ++i)
    {
	if ((fp = LookUpFunction (argv[i], TRUE)) != (FunctionList *)NULL)
	    PrintFunction (fp->tree, PF_MODE_NORMAL);

	else
	    PrintWarningMessage ("functions: %s is not a function", argv[i]);
    }

    return 0;
}

/*
 * History functions - history [-ieds]
 */

static int dohistory (int argc, char **argv)
{
    int		i;
    int		Start;
    long	value;

    if (!InteractiveFlag)
	return 1;

    if (argc < 2)
        Start = GetLastHistoryEvent () + 1;

/*
 * Check for options
 */

    else if (**(argv + 1) == CHAR_SWITCH)
    {
	ResetGetOptions ();		/* Reset GetOptions		*/

	while ((i = GetOptions (argc, argv, "sidel", 0)) != EOF)
	{
	    switch (i)
	    {
		case 's':
		    DumpHistory ();
		    break;

		case 'i':
		    ClearHistory ();
		    break;

		case 'l':
		    LoadHistory ();
		    break;

		case 'd':
		    HistoryEnabled = FALSE;
		    break;

		case 'e':
		    HistoryEnabled = TRUE;
		    break;

		default:
		    return UsageError (HistoryUsage);
	    }
	}

	return (OptionIndex != argc) ? UsageError (HistoryUsage) : 0;
    }

/* Check for number of display */

    else if ((argc == 2) && ConvertNumericValue (*(argv + 1), &value, 0))
	Start = (int)value + 1;

    else
	return UsageError (HistoryUsage);

/*
 * Display history
 */

    if ((i = (GetLastHistoryEvent () + 1) - Start) < 0)
	i = 0;

    PrintHistory (FALSE, TRUE, i, GetLastHistoryEvent () + 1, stdout);
    return 0;
}

/*
 * Type function: type [ command ]
 *
 * For each name, indicate how it would be interpreted
 */

static int dowhence (int argc, char **argv)
{
    char		*cp;
    int			n;			/* Argument count	*/
    int			inb;			/* Inbuilt function	*/
    bool		v_flag;
    bool		p_flag = FALSE;
    bool		t_flag = FALSE;
    char		*l_path;
    AliasList		*al;

/* Get some memory for the buffer */

    if ((l_path = AllocateMemoryCell (FFNAME_MAX + 4)) == (char *)NULL)
	return doOutofMemory (*argv);

    v_flag = (bool)(strcmp (*argv, LIT_type) == 0);

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((n = GetOptions (argc, argv, "pvt", 0)) != EOF)
    {
	switch (n)
	{
	    case 'v':	v_flag = TRUE;	break;
	    case 'p':	p_flag = TRUE;	break;
	    case 't':	t_flag = TRUE;	break;

	    default:
		return UsageError (WhenceUsage);
	}
    }

/* Process each parameter */

    while ((cp = argv[OptionIndex++]) != NOWORD)
    {

/* Check for alias */

	if ((al = LookUpAlias (cp, FALSE)) != (AliasList *)NULL)
	{
	    if (v_flag)
		printf ("%s is %s alias for %s", cp,
			(al->AFlags & ALIAS_TRACKED) ? "a tracked"
						     : "an", al->value);

	    else
		foputs (al->value);
	}


/* Check for currently use inbuilt version */

	else if (!p_flag && IsCommandBuiltIn (cp, &inb) && (inb & BLT_CURRENT))
	    WhenceLocation (v_flag, cp, ShellInternalCommand);

/* Check for a function */

	else if (!p_flag &&
		 (LookUpFunction (cp, FALSE) != (FunctionList *)NULL))
	    WhenceLocation (v_flag, cp, "is a function");

/* Scan the path for an executable */

	else if (FindLocationOfExecutable (l_path, cp) != EXTENSION_NOT_FOUND)
	{
	    PATH_TO_LOWER_CASE (l_path);

	    if (v_flag)
		printf ("%s is ", cp);

	    foputs (PATH_TO_UNIX (l_path));

	    if (t_flag)
	        WhenceType (l_path);
	}

/* If not found, check for inbuilt version */

	else if (!p_flag && (IsCommandBuiltIn (cp, &inb)))
	    WhenceLocation (v_flag, cp, ShellInternalCommand);

	else if (!p_flag && LookUpSymbol (cp))
	    WhenceLocation (v_flag, cp, "is a shell keyword");

	else if (v_flag)
	{
	    PrintWarningMessage (LIT_2Strings, cp, NotFound);
	    continue;
	}

	fputchar (CHAR_NEW_LINE);
    }

    return 0;
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

static void F_LOCAL WhenceType (char *path)
{
    unsigned long type = QueryApplicationType (path);
   
    foputs ("  [");

    if (type & EXETYPE_ERROR)
	foputs (ExeType_Error [type - 1]);

    else if (type & EXETYPE_DOS)
	foputs (ExeType_Dos [(type >> 4) - 1]);

    else if (type & EXETYPE_OS2)
	printf ("OS2 %dbit %s", (type & EXETYPE_OS2_32) ? 32 : 16,
		 ExeType_OS2 [((type & EXETYPE_OS2_TYPE) >> 8) - 1]);

    else if (type & EXETYPE_NT)
	printf ("Win NT %s subsystem", ExeType_NT [(type >> 12) - 1]);

    fputchar (']');
}

/*
 * Output location
 */

static void F_LOCAL WhenceLocation (bool v_flag, char *cp, char *mes)
{
    foputs (cp);

    if (v_flag)
	printf (" %s", mes);
}

/*
 * Table of internal commands.  Note that this table is sort in alphabetic
 * order.
 */

static struct builtin	builtin[] = {
	{ "((",		dolet,		(BLT_ALWAYS | BLT_CURRENT) },
	{ ".",		dodot,		(BLT_ALWAYS | BLT_CURRENT |
					 BLT_CENVIRON) },
	{ ":",		dolabel,	(BLT_ALWAYS | BLT_CURRENT |
					 BLT_CENVIRON) },
	{ "[",		dotest,		BLT_CURRENT },
	{ LIT_Test,	dotest,		(BLT_ALWAYS | BLT_CURRENT |
					 BLT_CENVIRON | BLT_NOGLOB) },
	{ LIT_alias,	doalias,	(BLT_ALWAYS | BLT_CURRENT |
					 BLT_CENVIRON | BLT_NOGLOB |
					 BLT_NOWORDS) },
#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
	{ "bind",	dobind,		(BLT_CURRENT | BLT_CENVIRON) },
#endif
	{ LIT_break,	dobreak,	(BLT_CURRENT | BLT_CENVIRON) },
	{ LIT_builtin,	dobuiltin,	(BLT_ALWAYS | BLT_CURRENT) },
	{ "cd",		dochdir,	BLT_CURRENT },
	{ "chdir",	dochdir,	(BLT_ALWAYS | BLT_CURRENT) },
	{ LIT_continue,	docontinue,	(BLT_CURRENT | BLT_CENVIRON) },

#if (OS_TYPE != OS_DOS) 
	{ "detach",	dodetach,	(BLT_ALWAYS | BLT_CURRENT) },
#endif

	{ "echo",	doecho,		BLT_CURRENT },
	{ "eval",	doeval,		(BLT_CURRENT | BLT_CENVIRON) },
	{ LIT_exec,	(int (*)(int, char **)) doexec,
					(BLT_CURRENT | BLT_CENVIRON) },
	{ LIT_exit,	doexit,		(BLT_CURRENT | BLT_CENVIRON) },
	{ LIT_export,	doexport,	(BLT_CURRENT | BLT_CENVIRON |
					 BLT_NOGLOB | BLT_NOWORDS) },

#if (OS_TYPE == OS_OS2)
	{ "extproc",	dolabel,	(BLT_CURRENT | BLT_CENVIRON) },
#endif

	{ "false",	dofalse,	BLT_CURRENT },
	{ "fc",		dofc,		BLT_CURRENT },
	{ "functions",	dofunctions,	(BLT_ALWAYS | BLT_CURRENT) },
	{ "getopts",	dogetopts,	BLT_CURRENT },
	{ LIT_history,	dohistory,	BLT_CURRENT },

#if (OS_TYPE != OS_DOS) 
	{ "jobs",	dojobs,		BLT_CURRENT },
	{ "kill",	dokill,		BLT_CURRENT },
#endif

	{ "let",	dolet,		BLT_CURRENT },
	{ "msdos",	domsdos,	BLT_CURRENT },
	{ LIT_print,	doecho,		BLT_CURRENT },
	{ "pwd",	dopwd,		BLT_CURRENT },
	{ LIT_read,	doread,		BLT_CURRENT },
	{ "readonly",	doreadonly,	(BLT_CURRENT | BLT_CENVIRON |
					 BLT_NOGLOB | BLT_NOWORDS) },
	{ "return",	doreturn,	(BLT_CURRENT | BLT_CENVIRON) },
	{ "set",	doset,		BLT_CURRENT },
	{ "shellinfo",	doshellinfo,	BLT_CURRENT },
	{ LIT_shift,	doshift,	(BLT_CURRENT | BLT_CENVIRON) },

#if (OS_TYPE == OS_OS2)
	{ "start",	dostart,	BLT_CURRENT },
#endif

	{ "swap",	doswap,		BLT_CURRENT },
	{ "test",	dotest,		BLT_CURRENT },
#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32) && !defined (__WATCOMC__)
	{ "times",	dotimes,	BLT_CURRENT },
#endif
#if (OS_TYPE == OS_NT) || (OS_TYPE == OS_UNIX)
	{ "times",	dotimes,	BLT_CURRENT },
#endif
	{ "trap",	dotrap,		(BLT_CURRENT | BLT_CENVIRON) },
	{ "true",	dolabel,	BLT_CURRENT },
	{ LIT_type,	dowhence,	BLT_CURRENT },
	{ "typeset",	dotypeset,	(BLT_CURRENT | BLT_CENVIRON |
					 BLT_NOGLOB | BLT_NOWORDS) },
	{ "umask",	doumask,	BLT_CURRENT },
	{ LIT_unalias,	dounalias,	(BLT_ALWAYS | BLT_CURRENT) },
	{ LIT_unfunction,
			dounset,	BLT_CURRENT },
	{ "unset",	dounset,	BLT_CURRENT },
	{ "ver",	dover,		BLT_CURRENT },

#if (OS_TYPE != OS_DOS)
	{ "wait",	dowait,		BLT_CURRENT },
#endif

	{ "whence",	dowhence,	BLT_CURRENT },
	{ (char *)NULL,	(int (*)(int, char **))NULL,	0 }
};

/*
 * Look up a built in command
 */

int (*IsCommandBuiltIn (char *s, int *b))(int, char **)
{
    struct builtin	*bp;
    int			res;

    *b = 0;

/* Check for change drive */

    if ((strlen (s) == 2) && isalpha (*s) && IsDriveCharacter (*(s + 1)))
    {
	*b = BLT_ALWAYS | BLT_CURRENT;
	return dodrive;
    }

/* Search for command */

    for (bp = builtin; bp->command != (char *)NULL; bp++)
    {
	if ((res = NOCASE_COMPARE (bp->command, s)) >= 0)
	{
	    if (res != 0)
	    {
		if (!isalpha (*(bp->command)))
		    continue;

	        break;
	    }

	    *b = bp->mode;
	    return bp->fn;
	}
    }

    return (int (*)(int, char **))NULL;
}

/*
 * Builtin - either list builtins or execute it
 *
 * builtin [-asd] [ names ]
 */

static int dobuiltin (int argc, char **argv)
{
    struct builtin	*bp;
    int			(*shcom)(int, char **) = (int (*)(int, char **))NULL;
    int			ReturnValue = 0;
    char		*carg;
    int			mode;
    int			action = -1;
    int			i;

    if (argc < 2)
    {
	for (bp = builtin; bp->command != (char *)NULL; bp++)
	    printf (LIT_3Strings, LIT_builtin, bp->command,
		      (bp->mode & BLT_CURRENT) ? " - preferred" : "");
	return 0;
    }

/* Check for changing options */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((i = GetOptions (argc, argv, "sad", 0)) != EOF)
    {
	switch (i)
	{
	    case 's':	action = 0;	break;
	    case 'a':	action = 1;	break;
	    case 'd':	action = 2;	break;

	    default:
		return UsageError (BuiltinUsage);
	}
    }

/* Check to see if we know about the builtin version */

    if (action == -1)
    {
	if ((shcom = IsCommandBuiltIn (argv[1], &mode))
		   == (int (*)(int, char **))NULL)
	{
	    printf (BasicErrorMessage, argv[1], NotBuiltinCommand);
	    return 1;
	}

/* Yes - so execute the builtin version.  Set up the word list correctly */

	argv++;
	ReturnValue = (*shcom)(CountNumberArguments (argv), argv);
	argv--;
	return ReturnValue;
    }

/* Execute the requested functions against the builtin commands */

    while ((carg = argv[OptionIndex++]) != NOWORD)
    {
	for (bp = builtin;
	    (bp->command != (char *)NULL) && (NOCASE_COMPARE (bp->command, carg) != 0);
	    bp++)
	    continue;

/* Command is not builtin */

	if (bp->command == (char *)NULL)
	{
	    printf (BasicErrorMessage, carg, NotBuiltinCommand);
	    ReturnValue = 1;
	    continue;
	}

/* Update on the action */

	switch (action)
	{
	    case 0:
		printf (BasicErrorMessage, carg, (bp->mode & BLT_CURRENT)
						    ? LIT_builtin : "external");
		break;

	    case 1:
		bp->mode |= BLT_CURRENT;
		break;

	    case 2:
		if (bp->mode & BLT_ALWAYS)
		    printf (BasicErrorMessage, carg, "always builtin");

		else
		    bp->mode &= ~BLT_CURRENT;

		break;
	}
    }

    return ReturnValue;
}

/*
 * Report Usage error
 */

static int F_LOCAL	UsageError (char *string)
{
    return PrintWarningMessage ("Usage: %s", string) + 1;
}

/*
 * Alias command: alias [ -t] [ name [=commands] ]...
 */

static int doalias (int argc, char **argv)
{
    int			ReturnValue = 0;
    int			i = 1;
    bool		tracked = FALSE;
    char		*path = (char *)NULL;
    char		*cp;

/* Check for tracked aliases */

    if ((argc > 1) && (strcmp (argv[1], "-t") == 0))
    {
	++i;
	tracked = TRUE;
    }

/* List only? */

    if (argc <= i)
	return PrintAllAlias (tracked);

/* Set them up or print them */

    while (i < argc)
    {

/* Not tracked - either set alias value if there is an equals or display
 * the alias value
 */

	if (!tracked)
	{
	    if ((cp = strchr (argv[i], '=')) != (char *)NULL)
		*(cp++) = 0;

/* Check for valid name */

	    if (!IsValidAliasName (argv[i], TRUE))
		return PrintWarningMessage (LIT_Emsg, LIT_alias, NotValidAlias,
					    argv[i]);

/* Save it if appropriate */

	    if (cp != (char *)NULL)
	    {
		if (!SaveAlias (argv[i], cp, tracked))
		    ReturnValue = 1;
	    }

/* Print it */

	    else if (LookUpAlias (argv[i], FALSE) == (AliasList *)NULL)
	    {
		PrintWarningMessage (NotAnAlias, LIT_alias, argv[i]);
		ReturnValue = 1;
	    }

	    else
		PrintAlias (argv[i]);
	}

/* Set up tracked alias */

	else if (!IsValidAliasName (argv[i], TRUE))
	    return PrintWarningMessage (LIT_Emsg, LIT_alias, NotValidAlias,
					argv[i]);

	else if ((path == (char *)NULL) &&
		 ((path = AllocateMemoryCell (FFNAME_MAX + 4)) == (char *)NULL))
	    return doOutofMemory (*argv);

/* Save the new path for the alias */

	else if (SaveAlias (argv[i],
			    (FindLocationOfExecutable (path, argv[i])
				!= EXTENSION_NOT_FOUND) ? path : null, tracked))
	    ReturnValue = 1;

	++i;
    }

    return ReturnValue;
}

/*
 * UnAlias command: unalias name...
 */

static int dounalias (int argc, char **argv)
{
    int			i;

/* Set them up or print them */

    for (i = 1; i < argc; ++i)
    {
	if (LookUpAlias (argv[i], FALSE) != (AliasList *)NULL)
	    DeleteAlias (argv[i]);

	else
	    PrintWarningMessage (NotAnAlias, LIT_unalias, argv[i]);
    }

    return 0;
}

/*
 * OS2 detach function
 */

#if (OS_TYPE != OS_DOS)
static int	dodetach (int argc, char **argv)
{
    int		RetVal;

    if (argc < 2)
	return UsageError ("detach program [ parameters ... ]");

    argv++;
    RetVal = ExecuteACommand (argv, EXEC_SPAWN_DETACH);
    argv--;
    return RetVal;
}
#endif

/*
 * Start a session
 */

#if (OS_TYPE == OS_OS2)
static int dostart (int argc, char **argv)
{
    bool		Direct = FALSE;
    bool		UseCMD = FALSE;
    STARTDATA		stdata;
    STARTDATA		*sdp = &stdata;
    Word_B		*wb = (Word_B *)NULL;
    Word_B		*ep = (Word_B *)NULL;
    int			c;
    bool		options = FALSE;
    bool		SetDefault = FALSE;
    char		ChangeEnv = 0;
    bool		FirstArg = TRUE;
    bool		Display = FALSE;
    OSCALL_RET		rc;
    OSCALL_PARAM	ulSessID;
    char		*sp;
    char		p_name[FFNAME_MAX];
    char		*SetPWD = GetVariableAsString (PWDVariable, FALSE);

/* Initialise the session control info */

    memset (&stdata, 0, sizeof (STARTDATA));
    stdata.Length = sizeof (STARTDATA);
    stdata.FgBg = SSF_FGBG_BACK;	/* Set background session	*/
    stdata.PgmTitle = (PGM_TITLE_TYPE *)NULL;
    stdata.Related = SSF_RELATED_CHILD;
    stdata.TraceOpt = SSF_TRACEOPT_NONE;
    stdata.InheritOpt = SSF_INHERTOPT_SHELL;
    stdata.IconFile = (char *)NULL;
    stdata.PgmHandle = 0L;
    stdata.PgmControl = SSF_CONTROL_NOAUTOCLOSE;
    stdata.InitXPos = 0;
    stdata.InitYPos = 0;
    stdata.InitXSize = 100;
    stdata.InitYSize = 100;

/* These values get reset somewhere along the line */

#  if (OS_SIZE == OS_16)
    stdata.SessionType = SSF_TYPE_WINDOWABLEVIO;
#  else
    stdata.SessionType = SSF_TYPE_DEFAULT;
#  endif

    stdata.Environment = (PBYTE)1;	/* Build the environment */

/* Switch on the arguments */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((c = GetOptions (argc, argv,
			    SetDefault ? "t:hHfWPFibIDxe:c:"
				       : "O:St:dhHfWPFxibCIe:c:A:X:", 0)) != EOF)
    {
	switch (c)
	{
	    case 'O':
		if (!FirstArg)
		    return UsageError (StartUsage);
		
		else if (!stricmp (OptionArgument, LIT_dos))
		    sdp = &DOS_SessionControlBlock;

		else if (!stricmp (OptionArgument, "pm"))
		    sdp = &PM_SessionControlBlock;
		
		else
		    return UsageError (StartUsage);
		
		SetDefault = TRUE;
		break;

	    case 'A':
	        if ((options) || (OptionIndex != argc))
		    return UsageError (StartUsage);

		 ulSessID = (OSCALL_PARAM) strtoul (OptionArgument, &sp, 0);

		 if (*sp)
		    return UsageError (StartUsage);

#  if (OS_SIZE == OS_16)
		 if ((rc = DosSelectSession (ulSessID, 0L)))
#  else
		 if ((rc = DosSelectSession (ulSessID)))
#  endif
		    return PrintWarningMessage (Start_NoSession, ulSessID,
					        GetOSSystemErrorMessage (rc));
		 return 0;

	    case 'c':		/* Set program control		*/
	    {
		sdp->PgmControl = 0;
		sp = OptionArgument;

		while (*sp)
		{
		    switch (*(sp++))
		    {
			case 'v':
			    sdp->PgmControl |= SSF_CONTROL_VISIBLE;
			    sdp->PgmControl &= ~SSF_CONTROL_INVISIBLE;
			    break;

			case 'i':
			    sdp->PgmControl |= SSF_CONTROL_INVISIBLE;
			    sdp->PgmControl &= ~SSF_CONTROL_VISIBLE;
			    break;

			case 'l':
			    sdp->PgmControl |= SSF_CONTROL_MAXIMIZE;
			    sdp->PgmControl &= ~SSF_CONTROL_MINIMIZE;
			    break;

			case 's':
			    sdp->PgmControl |= SSF_CONTROL_MINIMIZE;
			    sdp->PgmControl &= ~SSF_CONTROL_MAXIMIZE;
			    break;

			case 'n':
			    sdp->PgmControl |= SSF_CONTROL_NOAUTOCLOSE;
			    break;

			case 'a':
			    sdp->PgmControl &= ~SSF_CONTROL_NOAUTOCLOSE;
			    break;

			default:
			    return PrintWarningMessage
				("start: option must be one of [vilsna]") + 1;
		    }
		}

		break;
	    }

	    case 'X':			/* Set startup directory	*/
		SetPWD = OptionArgument;
		break;

	    case 't':			/* Set title			*/
		if (SetDefault && (sdp->PgmTitle != (PGM_TITLE_TYPE *)NULL))
		    ReleaseMemoryCell (sdp->PgmTitle);

		sdp->PgmTitle = (!strlen (OptionArgument))
				    ? (char *)NULL
				    : SetDefault
					? StringSave (OptionArgument)
				        : OptionArgument;

		if (strlen (OptionArgument) > 32)
		    sdp->PgmTitle[33] = 0;

		break;

	    case 'S':			/* Script			*/
		Direct = FALSE;
		break;

	    case 'd':			/* Direct 			*/
		Direct = TRUE;
		break;

	    case 'H':			/* Inherit options		*/
		sdp->InheritOpt = SSF_INHERTOPT_SHELL;
		break;

	    case 'h':			/* Inherit options		*/
		sdp->InheritOpt = SSF_INHERTOPT_PARENT;
		break;

	    case 'f':			/* Foreground			*/
		sdp->FgBg = SSF_FGBG_FORE;
		break;

	    case 'b':			/* Background			*/
		sdp->FgBg = SSF_FGBG_BACK;
		break;

    	    case 'I':
		sdp->Related = SSF_RELATED_INDEPENDENT;
		break;

    	    case 'x':
		sdp->Related = SSF_RELATED_CHILD;
		break;

	    case 'W':			/* PM Window			*/
		sdp->FgBg = SSF_FGBG_FORE;
	        sdp->SessionType = (sdp == &DOS_SessionControlBlock)
					? SSF_TYPE_WINDOWABLEVIO
					: SSF_TYPE_WINDOWEDVDM;
		break;

	    case 'P':			/* PM Session			*/
		sdp->FgBg = SSF_FGBG_FORE;
	        sdp->SessionType = (sdp == &DOS_SessionControlBlock)
					? SSF_TYPE_PM
					: SSF_TYPE_WINDOWEDVDM;
		break;

	    case 'F':			/* Full Screen			*/
		sdp->FgBg = SSF_FGBG_FORE;
	        sdp->SessionType = (sdp == &DOS_SessionControlBlock)
					? SSF_TYPE_FULLSCREEN
					: SSF_TYPE_VDM;
		break;

	    case 'C':			/* Use the CMD processor	*/
		UseCMD = TRUE;
		break;

	    case 'D':			/* Show options			*/
		Display = TRUE;
		break;

	    case 'e':			/* Define environment		*/
		if (ChangeEnv == 'i')
		    return UsageError (StartUsage);

		if ((sdp->Environment != (PBYTE)NULL) &&
		    (sdp->Environment != (PBYTE)1))
		{
		    ReleaseMemoryCell (sdp->Environment);
		    sdp->Environment = (PBYTE)NULL;
		}

		ChangeEnv = 'e';
		ep = AddWordToBlock (OptionArgument, ep);
		break;

	    case 'i':			/* Inherit environment		*/
		if (ChangeEnv == 'e')
		    return UsageError (StartUsage);

		if ((sdp->Environment != (PBYTE)NULL) &&
		    (sdp->Environment != (PBYTE)1))
		    ReleaseMemoryCell (sdp->Environment);

		ChangeEnv = 'i';
		sdp->Environment = (PBYTE)NULL;
		break;

	    default:
		return UsageError (StartUsage);
	}

	FirstArg = FALSE;
	options = TRUE;
    }

/* If setting default, no more parameters allowed */

    if (SetDefault)
    {
	if (OptionIndex != argc) 
	    return UsageError (StartUsage);
    }

    else
    {

/* Check for script */

	if ((OptionIndex != argc) && (!UseCMD) && (!Direct) &&
	    (FindLocationOfExecutable (p_name, argv[OptionIndex])
				     == EXTENSION_EXECUTABLE))
	    Direct = TRUE;

/* Find the program to start */

	if ((OptionIndex == argc) || (!Direct))
	{
	    if (!UseCMD)
	    {
		wb = AddWordToBlock (GetVariableAsString (ShellVariableName,
							  FALSE), wb);
		if (SetPWD != null)
		{
		    wb = AddWordToBlock ("-X", wb);
		    wb = AddWordToBlock (SetPWD, wb);
		}
	    }

	    else
	    {
		wb = AddWordToBlock (GetVariableAsString (ComspecVariable,
							  FALSE), wb);
		if (OptionIndex != argc)
		    wb = AddWordToBlock ("/c", wb);
	    }
	}

	else
	    wb = AddWordToBlock (argv[OptionIndex++], wb);
    }

/* Set up environment */

    if ((ep != (Word_B *)NULL) &&
	((sdp->Environment =
	    BuildOS2String (GetWordList (AddWordToBlock (NOWORD, ep)), 0))
			    == (PBYTE)NULL))
	return doOutofMemory ("start");

/* OK, if we set the default, processing is completed */

    if (SetDefault)
    {
	if (ep != (Word_B *)NULL)
	    SetMemoryAreaNumber (sdp->Environment, 0);

	if (Display)
	   DisplayStartData (sdp);

	return 0;
    }

/* Set up the session block and execute the command */

    SessionControlBlock = &stdata;

/* Build the argument block */

    while (OptionIndex != argc)
	wb = AddWordToBlock (argv[OptionIndex++], wb);

/* Start the session */

    argv = GetWordList (AddWordToBlock (NOWORD, wb));

    return (ExecuteACommand (argv, 0) == -1) ? 1 : 0;
}

/*
 * Clean up the start data structure, removing allocated space
 */

static void F_LOCAL DisplayStartData (STARTDATA *sdp)
{
    char	*sp;

    printf ("Start session defaults for %s mode\n",
	    (sdp == &DOS_SessionControlBlock) ? LIT_dos : "PM");

    printf ("    Window mode: %s%s%s %sautoclose.\n",
	    (sdp->PgmControl & SSF_CONTROL_INVISIBLE)
		? "invisible" : "visible",
	    (sdp->PgmControl & SSF_CONTROL_MINIMIZE)
		? " minimised" : null,
	    (sdp->PgmControl & SSF_CONTROL_MAXIMIZE)
		? " maximised" : null,
	    (sdp->PgmControl & SSF_CONTROL_NOAUTOCLOSE)
		? "no " : "");

    if (sdp->PgmTitle != (PGM_TITLE_TYPE *)NULL)
	printf ("    Program Title: %s\n", sdp->PgmTitle);

    printf ("    Run in %s.\n", (sdp->FgBg == SSF_FGBG_FORE)
				    ? "foreground" : "background");

    printf ("    %s session.\n",
	    (sdp->Related == SSF_RELATED_INDEPENDENT)
		? "Independent" : "Dependent");

    printf ("    Session type: %s.\n", 
	    (sdp->SessionType == SSF_TYPE_WINDOWABLEVIO)
	    ? "Windowed"
	    : (sdp->SessionType == SSF_TYPE_PM)
		? "PM"
		: (sdp->SessionType == SSF_TYPE_FULLSCREEN)
		    ? "Full screen"
		    : (sdp->SessionType == SSF_TYPE_WINDOWEDVDM)
			? "DOS Windowed"
			: (sdp->SessionType == SSF_TYPE_VDM)
			    ? "DOS Full screen"
			    : "Default");

    printf ("    Inherit %s environment\n", 
	    (sdp->InheritOpt == SSF_INHERTOPT_SHELL) ? "start up" : "current");

    if (sdp->Environment == (PBYTE)1)
	puts ("    Use current environment variables.");

    else if (sdp->Environment == (PBYTE)NULL)
	puts ("    Use start up environment variables.");

    else
    {
	sp = sdp->Environment;
	printf ("    Environment variables:\n");

	while (*sp)
	{
	    printf ("        %s\n", sp);
	    sp += strlen (sp) + 1;
	}
    }
}
#endif


/*
 * Set up new Parameter Variables
 */

static int F_LOCAL SetUpNewParameterVariables (char **Array,/* New values*/
					       int  Offset, /* Start offset */
					       int  Max, /* Number	*/
					       char *function)
{
    int		n;
    Word_B	*wb = (Word_B *)NULL;
    char	*cp;
    bool	Duplicate = (bool)(strcmp (function, LIT_shift) == 0);

    if ((wb = AddParameter (ParameterArray[0], wb, function)) == (Word_B *)NULL)
	return 1;

    for (n = Offset; n < Max; ++n)
    {
	if ((cp = (Duplicate) ? StringSave (Array[n]) : Array[n])
		== null)
	    return doOutofMemory (function);

	if ((wb = AddParameter (cp, wb, function)) == (Word_B *)NULL)
	    return 1;
    }

    return (AddParameter (NOWORD, wb, function) == (Word_B *)NULL)
		? 1 : 0;
}

/*
 * dolet - Each arg is an arithmetic expression to be evaluated.
 */

static int dolet (int argc, char **argv)
{
    long	Value = 0;
    int		i;

/* If (( )), ignore the terminating )) */

    if ((strcmp (argv[0], "((") == 0) &&
	(strcmp (argv[argc - 1], "))") == 0))
	argv[argc - 1] = NOWORD;

    ExpansionErrorDetected = FALSE;

    for (i = 1; !ExpansionErrorDetected && (argv[i] != NOWORD); ++i)
	Value = EvaluateMathsExpression (argv[i]);

    return !Value || ExpansionErrorDetected ? 1 : 0;
}

/*
 * Out of memory error
 */

static int F_LOCAL doOutofMemory (char *s)
{
    return PrintWarningMessage (BasicErrorMessage, s, Outofmemory1);
}

/*
 * MSDOS, EXPORT and READONLY functions: xxxx [ variable names... ]
 */

static int doexport (int argc, char **argv)
{
    return UpdateVariableStatus (argv + 1, STATUS_EXPORT);
}

static int doreadonly (int argc, char **argv)
{
    return UpdateVariableStatus (argv + 1, STATUS_READONLY);
}

static int domsdos (int argc, char **argv)
{
    return UpdateVariableStatus (argv + 1, STATUS_CONVERT_MSDOS);
}

static int F_LOCAL UpdateVariableStatus (char **argv, unsigned int Mask)
{
    if (*argv == NOWORD)
        return ListAllVariables (Mask, FALSE);

    else
    {
	memset (&TypesetValues, 0, sizeof (TypesetValues));
	TypesetValues.Flags_On = Mask;
	return TypesetVariables (argv);
    }
}

/*
 * List All variables matching a STATUS
 */

static int F_LOCAL ListAllVariables (unsigned int Mask, bool PrintValue)
{
    HandleSECONDandRANDOM ();

    DVE_Mask = Mask;
    DVE_PrintValue = PrintValue;

    twalk (VariableTree, DisplayVariableEntry);
    return 0;
}

/*
 * TWALK Function - display VARIABLE tree
 */

static void DisplayVariableEntry (const void *key, VISIT visit, int level)
{
    VariableList	*vp = *(VariableList **)key;

    if ((visit == postorder) || (visit == leaf))
    {
	if ((IS_VariableFC ((int)*vp->name)) &&
	    ((vp->status & DVE_Mask) ||
             (((vp->status & ~STATUS_GLOBAL) == 0) && (DVE_Mask == 0xffff))))
	    PrintEntry (vp, DVE_PrintValue,
			(DVE_Mask == 0xffff) ? 0 : DVE_Mask);
    }
}

/*
 * typeset function - [ [ [+-][Hflprtux] ] [+-][LRZi[n]] [ name [=value] ...]
 */

static int dotypeset (int argc, char **argv)
{
    int			ReturnValue = 0;
    bool		f_flag = FALSE;
    unsigned int	*CurrentFlags;
    int			tmp = 0;
    char		c_opt;
    char		*cp;

/* Initialise save area */

    memset (&TypesetValues, 0, sizeof (TypesetValues));
    OptionIndex = 1;

/* Scan the options */

    while ((cp = argv[OptionIndex]) != NOWORD)
    {
	if ((*cp != '-') && (*cp != '+'))
	    break;

	CurrentFlags = (*cp == '-') ? &TypesetValues.Flags_On
				    : &TypesetValues.Flags_Off;

	while (*(++cp))
	{
	    switch (*cp)
	    {
		case 'p':
		    fprintf (stderr, "typeset: Option (%c) not supported\n",
			     *cp);
		    break;

		case 'H':
		    *CurrentFlags |= STATUS_CONVERT_MSDOS;
		    break;

		case 'f':		/* Function only		*/
		    f_flag = TRUE;
		    break;

		case 'l':
		    *CurrentFlags |= STATUS_LOWER_CASE;
		    break;

		case 'r':
		    *CurrentFlags |= STATUS_READONLY;
		    break;

		case 't':
		    *CurrentFlags |= STATUS_TAGGED;
		    break;

		case 'u':
		    *CurrentFlags |= STATUS_UPPER_CASE;
		    break;

		case 'x':
		    *CurrentFlags |= STATUS_EXPORT;
		    break;

		case 'L':
		case 'R':
		case 'Z':
		case 'i':
		{
		    switch (c_opt = *cp)
		    {
			case 'L':
			    *CurrentFlags |= STATUS_LEFT_JUSTIFY;
			    break;

			case 'R':
			    *CurrentFlags |= STATUS_RIGHT_JUSTIFY;
			    break;

			case 'Z':
			    *CurrentFlags |= STATUS_ZERO_FILL;
			    break;

			case 'i':
			    *CurrentFlags |= STATUS_INTEGER;
			    break;
		    }

/* Only set width on on */

		    if (CurrentFlags != &TypesetValues.Flags_On)
			break;

/* Check for following numeric */

		    if (isdigit (*(cp + 1)))
			tmp = (int)strtol (cp + 1, &cp, 10);

		    else if ((*(cp + 1) == 0) &&
			     (OptionIndex + 1 < argc) &&
			     isdigit (*argv[OptionIndex + 1]))
			tmp = (int)strtol (argv[++OptionIndex], &cp, 10);

		    else
			break;

/* Check for invalid numeric */

		    if (!tmp || *(cp--))
			return UsageError (TypeSetUsage);

/* Width or base */

		    if (c_opt == 'i')
			TypesetValues.Base = tmp;

		    else
			TypesetValues.Width = tmp;

		    break;
		}

		default:
		    return UsageError (TypeSetUsage);
	    }
	}

	++OptionIndex;
    }

/* Check for f flag - function processing. */

    if (f_flag)
    {
	if (((TypesetValues.Flags_On | TypesetValues.Flags_Off) &
		~(STATUS_TAGGED | STATUS_EXPORT)) ||
	    (!(TypesetValues.Flags_On | TypesetValues.Flags_Off)))
	    return PrintWarningMessage ("typeset: Only -xt allowed with -f");

	for (tmp = OptionIndex; tmp < argc; tmp++)
	    ReturnValue |= HandleFunction (argv[tmp]);

	return ReturnValue;
    }

/* Process variable assignments */

    return TypesetVariables (&argv[OptionIndex]);
}

static int F_LOCAL TypesetVariables (char **argv)
{
    bool		PrintValue = C2bool (TypesetValues.Flags_Off);
    VariableList	*vp;
    char		*CurrentName;
    char		*NewValue;
    char		*OldValue;
    int			OldStatus;
    long		NValue;
    char		*xp;
    int			Retval = 0;
    unsigned int	Mask;
    long		Index;

    if ((Mask = (TypesetValues.Flags_On | TypesetValues.Flags_Off)) == 0)
	Mask = 0xffff;

/* Switch off any appropriate flags */

    if (TypesetValues.Flags_On & STATUS_LOWER_CASE)
	TypesetValues.Flags_Off |= STATUS_UPPER_CASE;

    if (TypesetValues.Flags_On & STATUS_UPPER_CASE)
	TypesetValues.Flags_Off |= STATUS_LOWER_CASE;

    if (TypesetValues.Flags_On & STATUS_RIGHT_JUSTIFY)
	TypesetValues.Flags_Off |= STATUS_LEFT_JUSTIFY;

    if (TypesetValues.Flags_On & STATUS_LEFT_JUSTIFY)
	TypesetValues.Flags_Off |= STATUS_RIGHT_JUSTIFY;

/* If no arguments, print all values matching the mask */

    if (*argv == NOWORD)
	return ListAllVariables (Mask, PrintValue);

/* Process each argument.  If no flags, print it */

    while ((CurrentName = *(argv++)) != NOWORD)
    {
	if (!GetVariableName (CurrentName, &Index, &NewValue, (bool *)NULL))
	{
	    PrintErrorMessage (BasicErrorMessage, CurrentName, LIT_BadID);
	    return 1;
	}

/* Convert the = to a null so we get the name and value */

	if (*NewValue)
	    *(NewValue++) = 0;

	else
	    NewValue = (char *)NULL;

/* If valid - update, otherwise print a message */

	if ((Mask != 0xffff) || (NewValue != (char *)NULL))
	{

/* Get the original value */

	    vp = LookUpVariable (CurrentName, (int)Index, TRUE);
	    OldStatus = vp->status;
	    OldValue = GetVariableArrayAsString (CurrentName, (int)Index,
						 FALSE);

/* Update status */

	    vp->status &= ~(TypesetValues.Flags_Off);
	    vp->status |= TypesetValues.Flags_On;

	    if ((CurrentFunction != (FunctionList *)NULL) &&
		(!(vp->status & STATUS_GLOBAL)))
		vp->status |= STATUS_LOCAL;

	    if (Index)
		vp->status &= ~(STATUS_EXPORT);

/* Set up a new integer value.  If the variable was not an integer
 * originally and there is an error, unset it
 */

	    xp = (NewValue != (char *)NULL) ? NewValue : OldValue;

	    if (vp->status & STATUS_INTEGER)
	    {
		if (ValidMathsExpression (xp, &NValue))
		{
		    Retval = PrintWarningMessage (LIT_Emsg, "bad numeric value",
						  CurrentName, xp);

		    if (!(OldStatus & STATUS_INTEGER))
			UnSetVariable (CurrentName, (int)Index, FALSE);

		    continue;
		}

		else if (OldStatus & STATUS_READONLY)
		{
		    Retval = PrintWarningMessage (LIT_2Strings, vp->name,
						  LIT_IsReadonly);
		    continue;
		}

/* Save the new integer value and set up base etc */

		vp->nvalue = NValue;

		if (!vp->base)
		    vp->base = (LastNumberBase != -1) ? LastNumberBase : 10;

		if (TypesetValues.Base)
		    vp->base = TypesetValues.Base;

		if (vp->value != null)
		    ReleaseMemoryCell ((void *)vp->value);

		vp->value = null;
	    }

/* String - update if appropriate, both the value and the width */

	    else if ((OldStatus & STATUS_READONLY) ||
		     (!(vp->status & STATUS_READONLY)))
		SetVariableArrayFromString (CurrentName, (int)Index, xp);

/* New status is readonly - allow set, then stop them */

	    else
	    {
		vp->status &= ~STATUS_READONLY;
		SetVariableArrayFromString (CurrentName, (int)Index, xp);
		vp->status |= STATUS_READONLY;
	    }

	    if (TypesetValues.Width)
		vp->width = TypesetValues.Width;
	}

/* Print if appropriate */

	else
	    PrintEntry (LookUpVariable (CurrentName, (int)Index, FALSE),
			PrintValue, Mask);
    }

    return Retval;
}

static void F_LOCAL PrintEntry (VariableList	*vp,
			        bool		PrintValue,
			        unsigned int	Mask)
{
    unsigned int	Flags = vp->status & Mask;

    if (vp->status & STATUS_NOEXISTANT)
	return;

    if (Flags & STATUS_INTEGER)
	printf ("integer ");

    if (Flags & STATUS_LEFT_JUSTIFY)
	printf ("left justified %d ", vp->width);

    if (Flags & STATUS_RIGHT_JUSTIFY)
	printf ("right justified %d ", vp->width);

    if (Flags & STATUS_ZERO_FILL)
	printf ("zero filled %d ", vp->width);

    if (Flags & STATUS_CONVERT_MSDOS)
	printf ("MS-DOS Format ");

    if (Flags & STATUS_LOWER_CASE)
	printf ("lowercase ");

    if (Flags & STATUS_UPPER_CASE)
	printf ("uppercase ");

    if (Flags & STATUS_READONLY)
	printf ("readonly ");

    if (Flags & STATUS_TAGGED)
	printf ("tagged ");

    if (Flags & STATUS_EXPORT)
	printf ("exported ");

/* Print the value */

    foputs (vp->name);

    if (vp->index || CountVariableArraySize (vp->name) > 1)
	printf (LIT_BNumber, vp->index);

    if (PrintValue)
	printf ("=%s", GetVariableArrayAsString (vp->name, vp->index, TRUE));

    fputchar (CHAR_NEW_LINE);
}

/*
 * Handle typeset -f
 */

static int F_LOCAL HandleFunction (char *name)
{
    FunctionList	*fop;

    if (strchr (name, CHAR_ASSIGN) != (char *)NULL)
	return PrintWarningMessage ("typeset: cannot assign to functions");

    if ((fop = LookUpFunction (name, FALSE)) == (FunctionList *)NULL)
	return PrintWarningMessage ("typeset: function %s does not exist",
				    name);

    fop->Traced = C2bool (TypesetValues.Flags_On & STATUS_TAGGED);
    return 0;
}

/*
 * Modified version of getopt for shell
 */

void	ResetGetOptions (void)
{
    OptionIndex = 1;			/* Reset the optind flag	*/
    GetOptionPosition = 1;		/* Current position	*/
}

int	GetOptions (int		argc,		/* Argument count	*/
		    char	**argv,		/* Argument array	*/
		    char	*optstring,	/* Options string	*/
		    int		flags)		/* Control flags	*/
{
    int		cur_option;		/* Current option		*/
    char	*cp;			/* Character pointer		*/

    BadOptionValue = 0;

    if (GetOptionPosition == 1)
    {

/* Special for doecho */

	if (flags & GETOPT_PRINT)
	    return EOF;

/* Check for out of range, correct start character and not single */

	if ((OptionIndex >= argc) ||
	    (!(((OptionStart = *argv[OptionIndex]) == '-') ||
	       (((flags & GETOPT_PLUS) && (OptionStart == '+'))))) ||
	    (!argv[OptionIndex][1]))
	    return EOF;

	if (!strcmp (argv[OptionIndex], DoubleHypen))
	    return EOF;
    }

/* Get the current character from the current argument vector */

    cur_option = argv[OptionIndex][GetOptionPosition];

/* Validate it */

    if ((cur_option == ':') ||
	((cp = strchr (optstring, cur_option)) == (char *)NULL))
    {
	if (flags & GETOPT_MESSAGE)
	    PrintWarningMessage ("%s: illegal option -- %c", argv[0],
				 cur_option);

/* Save the bad option value and move to the next offset */

	BadOptionValue = cur_option;

	if (!argv[OptionIndex][++GetOptionPosition])
	{
	    OptionIndex++;
	    GetOptionPosition = 1;
	}

	return '?';
    }

/* Parameters following ? */

    OptionArgument = (char *)NULL;

    if (*(++cp) == ':')
    {
	if (argv[OptionIndex][GetOptionPosition + 1])
	    OptionArgument = &argv[OptionIndex++][GetOptionPosition + 1];

	else if (++OptionIndex >= argc)
	{
	    if ((flags & (GETOPT_MESSAGE | GETOPT_AMISSING)) == GETOPT_MESSAGE)
		PrintWarningMessage ("%s: option (%c) requires an argument",
				     argv[0], cur_option);

	    BadOptionValue = cur_option;
	    OptionArgument = (char *)-1;
	    GetOptionPosition = 1;
	    return '?';
	}

	else
	    OptionArgument = argv[OptionIndex++];

	GetOptionPosition = 1;
    }

    else if (!argv[OptionIndex][++GetOptionPosition])
    {
	GetOptionPosition = 1;
	OptionIndex++;
    }

    return cur_option;
}


/*
 * Kill the specified processes
 */

#if (OS_TYPE != OS_DOS)
static struct KillSignalList {
    char	*Name;
    int		SigVal;
} KillSignalList [] = {
    {"term",	-1 },
#  if (OS_TYPE == OS_OS2)
    {"usr1",	PFLG_A },
    {"usr2",	PFLG_B },
    {"usr3",	PFLG_C },
#  elif (OS_TYPE == OS_NT)
    {"break",	CTRL_BREAK_EVENT},
    {"int",	CTRL_C_EVENT},
#  endif
};

#define MAX_KILL_SIGNALS	ARRAY_SIZE (KillSignalList)

static int dokill (int argc, char **argv)
{
    int		i;
    int		Sigv = -1;
    char	*cp;
    PID		pidProcess;
    long	value;
#  if (OS_TYPE == OS_NT)
    HANDLE	hp;
#  elif (OS_TYPE == OS_OS2)
    USHORT	rc;
#  endif

    if (argc < 2)
	return UsageError (KillUsage);

/* List signals ? */

    if (!strcmp (argv[1], "-l"))
    {
	for (i = 0; i < MAX_KILL_SIGNALS; ++i)
	    puts (KillSignalList[i].Name);

	return 0;
    }

/* Look up signal name */

    if (**(++argv) == '-')
    {
	cp = &argv[0][1];

	for (i = 0; i < MAX_KILL_SIGNALS; ++i)
	{
	    if (!stricmp (KillSignalList[i].Name, cp))
		break;
	}

	if (i == MAX_KILL_SIGNALS)
	    return PrintWarningMessage ("kill: bad signal name (%s)", cp);

	Sigv = KillSignalList[i].SigVal;

	if (*(++argv) == NOWORD)
	    return UsageError (KillUsage);
    }

/* Kill the processes */

    while (*argv != NOWORD)
    {

/* Find the PID */

        if (((**argv == CHAR_JOBID) && !ConvertJobToPid ((*argv) + 1, &pidProcess)) ||
	    ((**argv != CHAR_JOBID) && !ConvertNumericValue (*argv, &value, 0)))
	    return PrintWarningMessage ("kill: bad process/job id (%s)",
					 *argv);

/* If Process ID, its in value */

 	if (**argv != CHAR_JOBID)
	    pidProcess = (PID)value;

/* Send the signal */

#  if (OS_TYPE == OS_OS2)
	if (Sigv == -1)
	{
#    if (OS_SIZE == OS_16)
	    rc = DosKillProcess (DKP_PROCESSTREE, pidProcess);
#    else
	    if ((rc = DosKillProcess (DKP_PROCESSTREE, pidProcess)))
		 rc = (USHORT)DosSendSignalException (pidProcess,
						      XCPT_SIGNAL_BREAK);
#    endif
	}

	else
	    rc = Dos32FlagProcess (pidProcess, FLGP_SUBTREE, Sigv, 0);

	if (rc)
	    return PrintWarningMessage ("kill: Cannot signal process %s\n%s",
					 *argv,
					 GetOSSystemErrorMessage (rc));
#  elif (OS_TYPE == OS_NT)

/* Check for Ctl C or Break */

	if ((Sigv == CTRL_BREAK_EVENT) || (Sigv == CTRL_C_EVENT))
	{
	    if (!GenerateConsoleCtrlEvent (Sigv, pidProcess))
		return PrintWarningMessage ("kill: Cannot kill process %s\n%s",
					    *argv, 
				     GetOSSystemErrorMessage (GetLastError ()));

	}

/* Open the process and terminate it */

	else if ((hp = OpenProcess (PROCESS_ALL_ACCESS, TRUE,
				    pidProcess)) == NULL)
	    return PrintWarningMessage ("kill: Cannot access process %s\n%s",
					*argv, 
				    GetOSSystemErrorMessage (GetLastError ()));
	else if (!TerminateProcess (hp, 1))
	{
	    PrintWarningMessage ("kill: Cannot kill process %s\n%s", *argv, 
				 GetOSSystemErrorMessage (GetLastError ()));
	    CloseHandle (hp);
	    return 1;
	}

	CloseHandle (hp);
#  endif

	argv++;
    }

    return 0;
}

/*
 * Wait for process to end
 */

static int dowait (int argc, char **argv)
{
    PID		pidProcess;
    int		TermStat;
#  if (OS_TYPE != OS_NT)
    int		ReturnValue;
#  endif
    long	value;

/* Check usage */

    if (argc > 2)
	return UsageError ("wait [ job ]");

/* Wait for all jobs ? */

    if (argc < 2)
    {
#  if (OS_TYPE == OS_NT)
	return PrintWarningMessage ("wait: any job not supported on NT");
#  else
	TermStat = -1;

/* Yes - wait until wait returns an error */

	while ((pidProcess = wait (&ReturnValue)) != -1)
	{
	    DeleteJob (pidProcess);
	    TermStat = ReturnValue;
	}
#  endif
    }

/* Wait for a specific process.  Job or PID? */

    else
    {

/* Move to the ID */

	argv++;

/* Find the PID */

        if (((**argv == CHAR_JOBID) &&
	     !ConvertJobToPid ((*argv) + 1, &pidProcess)) ||
	    ((**argv != CHAR_JOBID) &&
	     !ConvertNumericValue (*argv, &value, 0)))
	    return PrintWarningMessage ("wait: bad process/job id (%s)",
					 *argv);

/* If Process ID, its in value */

 	if (**argv != CHAR_JOBID)
	    pidProcess = (PID)value;

/* Wait for the specified process */

#  if (OS_TYPE == OS_UNIX)
	fputs ("UNIX: Wait for process NI\n", stderr);

#  else
	if (cwait (&TermStat, pidProcess, WAIT_GRANDCHILD) == -1)
	    return PrintWarningMessage ("wait: Process id (%s) not active",
					*argv);
#  endif

	DeleteJob (pidProcess);
    }

/* Convert termination status to return code */

    if (TermStat == -1)
	return -1;

    else if (TermStat & 0x00ff)
	return TermStat & 0x00ff;

    else
	return (TermStat >> 8) & 0x00ff;
}

/*
 * Print the job info
 */

static int dojobs (int argc, char **argv)
{
    bool	ListMode = FALSE;
    bool	ListTree = FALSE;
    pid_t	p = getpid ();
#  if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_UNIX)
    long	value;
#  endif
    int		i;

/* List signals ? */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((i = GetOptions (argc, argv, "plP:", 0)) != EOF)
    {
        switch (i)
        {
	    case 'l':
		ListMode = TRUE;
		break;

#  if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_UNIX)
	    case 'p':
		ListTree = TRUE;
		break;

	    case 'P':
		ListTree = TRUE;

		if (!ConvertNumericValue (OptionArgument, &value, 0))
		    return PrintWarningMessage ("jobs: bad process id (%s)",
		    				OptionArgument);

		p = (pid_t)value;

		break;
#  endif

	    default:
		return UsageError (JobUsage);
	}
    }

#  if (OS_TYPE == OS_OS2) || (OS_TYPE == OS_UNIX)
    if (ListTree)
        return PrintProcessTree (p);
#  endif

/* Look up job name */

    return PrintJobs (ListMode);
}
#endif

/*
 * Print the option settings
 */

static int F_LOCAL	PrintOptionSettings (void)
{
    int		i;

    puts ("Current option settings:");

    for (i = 0; SetOptions[i].OptionName != (char *)NULL; i++)
	printf ("%-16s%s\n", SetOptions[i].OptionName,
		TestOptionValue (SetOptions[i].OptionName, FALSE)
			? "on" : "off");
    return 0;
}

/*
 * Change option value
 */

static bool F_LOCAL ChangeOptionValue (char *value, bool set)
{
    struct SetOptions	*entry = LookUpOptionValue (value);

    if (entry == (struct SetOptions *)NULL)
	return FALSE;

    else if (entry->HasOptionValue)
	SetClearFlag (entry->FlagValue, set);

/* If one of the Editor flags, disable all editor flags first */

    else if (entry->FlagValue == FLAGS_VERIFY_SWITCH)
	SetVerifyStatus (set);

#ifdef FLAGS_BREAK_SWITCH
    else if (entry->FlagValue == FLAGS_BREAK_SWITCH)
	SetBreakStatus (set);
#endif

#ifdef FLAGS_SET_OS2
    else if (entry->FlagValue == FLAGS_SET_OS2)
        BaseOS = (set) ? BASE_OS_OS2 : BASE_OS_DOS;
#endif

#ifdef FLAGS_SET_NT
    else if (entry->FlagValue == FLAGS_SET_NT)
        BaseOS = (set) ? BASE_OS_NT : BASE_OS_DOS;
#endif

/* Change the editor flags */

    else if (set)
    {
	if (entry->FlagValue & FLAGS_EDITORS)
	    ShellGlobalFlags &= ~(FLAGS_EDITORS);

	ShellGlobalFlags |= entry->FlagValue;
    }

    else
	ShellGlobalFlags &= ~(entry->FlagValue);


    return TRUE;
}

/*
 * Update shell switches
 */

static void F_LOCAL SetClearFlag (int Flag, bool set)
{
    if (set)
	FL_SET (Flag);

    else
	FL_CLEAR (Flag);
}

/*
 * Test an option
 */

static int F_LOCAL TestOptionValue (char *value, bool AllowJump)
{
    struct SetOptions	*entry = LookUpOptionValue (value);

    if (entry == (struct SetOptions *)NULL)
    {
	PrintWarningMessage ("%s: unknown option - %s", TestProgram, value);

	if (AllowJump)
	    longjmp (TestErrorReturn, 1);
	
	return 0;
    }

    else if (entry->FlagValue == FLAGS_VERIFY_SWITCH)
    {
#if (OS_TYPE == OS_OS2)
	BOOL	fVerifyOn;

	DosQVerify (&fVerifyOn);
	return fVerifyOn;

#elif (OS_TYPE != OS_DOS)

	return FALSE;

#elif (OS_TYPE == OS_DOS)

	union REGS	r;

	r.x.REG_AX = 0x5400;
	DosInterrupt (&r, &r);
	return r.h.al;
#endif
    }

#if (OS_TYPE == OS_DOS)
    else if (entry->FlagValue == FLAGS_BREAK_SWITCH)
    {
	union REGS	r;

	r.x.REG_AX = 0x3300;
	DosInterrupt (&r, &r);
	return r.h.dl;
    }
#endif

#ifdef FLAGS_SET_OS2
    else if (entry->FlagValue == FLAGS_SET_OS2)
        return BaseOS == BASE_OS_OS2;
#endif

#ifdef FLAGS_SET_NT
    else if (entry->FlagValue == FLAGS_SET_NT)
        return BaseOS == BASE_OS_NT;
#endif

    else if (entry->HasOptionValue)
	return (FL_TEST (entry->FlagValue) != 0);

    return (ShellGlobalFlags & entry->FlagValue);
}

/*
 * Find an Option entry
 */

static struct SetOptions * F_LOCAL LookUpOptionValue (char *value)
{
    int		i = 0;
    char	*cp;

    while ((cp = SetOptions[i].OptionName) != (char *)NULL)
    {
	if (!strcmp (cp, value))
	    return &SetOptions[i];

	++i;
    }

    return (struct SetOptions *)NULL;
}

/*
 * Get Unit number
 */

static int F_LOCAL GetUnitNumber (char *prog)
{
    int		Unit;

    if (((Unit = GetNumericValue (OptionArgument)) < 0) || (Unit > 9))
    {
	PrintWarningMessage (LIT_Emsg, LIT_bun, prog, OptionArgument);
	return -1;
    }

    return Unit;
}

/*
 * fc function - fc [-e EditorName] [-nlr] [First [Last]]
 *		 fc -e - [Old=New] [Command]
 */

static int dofc (int argc, char **argv)
{
    char		*Editor = GetVariableAsString (FCEditVariable, FALSE);
    bool		n_flag = TRUE;
    bool		l_flag = FALSE;
    bool		r_flag = FALSE;
    int			EventNumber[2];
    char		*Temp;
    char		*Change = (char *)NULL;
    char		*Change1;
    char		*NewBuffer;
    char		*NewArg[3];
    int			i;
    FILE		*fp;

/* Check status */

    if (!(InteractiveFlag && IS_TTY (0)))
	return PrintWarningMessage ("fc: only available in interactive mode");

    if ((NewBuffer = AllocateMemoryCell (LINE_MAX + 3)) == (char *)NULL)
	return doOutofMemory ("fc");

/* Process options */

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((i = GetOptions (argc, argv, "e:nlr", 0)) != EOF)
    {
	switch (i)
	{
	    case 'e':			/* editor name			*/
		Editor = OptionArgument;
		break;

	    case 'n':	n_flag = FALSE;	break;
	    case 'l':	l_flag = TRUE;	break;
	    case 'r':	r_flag = TRUE;	break;

	    default:
		return UsageError ("fc [ -e EditorName ] [ -nlr ] [ First [Last]]\n       fc -e - [ Old=New ] [ Command ]");
	}
    }

    argv += OptionIndex;
    argc -= OptionIndex;

/* Check for [old=new] */

    if (argc && ((Change1 = strchr (*argv, CHAR_ASSIGN)) != (char *)NULL))
    {
	Change = *(argv++);
	*(Change1++) = 0;
	--argc;
    }

    if (!l_flag)
	DeleteLastHistory ();

/* Get the first and last event number */

    for (i = 0; i < 2; i++)
    {
	EventNumber[i] = 0;

	if (argc)
	{
	    EventNumber[i] = (int)strtol (*argv, &Temp, 10);

	    if (*Temp)
		EventNumber[i] = SearchHistory (*argv);

	    else if (EventNumber[i] < 0)
		EventNumber[i] += GetLastHistoryEvent () - 1;

	    if (EventNumber[i] <= 0)
		return PrintWarningMessage ("fc: event <%s> not found",
					    *argv);

	    argv++;
	    --argc;
	}
    }

/* Set up first and last values */

    i = GetLastHistoryEvent () - 1;

    if (!EventNumber[0])
    {
        if ((EventNumber[0] = (l_flag) ? i - 16 : i) <= 0)
	    EventNumber[0] = 1;
    }

    if (!EventNumber[1])
        EventNumber[1] = (l_flag) ? i : EventNumber[0];

/* If l - print */

    if (l_flag)
	fp = stdout;

    else if (Editor == null)
	return PrintWarningMessage ("fc: editor not defined");

    else if ((fp = FOpenFile ((Temp = GenerateTemporaryFileName ()),
    			      sOpenWriteBinaryMode)) == (FILE *)NULL)
	return PrintWarningMessage ("fc: cannot create %s", Temp);

/* Process history */

    if (!l_flag)
	n_flag = FALSE;

    PrintHistory (r_flag, n_flag, EventNumber[0], EventNumber[1], fp);

    if (l_flag)
	return 0;

/* Check that we found some history */

    if (!ftell (fp))
	l_flag = TRUE;

    if (fp != stdout)
	CloseFile (fp);

    if (l_flag)
    {
	unlink (Temp);
	return PrintWarningMessage ("fc: no matches");
    }

/* Invoke the editor */

    if (strcmp (Editor, ShellOptionsVariable))
    {
	NewArg[0] = Editor;
	NewArg[1] = Temp;
	NewArg[2] = (char *)NULL;

	if (ExecuteACommand (NewArg, 0) == -1)
	{
	    unlink (Temp);
	    return 1;
	}
    }

/* Now execute it */

    if ((i = S_open (TRUE, Temp, O_RMASK)) < 0)
	return PrintWarningMessage ("fc: cannot re-open edit file (%s)",
				    Temp);

    argc = read (i, NewBuffer, LINE_MAX + 1);
    S_close (i, TRUE);

    if (argc <= 0)
	return (argc == 0) ? 0 : 1;

    else if (argc >= LINE_MAX - 1)
	return PrintWarningMessage (FCTooLong);

/* Strip off trailing EOFs and EOLs */

    CleanUpBuffer (argc, NewBuffer, 0x1a);

/* Check for substitution */

    if (Change == (char *)NULL)
	strcpy (ConsoleLineBuffer, NewBuffer);

    else
    {
	if ((Temp = strstr (NewBuffer, Change)) == (char *)NULL)
	    return PrintWarningMessage ("fc: string not found");

	if ((i = strlen (NewBuffer) - strlen (Change) +
		 strlen (Change1)) >= LINE_MAX - 2)
	    return PrintWarningMessage (FCTooLong);

/* Do the substitution */

	i = Temp - NewBuffer;
	strncpy (ConsoleLineBuffer, NewBuffer, i);
	strcpy (ConsoleLineBuffer + i, Change1);
	strcat (ConsoleLineBuffer, NewBuffer + strlen (Change) + i);
    }

    ReleaseMemoryCell ((void *)NewBuffer);

/* Tell the user what we've done */

    puts (ConsoleLineBuffer);

/* Flag the console driver not to read from the console, but use the
 * current contents of the ConsoleLineBuffer
 */

    UseConsoleBuffer = TRUE;
    return 0;
}

/*
 * Convert Job ID to process id
 */


#if (OS_TYPE != OS_DOS) 
static bool F_LOCAL ConvertJobToPid (char *String, PID *pid)
{
    long	value;
    JobList	*jp;

/* If numeric value, look up the job number */

    if (ConvertNumericValue (String, &value, 0))
	jp = LookUpJob ((int)value);

    else
	jp = SearchForJob (String);

    if (jp == (JobList *)NULL)
	return FALSE;

    PreviousJob = CurrentJob;
    CurrentJob = jp->pid;

    *pid = jp->pid;
    return TRUE;
}
#endif

/*
 * Missing OS2 2.x API
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32)
APIRET	 DosQFileMode (PSZ pszFName, PULONG pusAttr)
{
    APIRET	rc;
    FILESTATUS3	status;

    if ((rc = DosQueryPathInfo (pszFName, FIL_STANDARD, &status,
			       sizeof (FILESTATUS3))) == 0)
	*pusAttr = status.attrFile;

    return rc;
}
#endif

#if (OS_TYPE == OS_NT)
int	 DosQFileMode (char *pszFName, DWORD *pusAttr)
{
    *pusAttr = GetFileAttributes (pszFName);
    return (*pusAttr == 0xffffffff) ? -1 : 0;
}
#endif

/*
 * A sort of version of times
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32) && !defined (__WATCOMC__)
static int dotimes (int argc, char **argv)
{
    return PrintTimes ();
}
#endif

#if (OS_TYPE == OS_NT) || (OS_TYPE == OS_UNIX)
static int dotimes (int argc, char **argv)
{
    return PrintTimes ();
}
#endif

/*
 * Convert logical path to physical path, skipping out SUBST drives
 */

#if (OS_TYPE == OS_OS2)
static char * F_LOCAL GetPhysicalPath (char *inpath, bool AllowNetwork)
{
#  ifndef __EMX__
    char		*op;
    char		*res;

    if ((op = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	return inpath;

    if ((res = _fullpath (op, inpath, PATH_MAX + 6)) != (char *)NULL)
	strcpy (inpath, op);

    PATH_TO_LOWER_CASE (inpath);
    ReleaseMemoryCell (op);
#  endif

    return PATH_TO_UNIX (inpath);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static char * F_LOCAL GetPhysicalPath (char *inpath, bool AllowNetwork)
{
    char		*op;
    char		*res;

    if ((op = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	return inpath;

    if (!GetFullPathName (inpath, FFNAME_MAX, op, &res))
	strcpy (inpath, op);

    PATH_TO_LOWER_CASE (inpath);
    ReleaseMemoryCell (op);
    return PATH_TO_UNIX (inpath);
}
#endif

/* DOS version */

#if (OS_TYPE == OS_DOS)
static char * F_LOCAL GetPhysicalPath (char *inpath, bool AllowNetwork)
{
#  ifndef __EMX__
    char		*op;
    union REGS		r;
    struct SREGS	sr;

    if ((op = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	return inpath;

    PATH_TO_DOS (inpath);

    r.h.ah     = 0x60;
    r.x.REG_SI = FP_OFF (inpath);
    sr.ds      = FP_SEG (inpath);

    r.x.REG_DI = FP_OFF (op);
    sr.es      = FP_SEG (op);

    DosExtendedInterrupt (&r, &r, &sr);

/* If we are succesfully and this is not a networked drive, replace the
 * original path
 */

    if ((!(r.x.REG_CFLAGS & 1)) &&
        (AllowNetwork || (strncmp (op, "\\\\", 2) != 0)))
    {
	strlwr (strcpy (inpath, op));
	ReleaseMemoryCell (op);
    }
#  endif

    return PATH_TO_UNIX (inpath);
}
#endif

/* UNIX Version */

#if (OS_TYPE == OS_UNIX)
static char * F_LOCAL GetPhysicalPath (char *inpath, bool AllowNetwork)
{
#  ifdef S_IFLNK
    char		*op;
    struct stat		s;
    char		*res;
    int			len;

    if (((op = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL) ||
	(lstat (inpath, &s) != 0) || (s.st_mode & S_IFMT) != S_IFLNK)
	return inpath;

    if ((len = readlink (inpath, op, FFNAME_MAX)) == -1)
	return inpath;
	
    op [len] = 0;
    strcpy (inpath, op);
    ReleaseMemoryCell (op);

#  endif

    return inpath;
}
#endif
 

/*
 * Change the verify status.  No NT or UNIX functionality.
 */

static void F_LOCAL SetVerifyStatus (bool On)
{
#if (OS_TYPE == OS_OS2) 
    DosSetVerify (On);
#elif (OS_TYPE == OS_DOS)
    union REGS	r;

    r.x.REG_AX = On ? 0x2e01 : 0x2e00;
    r.x.REG_DX = 0;
    DosInterrupt (&r, &r);
#endif
}

/*
 * Change the break status
 */

#if (OS_TYPE == OS_DOS)
static void F_LOCAL SetBreakStatus (bool On)
{
    union REGS	r;

    r.x.REG_AX = 0x3301;
    r.h.dl = (unsigned char)(On ? 1 : 0);
    DosInterrupt (&r, &r);
}
#endif

/*
 * Bind an EMACS keystroke to a editing command
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
static int	dobind (int argc, char **argv)
{
    bool	macro = FALSE;
    char	*cp;
    int		c;

    ResetGetOptions ();			/* Reset GetOptions		*/

    while ((c = GetOptions (argc, argv, "m", 0)) != EOF)
    {
	if (c == 'm')
	    macro = TRUE;
	
	else
	    return UsageError ("bind [ -m ] [ keystroke=edit command ] ...");
    }

    argv += OptionIndex;

/* List All ? */

    if (*argv == (char *)NULL)	/* list all */
	return BindKeyStroke ((char *)NULL, (char *)NULL, FALSE);

/* Otherwise, bind */

    c = 0;

    while (*argv != (char *)NULL)
    {
	if ((cp = strchr (*argv, '=')) != (char *)NULL)
	    *(cp++) = 0;

	c |= BindKeyStroke (*(argv++), cp, macro);
    }

    return c;
}
#endif

static int	doshellinfo (int argc, char **argv)
{
    static char		*OSName [] = { LIT_dos, "Windows", "OS/2", "Win NT",
    				       "UNIX"};

    printf ("Global Flags              = 0x%.4x\n", ShellGlobalFlags);
    printf ("Max # of File descriptors = %d\n", MaxNumberofFDs);
    printf ("Disabled Variables        = 0x%.4x\n", DisabledVariables);
    printf ("Execute stack depth       = %d\n", Execute_stack_depth);
    printf ("Memory stack depth        = %d\n", MemoryAreaLevel);
    printf ("IO stack depth            = %d\n", NSave_IO_E);
    printf ("Subshell stack depth      = %d\n", NSubShells);
    printf ("Underlying OS             = <%s>\n", OSName[BaseOS]);
    return 0;
}

/*
 * Flag OS/2 process for _EMX_
 */

#if defined (__EMX__) && (OS_TYPE == OS_OS2)
USHORT	Dos32FlagProcess (PID pid, USHORT fScope, USHORT usFlagNum,
			  USHORT usFlagArg)
{
    return ((USHORT)
	    (_THUNK_PROLOG (2 + 2 + 2 + 2);
	     _THUNK_SHORT (pid);
	     _THUNK_SHORT (fScope);
	     _THUNK_SHORT (usFlagNum);
	     _THUNK_SHORT (usFlagArg);
	     _THUNK_CALL (Dos16FlagProcess)));
}
#endif

/*
 * Check file access
 */

static int F_LOCAL CheckFAccess (char *Name, int mode)
{
    int		fdn = CheckForFD (CheckDOSFileName (Name));
#if (OS_TYPE == OS_UNIX)
    struct stat		s;
    mode_t		tm = 0;
#else
    OSCALL_PARAM	usAttr;
#endif

    if (fdn < 0)
	return S_access (Name, mode);

#if (OS_TYPE == OS_UNIX)
    if (fstat (fdn, &s) < 0)
        return 0;

    if (mode == 0)
	return 1;

/* Find which bits to check */

    if (mode & W_OK)
	tm |= S_IWUSR;

    if (mode & R_OK)
	tm |= S_IRUSR;

    if (mode & X_OK)
	tm |= S_IXUSR;

/* Are we the owner, or group owner? */

    if (getuid () != st.st_uid)
    {
	tm >>= 3;

	if (getgid () != st.st_gid)
	    tm >>= 3;
    }

    return s.st_mode & tm;
#else

/* Non-Unix is almost as much pain */

    if (OS_GetFHAttributes (fdn, &usAttr) != 0)
        return 0;

    if ((mode & W_OK) && (usAttr & OS_FILE_READONLY))
        return 0;

    if (mode & X_OK)
        return 0;

    return 1;
#endif
}

/*
 * Check file type
 */

static int F_LOCAL CheckFType (char *Name, mode_t mode)
{
    struct stat		s;
    int			fdn = CheckForFD (CheckDOSFileName (Name));
#if (OS_TYPE != OS_UNIX)
    int			ftype;
#endif

/*
 * File Name
 */

    if (fdn < 0)
        return (S_stat (Name, &s) && ((s.st_mode & S_IFMT) == mode)) ? 1 : 0;

/*
 * File descriptor
 */
#if (OS_TYPE == OS_UNIX)
    return ((fstat (fdn, &s) >= 0) && ((s.st_mode & S_IFMT) == mode)) ? 1 : 0;
#else

/* Usual OS/2, MSDOS, WINNT pain */
    ftype = GetDescriptorType (fdn);

    if ((mode == S_IFREG) && (ftype == DESCRIPTOR_FILE))
        return 1;

    if ((mode == S_IFCHR) && 
        ((ftype == DESCRIPTOR_DEVICE) || (ftype == DESCRIPTOR_CONSOLE)))
        return 1;

    return 0;
#endif
}

/*
 * Check file mode
 */

static int F_LOCAL CheckFMode (char *Name, mode_t mode)
{
    int			fdn = CheckForFD (CheckDOSFileName (Name));
#if (OS_TYPE == OS_UNIX)
    struct stat		s;
#else
    OSCALL_PARAM	usAttr;
#endif

#if (OS_TYPE == OS_UNIX)
    if (((fdn < 0) && !S_stat (Name, &s)) ||
	((fdn >= 0) && (fstat (fdn, &s) < 0)))
	return 0;

    return (s.st_mode & mode);
#else
    if (fdn < 0)
	return (OS_GetFileAttributes (Name, &usAttr) == 0) && (usAttr & mode);

    return (OS_GetFHAttributes (fdn, &usAttr) == 0) && (usAttr & mode);
#endif
}

/*
 * Check file size
 */

static int F_LOCAL CheckFSize (char *Name)
{
    struct stat		s;
    int			fdn = CheckForFD (CheckDOSFileName (Name));

    if (((fdn < 0) && !S_stat (Name, &s)) ||
#if (OS_TYPE == OS_UNIX)
	((fdn >= 0) && (fstat (fdn, &s) < 0)))
#else
	((fdn >= 0) && ((s.st_size = filelength (fdn)) < 0)))
#endif
	return 0;
      
    return (s.st_size > 0L);
}

/*
 * Does this refer to a file descriptor - check for /dev/fd/n
 */

static int F_LOCAL CheckForFD (char *name)
{
    if (strncmp (name, LIT_devfd, sizeof (LIT_devfd) - 1) != 0)
	return -1;
    
    return atoi (name + sizeof (LIT_devfd) - 1);
}

/*
 * Get File Handler Attributes
 */

/*
 * Under DOS, there is no way to get the file attributes
 */

#if (OS_TYPE == OS_DOS)
static OSCALL_RET F_LOCAL OS_GetFHAttributes (int fd, OSCALL_PARAM *usAttr)
{
    if (lseek (fd, 0L, SEEK_CUR) == -1L)
        return -1;

    *usAttr = 0;
    return 0;
}
#endif

/*
 * OS/2 Version
 */

#if (OS_TYPE == OS_OS2)
static OSCALL_RET F_LOCAL OS_GetFHAttributes (int fd, OSCALL_PARAM *usAttr)
{
    OSCALL_RET		rc;
#  if (OS_SIZE == OS_32)
    FILESTATUS3		info;
#  else
    FILESTATUS		info;
#  endif

    rc = DosQFileInfo (fd, FIL_STANDARD, &info, sizeof (info));

    if (rc != 0)
	return rc;

    *usAttr = info.attrFile;
    return 0;
}
#endif

/*
 * NT Version
 */

#if (OS_TYPE == OS_NT)
static OSCALL_RET F_LOCAL OS_GetFHAttributes (int fd, OSCALL_PARAM *usAttr)
{
    extern long _CRTAPI1	_get_osfhandle (int);
    BY_HANDLE_FILE_INFORMATION	info;
    HANDLE			osfp = (HANDLE)_get_osfhandle (fp);

    switch (GetFileType (osfp))

    if (!GetFileInformationByHandle(osfp, &info))
	return 1;

    *usAttr = info.dwFileAttributes;
    return 0;
}
#endif
