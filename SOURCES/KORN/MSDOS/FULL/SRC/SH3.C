/*
 * MS-DOS SHELL - Parse Tree Executor
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
 * he incorporated into his shell, in particular the TCASE and TTIME
 * functionality in ExecuteParseTree, and the PrintAList function.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh3.c,v 2.17 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh3.c,v $
 *	Revision 2.17  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.16  1994/02/23  09:23:38  istewart
 *	Beta 233 updates
 *
 *	Revision 2.15  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.14  1994/01/20  14:51:43  istewart
 *	Release 2.3 Beta 1
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
 *	Revision 2.0  1992/05/07  20:31:39  Ian_Stewartson
 *	MS-Shell 2.0 Baseline release
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
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
#include <ctype.h>
#include <time.h>
#include "sh.h"

/*
 * Save struct for Parameters ($1, $2 etc)
 */

typedef struct SaveParameters {
    char	**Array;		/* The parameters		*/
    int		Count;			/* Number of them		*/
} SaveParameters;

/* static Function and string declarations */

static int F_LOCAL	ForkAndExecute (C_Op *, int, int, int, char **, char **,
					char **);
static bool F_LOCAL	SetUpIOHandlers (IO_Actions *, int, int);
static bool F_LOCAL	WriteToExtendedFile (FILE *, char *);
static void F_LOCAL	EchoCurrentCommand (char **);
static int F_LOCAL	ExecuteProgram (char *, char **, char **, int);
static bool F_LOCAL	CheckParameterLength (char **);
static void F_LOCAL	SaveNumericParameters (char **, SaveParameters *);
static void F_LOCAL	RestoreTheParameters (SaveParameters *);
static bool F_LOCAL	ExecuteFunction (char **, int *, bool);
static void F_LOCAL	PrintLoadError (char *);
static void F_LOCAL	TrackAllCommands (char *, char *);
static int F_LOCAL	ExtensionType (char *);
static char * F_LOCAL	FindFileAndExtension (char *, char *, char **);
static bool F_LOCAL	GetApplicationType (char *);
static bool F_LOCAL	BadApplication (char *);
static bool F_LOCAL	SetUpCLI (char *, Word_B **);

#ifdef OS_SWAPPING
static bool F_LOCAL	Get_EMS_Driver (void);
static bool F_LOCAL	Get_XMS_Driver (void);
static bool F_LOCAL	EMS_error (char *, int);
static int F_LOCAL	EMS_Close (void);
static bool F_LOCAL	XMS_error (char *, int);
static int F_LOCAL	XMS_Close (void);
static int F_LOCAL	SwapToDiskError (int, char *);
static int F_LOCAL	SwapToMemory (int, char **);
static int F_LOCAL	SpawnProcess (char **);
static int F_LOCAL	CheckForExecOnly (char *, int, int);
#endif

static char * F_LOCAL	ConvertErrorNumber (void);
static int F_LOCAL	BuildCommandLine (char *, char **, char **, int);
static int F_LOCAL	StartTheProcess (char *, char **, char **, int);
static int F_LOCAL	SetCommandReturnStatus (int);
static char ** F_LOCAL	FindNumberOfValues (char **, int *);
static int F_LOCAL	ExecuteScriptFile (char *, char **, char **, int, bool);
#if (OS_TYPE != OS_UNIX)
static int F_LOCAL	ExecuteWindows (char *, char **, char **, int);
#endif
static int F_LOCAL	ExecuteSpecialProcessor (char *, char **, char **, int,
						 Word_B *);
static int F_LOCAL	EnvironExecute (char **, int);
static int F_LOCAL 	LocalExecve (char **, char **, int);
static unsigned int F_LOCAL CheckForCommonOptions (LineFields *, int);
static char ** F_LOCAL	ProcessSpaceInParameters (char **);
static int F_LOCAL	CountDoubleQuotes (char *);
static void		BuildEnvironmentEntry (const void *, VISIT, int);
static char ** F_LOCAL	BuildCommandEnvironment (void);

#if (OS_TYPE != OS_DOS)
static void F_LOCAL	PrintPidStarted (void);

#  if (OS_TYPE == OS_OS2)
static char		*InsertCharacterAtStart (char *);
static int F_LOCAL	OS_DosExecProgram (int, char *, char **, char **);
#  elif (OS_TYPE == OS_NT)
static int F_LOCAL	OS_DosExecProgram (int, char *, char **, char **);
#  endif
#else
#  define PrintPidStarted()
#  define InsertCharacterAtStart(a)
#endif

#if (OS_TYPE == OS_OS2)
static int F_LOCAL	StartTheSession (STARTDATA *, char *, char **,
					 char **, int);
#endif

static char		*AE2big = "arg/env list too big";
			/* Extended Command line processing file name	*/
static char		*Extend_file = (char *)NULL;
static char		*DoubleQuotes = "\"";
static char		*WildCards = "*?[\"'";
static unsigned long	ApplicationType;

#if (OS_TYPE == OS_DOS)
static char		*LIT_STARTWINP = "STARTWINP";
#endif

/*
 * Global variables for TWALK to build command environment
 */

static Word_B	*BCE_WordList;
static int	BCE_Length;

/*
 * List of Executable (shell script, .exe, .com) extensions and list
 * including functions.
 */

static char	**ExecutableList = (char **)NULL;
static char	**ExecuteFunctionList = (char **)NULL;

/* Swapping messages */

#ifdef OS_SWAPPING
static char	*NoSwapFiles = "No Swap files";
static char	*MS_emsg = "Warning: %s Error (%x)";
static char	*MS_Space = "Warning: %s out of space";
static char	*SwapFailed = "%s swap failed (%x)";
static char	*Swap_File = (char *)NULL;	/* Swap file	*/
#endif

/*
 * OS2 load error mode
 */

#if (OS_TYPE == OS_OS2) 
static char		FailName[FFNAME_MAX];
#endif

#if (OS_TYPE != OS_DOS) 
static OSCALL_RET	OS_DosExecPgmReturnCode;
#endif

/*
 * Program started info
 */

#if (OS_TYPE != OS_DOS) 
struct PidInfo {
    bool	Valid;
    int		JobNo;
    int		PidNo;
} PidInfo;
#endif

/*
 * Common fields in EXTENDED_LINE file
 */

#define COMMON_FIELD_COUNT	ARRAY_SIZE (CommonFields)

static struct CommonFields {
    char		*Name;
    unsigned int	Flag;
} CommonFields [] = {
    { "switch",		EP_CONVERT },
    { LIT_export,	EP_EXPORT },
    { "noswap",		EP_NOSWAP },
    { "noexpand",	EP_NOEXPAND },
    { "noquote",	EP_NOQUOTE },
    { "ignoretype",	EP_IGNTYPE },
    { "pipetty",	EP_PSEUDOTTY },
    { "quotewild",	EP_QUOTEWILD }
};

/*
 * execute tree recursively
 */

int ExecuteParseTree (C_Op	*t,
		      int	StandardIN,
		      int	StandardOUT,
		      int	Actions)
{
    int			Count;
    int			LocalPipeFP;	/* Pipe handlers		*/

#if (OS_TYPE != OS_DOS) 
    int			ReadPipeFP;
    int			WritePipeFP;
#  if OS_TYPE == OS_NT
    extern int		_open_osfhandle (long, int);
    HANDLE		ReadPipeH;
    HANDLE		WritePipeH;
#  endif
#endif

    char		*cp;
    char		**AList;	/* Argument list		*/
    Break_C		BreakContinue;
					/* Save longjmp returns		*/
    Break_C		*S_RList = Return_List;
    Break_C		*S_BList = Break_List;
    Break_C		*S_SList = SShell_List;

    GetoptsIndex	GetoptsSave;
    int			Local_depth;	/* Save local values		*/
    int			Local_MemoryAreaLevel;
    int			RetVal = 0;	/* Return value			*/
    char		*InputBuffer;	/* Select input Buffer		*/
    char		*EndIB;		/* End of buffer		*/
    char		*LastWord = null;

/* End of tree ? */

    if (t == (C_Op *)NULL)
	return SetCommandReturnStatus (0);

    DPRINT (1, ("ExecuteParseTree: t->type = %d, Depth = %d",
    		t->type, Execute_stack_depth));

/* Save original and Increment execute function recursive level */

    Local_depth = Execute_stack_depth++;

/* Save original and increment area number */

    Local_MemoryAreaLevel = MemoryAreaLevel++;

/* Switch on tree node type */

    switch (t->type)
    {
	case TFUNC:			/* name () { list; }	*/
	    RetVal = SaveFunction (t) ? 0 : 1;
	    SetCommandReturnStatus (RetVal);
	    break;

/* In the case of a () command string, we need to save and restore the
 * current environment, directory and traps (can't think of anything else).
 * For any other, we just restore the current directory.  Also, we don't
 * want changes in the Variable list header saved for SubShells, because
 * we are effectively back at execute depth zero.
 */
	case TPAREN:			/* ()			*/
	    if ((RetVal = CreateGlobalVariableList (FLAGS_NONE)) == -1)
		break;

/* Save Getopts pointers */

	    GetGetoptsValues (&GetoptsSave);


	    if (setjmp (BreakContinue.CurrentReturnPoint) == 0)
	    {
		Return_List = (Break_C *)NULL;
		Break_List  = (Break_C *)NULL;
		BreakContinue.NextExitLevel  = SShell_List;
		SShell_List = &BreakContinue;
		RetVal = ForkAndExecute (t, StandardIN, StandardOUT, Actions,
					 NOWORDS, NOWORDS, &LastWord);
	    }

/* Restore the original environment */

	    else
		RetVal = (int)GetVariableAsNumeric (StatusVariable);

	    SaveGetoptsValues (GetoptsSave.Index, GetoptsSave.SubIndex);
	    Return_List	= S_RList;
	    Break_List	= S_BList;
	    SShell_List	= S_SList;
	    RestoreEnvironment (RetVal, Local_depth);
	    break;

/* After a normal command, we need to restore the original directory.  Note
 * that a cd will have updated the variable $~, so no problem
 */

	case TCOM:			/* A command process	*/
	{
	    ExeMode 	SaveValues;
	    char	**SetVlist;	/* Set variable list		*/

	    SetVlist = ExpandWordList (t->vars, EXPAND_TILDE, (ExeMode *)NULL);
	    AList = ExpandWordList (t->args, EXPAND_SPLITIFS | EXPAND_GLOBBING |
				    EXPAND_TILDE, &SaveValues);

	    ExecProcessingMode = SaveValues;

#if (OS_TYPE != OS_DOS) 
	    PidInfo.Valid = FALSE;
#endif

	    RetVal = ForkAndExecute (t, StandardIN, StandardOUT, Actions,
				     AList, SetVlist, &LastWord);

	    PrintPidStarted ();
	    RestoreEnvironment (RetVal, Local_depth);

/* Save last word if appropriate */

	    if (!(DisabledVariables & DISABLE_LASTWORD))
	    {
		SetVariableFromString (LastWordVariable, LastWord);
		SetVariableStatus (LastWordVariable, STATUS_EXPORT);
	    }
	    break;
	}

	case TTIME:			/* Time a command process	*/
	{
	    clock_t	stime;
	    clock_t	etime;
	    clock_t	dif;

	    stime = clock ();
	    RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT, 0);
	    etime = clock ();

	    feputc (CHAR_NEW_LINE);

	    if ((dif = (etime - stime) / (60L * (clock_t)CLOCKS_PER_SEC)))
		fprintf (stderr, "%ldm ", dif);

	    dif = (etime - stime) % (60L * (clock_t)CLOCKS_PER_SEC);

	    fprintf (stderr, "%ld.%.3lds real\n",
		     dif / (clock_t)CLOCKS_PER_SEC,
		     dif % (clock_t)CLOCKS_PER_SEC);

	    break;
	}

	case TPIPE:			/* Pipe processing		*/

	    Actions |= EXEC_PIPE_IN;

#if (OS_TYPE == OS_UNIX) 
	    fputs ("UNIX: Pipes not implemented\n", stderr);
	    RetVal = -1;

#else
#  if (OS_TYPE != OS_DOS) 
/* Do we want to use real pipes under OS2? */

	    if (ShellGlobalFlags & FLAGS_REALPIPES)
	    {
#    if (OS_TYPE == OS_OS2)
#      if (OS_SIZE == OS_32)
		if (DosCreatePipe ((PHFILE) &ReadPipeFP,
				   (PHFILE) &WritePipeFP, 4096))
		    break;
#      else
		if (DosMakePipe ((PHFILE) &ReadPipeFP,
				 (PHFILE) &WritePipeFP, 0))
		    break;
#      endif

/* Remap the IO handler */

		ReadPipeFP = ReMapIOHandler (ReadPipeFP);
		WritePipeFP = ReMapIOHandler (WritePipeFP);
		DosSetFHandState (ReadPipeFP, OPEN_FLAGS_NOINHERIT);
		DosSetFHandState (WritePipeFP, OPEN_FLAGS_NOINHERIT);

#    elif (OS_TYPE == OS_NT)
		if (!CreatePipe (&ReadPipeH, &WritePipeH, 0, 0))
		    break;
		
/* Remap the IO handler */

		ReadPipeFP = _open_osfhandle ((long)ReadPipeH, _O_RDONLY);
		WritePipeFP = _open_osfhandle ((long)WritePipeH, _O_APPEND);
		ReadPipeFP = ReMapIOHandler (ReadPipeFP);
		WritePipeFP = ReMapIOHandler (WritePipeFP);
#    endif


/* Is this a foreground thingy? */

		if (!(Actions & (EXEC_SPAWN_NOWAIT | EXEC_SPAWN_IGNOREWAIT)))
		{
		    int		WaitPid;

		    WaitPid = ExecuteParseTree (t->left, StandardIN,
						WritePipeFP,
					        EXEC_SPAWN_IGNOREWAIT |
	    					(Actions & (EXEC_PIPE_IN |
							    EXEC_PIPE_SUBS)));
		    S_close (WritePipeFP, TRUE);

		    RetVal = ExecuteParseTree (t->right, ReadPipeFP,
					       StandardOUT,
					       Actions | EXEC_PIPE_SUBS);
		    S_close (ReadPipeFP, TRUE);
		    cwait (&WaitPid, WaitPid, WAIT_GRANDCHILD);
		}

/* Background processing */

		else
		{
		    ExecuteParseTree (t->left, StandardIN, WritePipeFP,
				      EXEC_SPAWN_IGNOREWAIT |
				      (Actions & (EXEC_PIPE_IN |
						  EXEC_PIPE_SUBS)));

		    S_close (WritePipeFP, TRUE);

		    RetVal = ExecuteParseTree (t->right, ReadPipeFP,
					       StandardOUT,
					       Actions | EXEC_PIPE_SUBS);
		    S_close (ReadPipeFP, TRUE);
		}

		break;
	    }
#  endif

/* MSDOS or OS/2 without real pipes - use files.  Safer */

	    if ((RetVal = OpenAPipe ()) < 0)
		break;

/* Create pipe, execute command, reset pipe, execute the other side, close
 * the pipe and fini
 */

	    LocalPipeFP = ReMapIOHandler (RetVal);
	    ExecuteParseTree (t->left, StandardIN, LocalPipeFP,
			      (Actions & (EXEC_PIPE_IN | EXEC_PIPE_SUBS)));

/* Close the Input to release the file descriptor */

	    CloseThePipe (StandardIN);

	    lseek (LocalPipeFP, 0L, SEEK_SET);
	    RetVal = ExecuteParseTree (t->right, LocalPipeFP, StandardOUT,
				       Actions | EXEC_PIPE_SUBS);
	    CloseThePipe (LocalPipeFP);
#endif
	    break;

	case TLIST:			/* Entries in a for statement	*/
	    while (t->type == TLIST)
	    {
		ExecuteParseTree (t->left, StandardIN, StandardOUT, 0);
		t = t->right;
	    }

	    RetVal = ExecuteParseTree (t, StandardIN, StandardOUT, 0);
	    break;

	case TCOPROCESS:		/* Co processes			*/
	    if (!FL_TEST (FLAG_WARNING))
		PrintWarningMessage ("sh: co-processes not supported");

	    SetCommandReturnStatus (RetVal = -1);
	    break;

	case TASYNC:			/* Async - not supported	*/
#if (OS_TYPE == OS_DOS) 
	    if (!FL_TEST (FLAG_WARNING))
		PrintWarningMessage ("sh: Async commands not supported");

	    SetCommandReturnStatus (RetVal = -1);
#else
	    RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT,
				       EXEC_SPAWN_NOWAIT);
#endif
	    break;

	case TOR:			/* || and &&			*/
	case TAND:
	    RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT, 0);

	    if ((t->right != (C_Op *)NULL) &&
		((RetVal == 0) == (t->type == TAND)))
		RetVal = ExecuteParseTree (t->right, StandardIN, StandardOUT,
					   0);

	    break;


/* for x do...done and for x in y do...done - find the start of the variables
 * count the number.
 */
	case TFOR:
	case TSELECT:
	    AList = FindNumberOfValues (ExpandWordList (t->vars,
							EXPAND_SPLITIFS |
						        EXPAND_GLOBBING |
						        EXPAND_TILDE,
							(ExeMode *)NULL),
							&Count);


/* Set up a long jump return point before executing the for function so that
 * the continue statement is executed, ie we reprocessor the for condition.
 */

	    while ((RetVal = setjmp (BreakContinue.CurrentReturnPoint)))
	    {

/* Restore the current stack level and clear out any I/O */

		RestoreEnvironment (0, Local_depth + 1);
		Return_List = S_RList;
		SShell_List = S_SList;

/* If this is a break - clear the variable and terminate the while loop and
 * switch statement
 */

		if (RetVal == BC_BREAK)
		    break;
	    }

	    if (RetVal == BC_BREAK)
		break;

/* Process the next entry - Add to the break/continue chain */

	    BreakContinue.NextExitLevel = Break_List;
	    Break_List = &BreakContinue;

/* Execute the command tree */

	    if (t->type == TFOR)
	    {
		while (Count--)
		{
		    SetVariableFromString (t->str, *AList++);
		    RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT,
					       0);
		}
	    }

/* Select option */

	    else if (!Count)
	    /* SKIP */;

/* Get some memory for the select input buffer */

	    else if ((InputBuffer = AllocateMemoryCell (LINE_MAX))
			== (char *)NULL)
	    {
		ShellErrorMessage (Outofmemory1);
		RetVal = -1;
	    }

/* Process the select command */

	    else
	    {
		bool	OutputList = TRUE;

		EndIB = &InputBuffer[LINE_MAX - 2];

		while (TRUE)
		{
		    int		ReadCount;	/* Local counter	*/
		    int		OnlyDigits;	/* Only digits in string*/

/* Output list of words */

		    if (OutputList)
		    {
		        PrintAList (Count, AList);
		        OutputList = FALSE;
		    }

/* Output prompt */

		    OutputUserPrompt (PS3);
		    OnlyDigits = 1;

/* Read in until end of line, file or a field separator is detected */

		    for (cp = InputBuffer; (cp < EndIB); cp++)
		    {
			if (((ReadCount = read (STDIN_FILENO, cp, 1)) != 1) ||
			    (*cp == CHAR_NEW_LINE))
			{
			    break;
			}

			OnlyDigits = OnlyDigits && isdigit (*cp);
		    }

		    *cp = 0;

/* Check for end of file */

		    if (ReadCount != 1)
			break;

/* Check for empty line */

		    if (!strlen (InputBuffer))
		    {
			OutputList = TRUE;
			continue;
		    }

		    SetVariableFromString (LIT_REPLY, InputBuffer);

/* Check that OnlyDigits is a valid number in the select range */

		    if (OnlyDigits &&
			((OnlyDigits = atoi (InputBuffer)) > 0) &&
			(OnlyDigits <= Count))
			SetVariableFromString (t->str, AList[OnlyDigits - 1]);

		    else
			SetVariableFromString (t->str, null);

		    RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT,
					       0);
		}
	    }

/* Remove this tree from the break list */

	    Break_List = S_BList;
	    break;

/* While and Until function.  Similar to the For function.  Set up a
 * long jump return point before executing the while function so that
 * the continue statement is executed OK.
 */

	case TWHILE:			/* WHILE and UNTIL functions	*/
	case TUNTIL:
	    while ((RetVal = setjmp (BreakContinue.CurrentReturnPoint)))
	    {

/* Restore the current stack level and clear out any I/O */

		RestoreEnvironment (0, Local_depth + 1);
		Return_List = S_RList;
		SShell_List = S_SList;

/* If this is a break, terminate the while and switch statements */

		if (RetVal == BC_BREAK)
		    break;
	    }

	    if (RetVal == BC_BREAK)
		break;

/* Set up links */

	    BreakContinue.NextExitLevel = Break_List;
	    Break_List = &BreakContinue;

	    while ((ExecuteParseTree (t->left, StandardIN, StandardOUT, 0) == 0)
				== (t->type == TWHILE))
		RetVal = ExecuteParseTree (t->right, StandardIN, StandardOUT,
					   0);

	    Break_List = S_BList;
	    break;

	case TIF:			/* IF and ELSE IF functions	*/
	case TELIF:
	    if (t->right != (C_Op *)NULL)
		RetVal = ExecuteParseTree (!ExecuteParseTree (t->left,
							      StandardIN,
							      StandardOUT, 0)
					    ? t->right->left : t->right->right,
					    StandardIN, StandardOUT, 0);

	    break;

	case TCASE:			/* CASE function		*/
	{
	    C_Op	*ts = t->left;

	    cp = ExpandAString (t->str, 0);

	    while ((ts != (C_Op *)NULL) && (ts->type == TPAT))
	    {
		for (AList = ts->vars; *AList != NOWORD; AList++)
		{
		    if ((EndIB = ExpandAString (*AList, EXPAND_PATTERN)) &&
			GeneralPatternMatch (cp, (unsigned char *)EndIB, FALSE,
					     (char **)NULL, GM_ALL))
			goto Found;
		}

		ts = ts->right;
	    }

	    break;

Found:
	    RetVal = ExecuteParseTree (ts->left, StandardIN, StandardOUT, 0);
	    break;
	}

	case TBRACE:			/* {} statement			*/
	    if ((RetVal >= 0) && (t->left != (C_Op *)NULL))
		RetVal = ExecuteParseTree (t->left, StandardIN, StandardOUT,
					   (Actions & EXEC_FUNCTION));

	    break;
    }

/* Processing Completed - Restore environment */

    Execute_stack_depth = Local_depth;

/* Remove unwanted malloced space */

    ReleaseMemoryArea (MemoryAreaLevel);
    MemoryAreaLevel = Local_MemoryAreaLevel;

/* Check for traps */

    if (t->type == TCOM)
    {
	RunTrapCommand (-1);		/* Debug trap			*/

	if (RetVal)
	    RunTrapCommand (-2);	/* Err trap			*/
    }

/* Interrupt traps */

    if ((Count = InterruptTrapPending) != 0)
    {
	InterruptTrapPending = 0;
	RunTrapCommand (Count);
    }

/* Check for interrupts */

    if (InteractiveFlag && IS_TTY (0) && SW_intr)
    {
	CloseAllHandlers ();
	TerminateCurrentEnvironment (TERMINATE_COMMAND);
    }

    return RetVal;
}

/*
 * Restore the original directory
 */

void RestoreCurrentDirectory (char *path)
{
    SetCurrentDrive (GetDriveNumber (*path));

    if (!S_chdir (&path[2]))
    {
	if (!FL_TEST (FLAG_WARNING))
	    feputs ("Warning: current directory reset to /\n");

	S_chdir (DirectorySeparator);
	GetCurrentDirectoryPath ();
    }
}

/*
 * Ok - execute the program, resetting any I/O required
 */

static int F_LOCAL ForkAndExecute (C_Op	*t,
				   int	StandardIN,
				   int	StandardOUT,
				   int	ForkAction,
				   char	**AList,
				   char	**VList,
				   char	**LastWord)
{
    int			RetVal = -1;	/* Return value			*/
    int			(*shcom)(int, char **) = (int (*)(int, char **))NULL;
    char		*cp;
    char		**alp;
    IO_Actions		**iopp = t->ioact;
    int			builtin = 0;	/* Builtin function		*/
    bool		CGVLCalled = FALSE;
    bool		InternalExec = FALSE;
    int			Index;

    if (t->type == TCOM)
    {
	cp = *AList;

/* strip all initial assignments not correct wrt PATH=yyy command  etc */

	if (FL_TEST (FLAG_PRINT_EXECUTE) ||
	   ((CurrentFunction != (FunctionList *)NULL) &&
	    CurrentFunction->Traced))
	    EchoCurrentCommand (cp != NOWORD ? AList : VList);

/* Is it only an assignement? */

	if ((cp == NOWORD) && (t->ioact == (IO_Actions **)NULL))
	{
	    while (((cp = *(VList++)) != NOWORD) &&
		   AssignVariableFromString (cp, &Index))
		continue;

#if (OS_TYPE != OS_DOS) 
	    ExitWithJobsActive = FALSE;
#endif

/* Get the status variable if expand changed it */

	    return (int)GetVariableAsNumeric (StatusVariable);
	}

/* Check for built in commands */

	else if (cp != NOWORD)
	{
	    shcom = IsCommandBuiltIn (cp, &builtin);
	    InternalExec = C2bool ((strcmp (cp, LIT_exec)) == 0);

/*
 * Reset the ExitWithJobsActive flag to enable the 'jobs active' on exit
 * message
 */

#if (OS_TYPE != OS_DOS) 
	    if (strcmp (cp, LIT_exit))
		ExitWithJobsActive = FALSE;
#endif
	}

#if (OS_TYPE != OS_DOS) 
	else
	    ExitWithJobsActive = FALSE;
#endif
    }

#if (OS_TYPE != OS_DOS) 
    else
	ExitWithJobsActive = FALSE;
#endif

/*
 * UNIX fork simulation?
 */

/* If there is a command to execute or we are exec'ing and this is not a
 * TPAREN, save the current environment
 */

    if (t->type != TPAREN)
    {
	if (((*VList != NOWORD) &&
	     ((builtin & BLT_SKIPENVIR) != BLT_SKIPENVIR)) ||
	    (ForkAction & EXEC_WITHOUT_FORK))
	{
	    if (CreateGlobalVariableList (FLAGS_FUNCTION) == -1)
		return -1;

	    CGVLCalled = TRUE;
	}

/* Set up any variables.  Note there is an assumption that
 * AssignVariableFromString sets the equals sign to 0, hiding the value;
 */

	if (shcom == (int (*)(int, char **))NULL)
	{
	    while (((cp = *(VList++)) != NOWORD) &&
		   AssignVariableFromString (cp, &Index))
		SetVariableArrayStatus (cp, Index,
					(STATUS_EXPORT | STATUS_LOCAL));

/* If the child shells from this process communicate via pipes, set the
 * environment so the child shell will know.  This is a special case for OS/2
 * (and Win NT?) EMACS.
 */

	   if (ExecProcessingMode.Flags & EP_PSEUDOTTY)
	   {
	       char	temp[30];

	       AssignVariableFromString (strcat (strcpy (temp, LIT_AllowTTY),
						 "=true"), &Index);
	       SetVariableArrayStatus (temp, Index,
				       (STATUS_EXPORT | STATUS_LOCAL));
	   }
	}
    }


/* If this is the first command in a pipe, set stdin to /dev/null */

   if ((ForkAction & (EXEC_PIPE_IN | EXEC_PIPE_SUBS)) == EXEC_PIPE_IN)
   {
	if ((Index = S_open (FALSE, "/dev/null", O_RDONLY)) != 0)
	{
	    S_dup2 (Index, 0);
	    S_close (Index, TRUE);
	}
   }

/* We cannot close the pipe, because once the exec/spawn has taken place
 * the processing of the pipe is not yet complete.
 */

    if (StandardIN != NOPIPE)
    {
	S_dup2 (StandardIN, STDIN_FILENO);
	/*lseek (STDIN_FILENO, 0L, SEEK_SET);*/
    }

    if (StandardOUT != NOPIPE)
    {
	FlushStreams ();
	S_dup2 (StandardOUT, STDOUT_FILENO);
	lseek (STDOUT_FILENO, 0L, SEEK_END);
    }

/* Set up any other IO required */

    FlushStreams ();

    if (iopp != (IO_Actions **)NULL)
    {
	while (*iopp != (IO_Actions *)NULL)
	{
	    if (SetUpIOHandlers (*(iopp++), StandardIN, StandardOUT))
		return RetVal;
	}
    }


/* All fids above 10 are autoclosed in the exec file because we have used
 * the O_NOINHERIT flag.  Note I patched open.obj to pass this flag to the
 * open function.
 */

    if (t->type == TPAREN)
	return RestoreStandardIO (ExecuteParseTree (t->left, NOPIPE, NOPIPE, 0),
				  TRUE);

/* Are we just changing the I/O re-direction for the shell ? */

    if (*AList == NOWORD)
    {
	if ((ForkAction & EXEC_WITHOUT_FORK) == 0)
	    RestoreStandardIO (0, TRUE);

/* Get the status variable if expand changed it */

	return (int)GetVariableAsNumeric (StatusVariable);
    }

/*
 * Find the end of the parameters and set up $_ environment variable
 */

    alp = AList;
    while (*alp != NOWORD)
       alp++;

/* Move back to last parameter, and save it */

    *LastWord = StringCopy (*(alp - 1));

/* No - Check for a function the program.  At this point, we need to put
 * in some processing for return.
 */

    if (!(builtin & BLT_CURRENT) &&
	ExecuteFunction (AList, &RetVal, CGVLCalled))
	return RetVal;

/* Check for another drive or directory in the restricted shell */

    if ((strpbrk (*AList, ":/\\") != (char *)NULL) &&
	CheckForRestrictedShell (*AList))
	return RestoreStandardIO (-1, TRUE);

/* A little cheat to allow us to use the same code to start OS/2 sessions
 * as to load and execute a program
 */

#if (OS_TYPE == OS_OS2) 
    SessionControlBlock = (STARTDATA *)NULL;
#endif

/* Ok - execute the program */

    if (!(builtin & BLT_CURRENT))
    {
	RetVal = EnvironExecute (AList, ForkAction);

	if (ExecProcessingMode.Flags != EP_ENVIRON)
	    RetVal = LocalExecve (AList, BuildCommandEnvironment (),
				  ForkAction);
    }

/* If we didn't find it, check for internal command
 *
 * Note that the exec command is a special case
 */

    if ((builtin & BLT_CURRENT) || ((RetVal == -1) && (errno == ENOENT)))
    {
	if (shcom != (int (*)(int, char **))NULL)
	{
	    if (InternalExec)
		RetVal = doexec (t);

	    else
		RetVal = (*shcom)(CountNumberArguments (AList), AList);

	    SetCommandReturnStatus (RetVal);
	}
    }

    if (RetVal == -1)
	PrintLoadError (*AList);

    return RestoreStandardIO (RetVal, TRUE);
}

/*
 * Restore Local Environment
 */

void RestoreEnvironment (int retval, int stack)
{
    Execute_stack_depth = stack;
    DeleteGlobalVariableList ();
    RestoreCurrentDirectory (CurrentDirectory->value);
    RestoreStandardIO (SetCommandReturnStatus (retval), TRUE);
}

/*
 * Set up I/O redirection.  0< 1> are ignored as required within pipelines.
 */

static bool F_LOCAL SetUpIOHandlers (IO_Actions	*iop,
				     int	pipein,
				     int	pipeout)
{
    int		u;
    char	*cp, *msg;

/* Check for pipes */

    if ((pipein != NOPIPE) && (iop->io_unit == STDIN_FILENO))
	return FALSE;

    if ((pipeout != NOPIPE) && (iop->io_unit == STDOUT_FILENO))
	return FALSE;

    msg = ((iop->io_flag & IOTYPE) == IOWRITE) ? "create" : "open";

    if ((iop->io_flag & IOTYPE) != IOHERE)
	cp = ExpandOneStringFirstComponent (iop->io_name,
					    EXPAND_TILDE | EXPAND_GLOBBING);

/*
 * Duplicate - check for close
 */

    if ((iop->io_flag & IOTYPE) == IODUP)
    {
	if ((cp[1]) || (!isdigit (*cp) && (*cp != CHAR_CLOSE_FD)))
	{
	    ShellErrorMessage ("illegal >& argument (%s)", cp);
	    return TRUE;
	}

	if (*cp == CHAR_CLOSE_FD)
	{
	    iop->io_flag &= ~IOTYPE;
	    iop->io_flag |= IOCLOSE;
	}
    }

/*
 * When writing to /dev/???, we have to cheat because MSDOS appears to
 * have a problem with /dev/ files after find_first/find_next.
 */

#if (OS_TYPE != OS_UNIX)
    if (((iop->io_flag & IOTYPE) == IOWRITE) &&
	(strnicmp (cp, DeviceNameHeader, LEN_DEVICE_NAME_HEADER) == 0))
    {
	iop->io_flag &= ~IOTYPE;
	iop->io_flag |= IOCAT;
    }
#endif

/* Open the file in the appropriate mode */

    switch (iop->io_flag & IOTYPE)
    {
	case IOREAD:				/* <			*/
	    u = S_open (FALSE, cp, O_RDONLY);
	    break;

	case IOHERE:				/* <<			*/
	    u = OpenHereFile (iop->io_name, C2bool (iop->io_flag & IOEVAL));
	    cp = "here file";
	    break;

	case IORDWR:				/* <>			*/
	    if (CheckForRestrictedShell (cp))
		return TRUE;

	    u = S_open (FALSE, cp, O_RDWR);
	    break;

	case IOCAT:				/* >>			*/
	    if (CheckForRestrictedShell (cp))
		return TRUE;

	    if ((u = S_open (FALSE, cp, O_WRONLY | O_BINARY)) >= 0)
	    {
		lseek (u, 0L, SEEK_END);
		break;
	    }

	case IOWRITE:				/* >			*/
	    if (CheckForRestrictedShell (cp))
		return TRUE;

	    if ((ShellGlobalFlags & FLAGS_NOCLOBER) &&
		(!(iop->io_flag & IOCLOBBER)) &&
		(S_access (CheckDOSFileName (cp), F_OK)))
	    {
		u = -1;
		break;
	    }

	    u = S_open (FALSE, cp, O_CMASK);
	    break;

	case IODUP:				/* >&			*/
	    if (CheckForRestrictedShell (cp))
		return TRUE;

	    u = S_dup2 (*cp - '0', iop->io_unit);
	    break;

	case IOCLOSE:				/* >-			*/
	    if ((iop->io_unit >= STDIN_FILENO) &&
		(iop->io_unit <= STDERR_FILENO))
		S_dup2 (-1, iop->io_unit);

	    S_close (iop->io_unit, TRUE);
	    return FALSE;
    }

    if (u < 0)
    {
	PrintWarningMessage (LIT_3Strings, cp, "cannot ", msg);
	return TRUE;
    }

    else if (u != iop->io_unit)
    {
	S_dup2 (u, iop->io_unit);
	S_close (u, TRUE);
    }

    return FALSE;
}

/*
 * -x flag - echo command to be executed
 */

static void F_LOCAL EchoCurrentCommand (char **wp)
{
    int		i;

    if ((CurrentFunction != (FunctionList *)NULL) && CurrentFunction->Traced)
	fprintf (stderr, "%s:", CurrentFunction->tree->str);

    feputs (GetVariableAsString (PS4, TRUE));

    for (i = 0; wp[i] != NOWORD; i++)
    {
	if (i)
	    feputc (CHAR_SPACE);

	feputs (wp[i]);
    }

    feputc (CHAR_NEW_LINE);
}

/*
 * Set up the status on exit from a command
 */

static int F_LOCAL	SetCommandReturnStatus (int s)
{
    SetVariableFromNumeric (StatusVariable, (long) abs (ExitStatus = s));
    return s;
}

/*
 * Execute a command
 */

int ExecuteACommand (char **argv, int mode)
{
    int		RetVal;

    CheckProgramMode (*argv, &ExecProcessingMode);
    RetVal = EnvironExecute (argv, 0);

    if (ExecProcessingMode.Flags != EP_ENVIRON)
	RetVal = LocalExecve (argv, BuildCommandEnvironment (), mode);

    return RetVal;
}

/*
 * PATH-searching interface to execve.
 */

static int F_LOCAL LocalExecve (char **argv, char **envp, int ForkAction)
{
    int			res = -1;		/* Result		*/
    char		*p_name;		/* Program name		*/
    int			i;

/* Clear the DosExec Failname field */

#if (OS_TYPE == OS_OS2) 
    *FailName = 0;
#endif

#if (OS_TYPE != OS_DOS) 
    OS_DosExecPgmReturnCode = 0;
#endif

/* If the environment is null - It is too big - error */

    if (envp == NOWORDS)
	errno = E2BIG;

    else if ((p_name = AllocateMemoryCell (FFNAME_MAX)) == (char *)NULL)
	errno = ENOMEM;

    else
    {
/* Start off on the search path for the executable file */

	switch (i = FindLocationOfExecutable (p_name, argv[0]))
	{
	    case EXTENSION_EXECUTABLE:
		res = ExecuteProgram (p_name, argv, envp, ForkAction);
		break;

/* Script file */

	    case EXTENSION_BATCH:
	    case EXTENSION_SHELL_SCRIPT:
		res = ExecuteScriptFile (p_name, argv, envp, ForkAction,
					 (bool)(i == EXTENSION_SHELL_SCRIPT));
		break;
	}

	if (res != -1)
	{
	    TrackAllCommands (p_name, *argv);
	    return res;
	}
    }

/* If no fork - exit */

    if (ForkAction & EXEC_WITHOUT_FORK)
    {
	PrintLoadError (*argv);
	FinalExitCleanUp (-1);
    }

    return -1;
}

/*
 * Exec or spawn the program ?
 */

static int F_LOCAL ExecuteProgram (char	*path,
				   char **parms,
				   char **envp,
				   int  ForkAction)
{
    int			res;
#ifdef OS_SWAPPING
    char		*ep;
    unsigned int	size = 0;
    int			serrno;
    unsigned int	c_cur = (unsigned int)(_psp - 1);
    struct MCB_list	*mp = (struct MCB_list *)((unsigned long)c_cur << 16L);
#endif

/* Check we have access to the file */

    if (!S_access (path, F_OK))
	return SetCommandReturnStatus (-1);

/* Process the command line.  If no swapping, we have executed the program */

    res = BuildCommandLine (path, parms, envp, ForkAction);

#if (OS_TYPE != OS_DOS) || (OS_SIZE == OS_32)
    SetWindowName ((char *)NULL);
    ClearExtendedLineFile ();
    return SetCommandReturnStatus (res);

#else
    if ((ExecProcessingMode.Flags & EP_NOSWAP) || (Swap_Mode == SWAP_OFF) ||
	res)
    {
	ClearExtendedLineFile ();
	return SetCommandReturnStatus (res);
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
	    PrintErrorMessage ("Fatal: Memory chain corrupt");
	    return SetCommandReturnStatus (-1);
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
    SW_SBlocks = ((size - etext + _psp - 1) / 0x0400) + 1;

/*
 * Minimum Environment space
 */

    SW_MinESpace = (unsigned int)GetVariableAsNumeric ("ENVIRONMENTSPACE");

/* OK Now we've set up the FCB's, command line and opened the swap file.
 * Get some sys info for the swapper and execute my little assembler
 * function to swap us out
 */

/* Ok - 3 methods of swapping.  Set up the program to execute */

    strcpy (path_line, path);

/* If expanded memory - try that */

    if ((Swap_Mode & SWAP_EXPAND) && Get_EMS_Driver ())
    {
	SW_Mode = 3;			/* Set Expanded memory swap	*/

	if ((res = SwapToMemory (SWAP_EXPAND, envp)) != -2)
	    return CheckForExecOnly (*parms, res,
				     (ForkAction & EXEC_WITHOUT_FORK));
    }

    if ((Swap_Mode & SWAP_EXTEND) && Get_XMS_Driver ())
    {
	SW_Mode = (SW_fp == -1) ? 2 : 4;/* Set Extended memory or XMS driver */

	if ((res = SwapToMemory (SWAP_EXTEND, envp)) != -2)
	    return CheckForExecOnly (*parms, res,
				     (ForkAction & EXEC_WITHOUT_FORK));

	Swap_Mode &= ~SWAP_EXTEND;
    }

/* Try the disk if available */

    if (Swap_Mode & SWAP_DISK)
    {
	SW_Pwrite = 0;

	if (Swap_File == (char *)NULL)
	    SW_fp = S_open (FALSE, (ep = GenerateTemporaryFileName ()),
			    O_SMASK);

	else
	{
	    SW_fp = S_open (FALSE, Swap_File, O_SaMASK);
	    SW_Pwrite = 1;
	}

	if (SW_fp < 0)
	    return SwapToDiskError (ENOSPC, NoSwapFiles);

/* Save the swap file name ? */

	if ((Swap_File == (char *)NULL) &&
	    ((Swap_File = StringSave (ep)) == null))
		Swap_File = (char *)NULL;

	SW_Mode = 1;			/* Set Disk file swap		*/

/* Seek to correct location */

	if (SW_Pwrite)
	{
	    long	loc = (long)(etext - _psp + 1) * 16L;

	    if (lseek (SW_fp, loc, SEEK_SET) != loc)
		return SwapToDiskError (ENOSPC, NoSwapFiles);
	}

/* Execute the program */

	res = SpawnProcess (envp);

/* Close the extended command line file */

	ClearExtendedLineFile ();

/* Check for out of swap space */

	if (res == -2)
	{
	    SwapToDiskError (errno, "Swap file write failed");
	    return CheckForExecOnly (*parms, res,
				     (ForkAction & EXEC_WITHOUT_FORK));
	}

/* Close the swap file */

	serrno = errno;
	S_close (SW_fp, TRUE);
	errno = serrno;

/* Return the result */

	return CheckForExecOnly (*parms, SetCommandReturnStatus (res),
				 (ForkAction & EXEC_WITHOUT_FORK));
    }

/* No swapping available - give up */

    ClearExtendedLineFile ();
    PrintErrorMessage ("All Swapping methods failed");
    Swap_Mode = SWAP_OFF;
    errno = ENOSPC;
    return SetCommandReturnStatus (-1);
#endif
}

/*
 * OS2 and WinNT and DOS 32 do not require swapping
 *
 * Check for Exec only.  We check on exec and do a spawn and then exit
 * to solve some memory allocation problems and because I can't be bothered
 * with complexities of doing an exec in sh0.asm.
 */

#ifdef OS_SWAPPING
static int F_LOCAL CheckForExecOnly (char *pgm, int res, int ExecMode)
{
    if (ExecMode)
    {
	if (res >= 0)
	    FinalExitCleanUp (0);

	PrintLoadError (pgm);
	FinalExitCleanUp (-1);
    }

/* Convert swap error to general error */

    else if (res == -2)
	res = -1;
    
    return res;
}

/*
 * Get the XMS Driver information
 */

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

    or.x.REG_AX = 0x4300;
    SystemInterrupt (0x2f, &or, &or);

    if (or.h.al != 0x80)
    {
	or.x.REG_AX = 0x8800;
	SystemInterrupt (0x15, &or, &or);
	SW_EMsize = or.x.REG_AX / 16;

	if ((SW_EMsize <= SW_Blocks) ||
	     (((long)(SW_EMstart - 0x100000L) +
	      ((long)(SW_Blocks - SW_EMsize) * 16L * 1024L)) < 0L))
	    return XMS_error (MS_Space, 0);

	else
	    return TRUE;
    }

/* Get the driver interface */

    or.x.REG_AX = 0x4310;
    SystemExtendedInterrupt (0x2f, &or, &or, &sr);
    SW_XMS_Driver = (unsigned long)((unsigned long)(sr.es) << 16L |
				    (unsigned long)(or.x.REG_BX));

/* Support for version 3 of XMS driver */

    if ((SW_XMS_Gversion () & 0xff00) < 0x0200)
	return !FL_TEST (FLAG_WARNING)
		? XMS_error ("Warning: %s Version < 2", 0) : FALSE;

    else if (SW_XMS_Available () < (SW_Blocks * 16))
	return !FL_TEST (FLAG_WARNING) ? XMS_error (MS_Space, 0) : FALSE;

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

    or.x.REG_AX = 0x3567;
    DosExtendedInterrupt (&or, &or, &sr);

    sp = (char *)((unsigned long)(sr.es) << 16L | 10L);

/* If not there - disable */

    if (memcmp ("EMMXXXX0", sp, 8) != 0)
	return !FL_TEST (FLAG_WARNING)
		    ? EMS_error ("Warning: %s not available", 0) : FALSE;

    or.h.ah = 0x40;			/* Check status			*/
    SystemInterrupt (0x67, &or, &or);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

/* Check version greater than 3.2 */

    or.h.ah = 0x46;
    SystemInterrupt (0x67, &or, &or);

    if ((or.h.ah != 0) || (or.h.al < 0x32))
	return !FL_TEST (FLAG_WARNING)
		    ? EMS_error ("Warning: %s Version < 3.2", 0) : FALSE;

/*  get page frame address */

    or.h.ah = 0x41;
    SystemInterrupt (0x67, &or, &or);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

    SW_EMSFrame = or.x.REG_BX;		/* Save the page frame		*/

/* Get the number of pages required */

    or.h.ah = 0x43;
    or.x.REG_BX = SW_Blocks;
    SystemInterrupt (0x67, &or, &or);

    if (or.h.ah == 0x088)
	return EMS_error (MS_Space, 0);

    if (or.h.ah != 0)
	return EMS_error (MS_emsg, or.h.ah);

/* Save the EMS Handler */

    SW_fp = or.x.REG_DX;

/* save EMS page map */

    or.h.ah = 0x47;
    or.x.REG_DX = SW_fp;
    SystemInterrupt (0x67, &or, &or);

    return (or.h.ah != 0) ? EMS_error (MS_emsg, or.h.ah) : TRUE;
}

/* Print EMS error message */

static bool F_LOCAL EMS_error (char *s, int v)
{
    PrintWarningMessage (s, "EMS", v);
    Swap_Mode &= ~(SWAP_EXPAND);
    EMS_Close ();
    return FALSE;
}

/* Print XMS error message */

static bool F_LOCAL XMS_error (char *s, int v)
{
    PrintWarningMessage (s, "XMS", v);
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
    or.x.REG_DX = SW_fp;
    SystemInterrupt (0x67, &or, &or);

    if (or.h.ah != 0)
	res = or.h.al;

    or.h.ah = 0x45;
    or.x.REG_DX = SW_fp;
    SystemInterrupt (0x67, &or, &or);

    SW_fp = -1;
    return (res) ? res : or.h.ah;
}
#endif

/* Set up command line.  If the EXTENDED_LINE variable is set, we create
 * a temporary file, write the argument list (one entry per line) to the
 * this file and set the command line to @<filename>.  If NOSWAPPING, we
 * execute the program because I have to modify the argument line
 */

static int F_LOCAL BuildCommandLine (char *path,
				     char **argv,
				     char **envp,
				     int  ForkAction)
{
    char		**pl = argv;
    FILE		*fd;
    bool		found;
    char		*new_args[3];
#ifndef OS_SWAPPING
    char		cmd_line[FFNAME_MAX];
#endif

/* Translate process name to MSDOS format */

    if ((GenerateFullExecutablePath (path) == (char *)NULL) ||
	(!GetApplicationType (path)))
	return -1;

/* If this is a windows program - special */

#if (OS_TYPE != OS_UNIX)
    if ((ApplicationType == EXETYPE_DOS_GUI) && (BaseOS != BASE_OS_NT))
        return ExecuteWindows (path, argv, envp, ForkAction);
#endif

/* Extended command line processing */

    Extend_file = (char *)NULL;		/* Set no file		*/
    found = C2bool ((ExecProcessingMode.Flags & EP_UNIXMODE) ||
		    (ExecProcessingMode.Flags & EP_DOSMODE));

/* Set up a blank command line */

    cmd_line[0] = 0;
    cmd_line[1] = CHAR_RETURN;

/* If there are no parameters, or they fit in the DOS command line
 * - start the process */

    if ((*(++pl) == (char *)NULL) || CheckParameterLength (pl))
	return StartTheProcess (path, argv, envp, ForkAction);

/* If we can use an alternative approach - indirect files, use it */

    else if (found)
    {
	char	**pl1 = pl;

/* Check parameters don't contain a re-direction parameter */

	while (*pl1 != NOWORD)
	{
	    if (**(pl1++) == CHAR_INDIRECT)
	    {
		found = FALSE;
		break;
	    }
	}

/* If we find it - create a temporary file and write the stuff */

	if ((found) &&
	    ((fd = FOpenFile (Extend_file = GenerateTemporaryFileName (),
			      sOpenWriteMode)) != (FILE *)NULL))
	{
	    if ((Extend_file = StringSave (Extend_file)) == null)
		Extend_file = (char *)NULL;

/* Copy to end of list */

	    do
	    {
		if (!WriteToExtendedFile (fd, *pl))
		    return -1;
	    } while (*(pl++) != NOWORD);

/* Set up cmd_line[1] to contain the filename */

#ifdef OS_SWAPPING
	    memset (cmd_line, 0, CMD_LINE_MAX);
#else
	    memset (cmd_line, 0, FFNAME_MAX);
#endif
	    cmd_line[1] = CHAR_SPACE;
	    cmd_line[2] = CHAR_INDIRECT;
	    strcpy (&cmd_line[3], Extend_file);
	    cmd_line[0] = (char)(strlen (Extend_file) + 2);

/* Correctly terminate cmd_line in no swap mode */

#ifdef OS_SWAPPING
	    if (!(ExecProcessingMode.Flags & EP_NOSWAP) &&
		(Swap_Mode != SWAP_OFF))
		cmd_line[cmd_line[0] + 2] = CHAR_RETURN;
#endif

/* If the name in the file is in upper case - use \ for separators */

	    if (ExecProcessingMode.Flags & EP_DOSMODE)
		PATH_TO_DOS (&cmd_line[2]);

/* OK we are ready to execute */

#ifdef OS_SWAPPING
	    if ((ExecProcessingMode.Flags & EP_NOSWAP) ||
		(Swap_Mode == SWAP_OFF) || (ForkAction & EXEC_WITHOUT_FORK))
	    {
#endif
		new_args[0] = *argv;
		new_args[1] = &cmd_line[2];
		new_args[2] = (char *)NULL;

		return StartTheProcess (path, new_args, envp, ForkAction);
#ifdef OS_SWAPPING
	    }

	    else
		return 0;
#endif
	}
    }

    return -1;
}

/*
 * Clear Extended command line file
 */

void ClearExtendedLineFile (void)
{
    if (Extend_file != (char *)NULL)
    {
	unlink (Extend_file);
	ReleaseMemoryCell ((void *)Extend_file);
    }

    Extend_file = (char *)NULL;
}

/*
 * Clear Disk swap file file
 */

#ifdef OS_SWAPPING
void ClearSwapFile (void)
{
    if (SW_fp >= 0)
	S_close (SW_fp, TRUE);

    if (Swap_File != (char *)NULL)
    {
	unlink (Swap_File);
	ReleaseMemoryCell ((void *)Swap_File);
    }

    SW_fp = -1;
    Swap_File = (char *)NULL;
}
#endif

/*
 * Convert the executable path to the full path name
 */

char	*GenerateFullExecutablePath (char *path)
{
    char		cpath[PATH_MAX + 6];
    char		npath[FFNAME_MAX + 2];
    char		n1path[PATH_MAX + 6];
    char		*p;
    int			drive;

    PATH_TO_UPPER_CASE (path);

/* Get the current path */

    S_getcwd (cpath, 0);
    strcpy (npath, cpath);

/* In current directory ? */

    if ((p = FindLastPathCharacter (path)) == (char *)NULL)
    {
	 p = path;

/* Check for a:program case */

	 if (IsDriveCharacter (*(p + 1)))
	 {
	    p += 2;

/* Get the path of the other drive */

	    S_getcwd (npath, GetDriveNumber (*path));
	 }
    }

/* In root directory */

    else if ((p - path) == 0)
    {
	++p;
	strcpy (npath, RootDirectory);
	*npath = *path;
	*npath = *cpath;
    }

    else if (((p - path) == 2) && IsDriveCharacter (*(path + 1)))
    {
	++p;
	strcpy (npath, RootDirectory);
	*npath = *path;
    }

/* Find the directory */

    else
    {
	*(p++) = 0;

/* Change to the directory containing the executable */

	drive = IsDriveCharacter (*(path + 1))
			? GetDriveNumber (*path)
#if (OS_TYPE == OS_OS2) && !defined (__WATCOMC__)
			: _getdrive ();
#else
			: 0;
#endif

/* Save the current directory on this drive */

	S_getcwd (n1path, drive);

/* Find the directory we want */

	if (!S_chdir (path))
	    return (char *)NULL;

	S_getcwd (npath, drive);		/* Save its full name */
	S_chdir (n1path);			/* Restore the original */

/* Restore our original directory */

	if (!S_chdir (cpath))
	    return (char *)NULL;
    }

    if (!IsPathCharacter (npath[strlen (npath) - 1]))
	strcat (npath, DirectorySeparator);

    strcat (npath, p);
    return PATH_TO_DOS (strcpy (path, npath));
}

/*
 * Find the number of values to use for a for or select statement
 */

static char ** F_LOCAL FindNumberOfValues (char **wp, int *Count)
{

/* select/for x do...done - use the parameter values.  Need to know how many as
 * it is not a NULL terminated array
 */

    if (wp == NOWORDS)
    {
	if ((*Count = ParameterCount) < 0)
	    *Count = 0;

	return ParameterArray + 1;
    }

/* select/for x in y do...done - find the start of the variables and
 * use them all
 */

    *Count = CountNumberArguments (wp);

    return wp;
}

/*
 * Count the number of entries in an array
 */

int	CountNumberArguments (char **wp)
{
    int		Count = 0;

    while (*(wp++) != NOWORD)
	Count++;

    return Count;
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
	if (CloseFile (fd) != EOF)
	    return TRUE;

	WriteOk = FALSE;
    }

    else if (strlen (string))
    {

/* Write the string, converting newlines to backslash newline */

	while (WriteOk && (cp != (char *)NULL))
	{
	    if ((cp = strchr (sp, CHAR_NEW_LINE)) != (char *)NULL)
		*cp = 0;

	    if (fputs (sp, fd) == EOF)
		WriteOk = FALSE;

	    else if (cp != (char *)NULL)
		WriteOk = C2bool (fputs ("\\\n", fd) != EOF);

	    sp = cp + 1;
	}
    }

    if (WriteOk && (fputc (CHAR_NEW_LINE, fd) != EOF))
	return TRUE;

    CloseFile (fd);
    ClearExtendedLineFile ();
    errno = ENOSPC;
    return FALSE;
}

/*
 * Execute or spawn the process
 */

#ifdef OS_SWAPPING
static int F_LOCAL StartTheProcess (char *path, char **argv, char **envp,
				    int ForkAction)
{
    if (ForkAction & EXEC_WITHOUT_FORK)
    {
	DumpHistory ();				/* Exec - save history */
	ProcessSpaceInParameters (argv);
	return 0;
    }

    return ((ExecProcessingMode.Flags & EP_NOSWAP) || (Swap_Mode == SWAP_OFF))
		? spawnve (P_WAIT, path, ProcessSpaceInParameters (argv), envp)
		: 0;
}
#endif

/* DOS, 32 bit version */

#if (OS_TYPE == OS_DOS) && (OS_SIZE == OS_32)
static int F_LOCAL StartTheProcess (char *path, char **argv, char **envp,
				    int ForkAction)
{
    int		retval;

    if (ForkAction & EXEC_WITHOUT_FORK)
    {
	DumpHistory ();				/* Exec - save history */

	if ((retval = spawnve (P_WAIT, path, ProcessSpaceInParameters (argv),
		    	       envp)) != -1)
	    exit (retval);

	PrintErrorMessage ("unexpected return from exec (%d)", errno);
	return retval;
    }

    return spawnve (P_WAIT, path, ProcessSpaceInParameters (argv), envp);
}
#endif

/* OS/2 Version */

#if (OS_TYPE == OS_OS2)
static int F_LOCAL StartTheProcess (char *path, char **argv, char **envp,
				    int ForkAction)
{
    int			RetVal;
    STARTDATA		stdata;

/* Is this a start session option */

    if (SessionControlBlock != (STARTDATA *)NULL)
	return StartTheSession (SessionControlBlock, path, argv, envp,
				ForkAction);

/* Exec ? */

    if (ForkAction & EXEC_WITHOUT_FORK)
	return OS_DosExecProgram (OLD_P_OVERLAY, path, argv, envp);

    if (ForkAction & (EXEC_SPAWN_DETACH | EXEC_SPAWN_NOWAIT |
		      EXEC_SPAWN_IGNOREWAIT))
    {
	int	Mode = (ForkAction & EXEC_SPAWN_DETACH)
			? P_DETACH
			: ((ForkAction & EXEC_SPAWN_IGNOREWAIT)
			   ? P_NOWAITO
			   : (FL_TEST (FLAG_SEPARATE_GROUP)
			     ? P_DETACH
 			     : P_NOWAIT));

	RetVal = OS_DosExecProgram (Mode, path, argv, envp);

/* Remove the reference to the temporary file for background tasks */

	ReleaseMemoryCell ((void *)Extend_file);
	Extend_file = (char *)NULL;

	if ((RetVal != -1) && (Mode != P_NOWAITO))
	{
	    if (InteractiveFlag && (source->type == STTY))
	    {
	        if (ForkAction & EXEC_SPAWN_DETACH)
		    fprintf (stderr, "[0] Process %d detached\n", RetVal);

		else
		{
		    PidInfo.Valid = TRUE;
		    PidInfo.JobNo = AddNewJob (RetVal, 0, path);
		    PidInfo.PidNo = RetVal;
		}
	    }

	    SetVariableFromNumeric ("!", RetVal);
	    return 0;
	}

	return RetVal;
    }

/* In OS/2, we need the type of the program because PM programs have to be
 * started in a session (or at least that was the only way I could get them
 * to work).
 */

/* Do we need to start a new session for this program ? */

    if (((ApplicationType & EXETYPE_OS2_TYPE) == EXETYPE_OS2_GUI) ||
	(ApplicationType & EXETYPE_DOS))
    {
	if ((OS_VERS_N > 1) && (ApplicationType & EXETYPE_DOS))
	    memcpy (&stdata, &DOS_SessionControlBlock, sizeof (STARTDATA));

	else
	    memcpy (&stdata, &PM_SessionControlBlock, sizeof (STARTDATA));

	if (ForkAction & EXEC_WINDOWS)
	    stdata.PgmControl = 0;

	return StartTheSession (&stdata, path, argv, envp, ForkAction);
    }

/* Just DosExecPgm it */

    return OS_DosExecProgram (P_WAIT, path, argv, envp);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static int F_LOCAL StartTheProcess (char *path, char **argv, char **envp,
				    int ForkAction)
{
    int			RetVal;

/* Exec ? */

    if (ForkAction & EXEC_WITHOUT_FORK)
	return OS_DosExecProgram (OLD_P_OVERLAY, path, argv, envp);

    if (ForkAction & (EXEC_SPAWN_DETACH | EXEC_SPAWN_NOWAIT |
		      EXEC_SPAWN_IGNOREWAIT))
    {
	int	Mode = (ForkAction & EXEC_SPAWN_DETACH)
			? P_DETACH
			: ((ForkAction & EXEC_SPAWN_IGNOREWAIT)
			   ? P_NOWAITO
			   : (FL_TEST (FLAG_SEPARATE_GROUP)
			     ? P_DETACH
 			     : P_NOWAIT));

	RetVal = OS_DosExecProgram (Mode, path, argv, envp);

/* Remove the reference to the temporary file for background tasks */

	ReleaseMemoryCell ((void *)Extend_file);
	Extend_file = (char *)NULL;

	if ((RetVal != -1) && (Mode != P_NOWAITO))
	{
	    if (InteractiveFlag && (source->type == STTY))
	    {
	        if (ForkAction & EXEC_SPAWN_DETACH)
		    fprintf (stderr, "[0] Process %d detached\n", RetVal);

		else
		{
		    PidInfo.Valid = TRUE;
		    PidInfo.JobNo = AddNewJob (RetVal, 0, path);
		    PidInfo.PidNo = RetVal;
		}
	    }

	    SetVariableFromNumeric ("!", RetVal);
	    return 0;
	}

	else
	    return RetVal;
    }

/* Just DosExecPgm it */

    return OS_DosExecProgram (P_WAIT, path, argv, envp);
}
#endif

/*
 * Start a session
 */

#if (OS_TYPE == OS_OS2)
static int F_LOCAL StartTheSession (STARTDATA	*SessionData,
				    char	*path,
				    char	**argv,
				    char	**envp,
				    int		ForkAction)
{
    OSCALL_PARAM	usType;
    OSCALL_PARAM	idSession;
#  if (OS_SIZE == OS_32)
    PID			pid;
#  else
    USHORT		pid;
#  endif

/*
 * For OS/2 2.x, we can start DOS sessions!!
 */

    if ((OS_VERS_N > 1) && (ApplicationType & EXETYPE_DOS))
    {
	if ((ForkAction & EXEC_WINDOWS) ||
	    (SessionData->SessionType == SSF_TYPE_FULLSCREEN))
	    SessionData->SessionType = SSF_TYPE_VDM;

	else
	    SessionData->SessionType = SSF_TYPE_WINDOWEDVDM;

	if (SessionData->Environment == (PBYTE)1)
	    SessionData->Environment = (PBYTE)NULL;
    }

    else if ((ApplicationType & EXETYPE_OS2_TYPE) == EXETYPE_OS2_GUI)
	SessionData->SessionType = SSF_TYPE_PM;

    SessionData->PgmName = path;
    SessionData->TermQ = SessionEndQName;	/* Queue name		*/

    /*
     * Build the environment if the current value is 1
     */

    if ((SessionData->Environment == (PBYTE)1) &&
	((SessionData->Environment = (PBYTE)BuildOS2String (envp, 0))
				   == (PBYTE)NULL))
	return -1;

    ProcessSpaceInParameters (argv);

    if ((SessionData->PgmInputs = (PBYTE)BuildOS2String (&argv[1], CHAR_SPACE))
    				== (PBYTE)NULL)
	return -1;

/*
 * Start the session.  Do not record if it is independent
 */

    DISABLE_HARD_ERRORS;
#  if (OS_SIZE == OS_16) && !defined (OS2_16_BUG)
    SessionData->Related = SSF_RELATED_INDEPENDENT;
#  endif
    usType = DosStartSession (SessionData, &idSession, &pid);
    ENABLE_HARD_ERRORS;

    if ((!usType) || (usType == ERROR_SMG_START_IN_BACKGROUND))
    {
	if (InteractiveFlag && (source->type == STTY))
	{
	    if (SessionData->Related == SSF_RELATED_INDEPENDENT)
		fprintf (stderr, "[0] Independent Session %d started\n",
			 idSession);

	    else
		fprintf (stderr, "[%d] Session %d (PID %d) started\n",
			 AddNewJob (pid, idSession, path), idSession, pid);

	    if (usType)
		fprintf (stderr, "%s\n", GetOSSystemErrorMessage (usType));
	}

        SetVariableFromNumeric ("!", pid);
	return 0;
    }

    else
    {
	strcpy (FailName, GetOSSystemErrorMessage (usType));

	errno = ENOENT;
	return -1;
    }
}
#endif

/*
 * Build the OS2 format <value>\0<value>\0 etc \0
 */

char	*BuildOS2String (char **Array, char sep)
{
    int		i = 0;
    int		Length = 0;
    char	*Output;
    char	*sp, *cp;

/* Find the total data length */

    while ((sp = Array[i++]) != NOWORD)
	Length += strlen (sp) + 1;

    Length += 2;

    if ((Output = AllocateMemoryCell (Length)) == (char *)NULL)
	return (char *)NULL;

/* Build the new string */

    i = 0;
    sp = Output;

/* Build the string */

    while ((cp = Array[i++]) != NOWORD)
    {
	while ((*sp = *(cp++)))
	    ++sp;

	if (!sep || (Array[i] != NOWORD))
	    *(sp++) = sep;
    }

    *sp = 0;
    return Output;
}


/*
 * List of default known extensions and their type
 */
 
#define MAX_DEFAULT_EXTENSIONS	ARRAY_SIZE (DefaultExtensions)

static struct ExtType {
    char	*Extension;
    char	Type;
} DefaultExtensions [] = {
    { null,		EXTENSION_SHELL_SCRIPT },
    { SHELLExtension,	EXTENSION_SHELL_SCRIPT },
    { KSHELLExtension,	EXTENSION_SHELL_SCRIPT },
#if (OS_TYPE != OS_UNIX)
    { BATExtension,	EXTENSION_BATCH },
    { EXEExtension,	EXTENSION_EXECUTABLE },
    { COMExtension,	EXTENSION_EXECUTABLE },
#endif
};

/*
 * Built the list of valid extensions
 */

void	BuildExtensionLists (void)
{
    Word_B	*DList = (Word_B *)NULL;
    Word_B	*FList = (Word_B *)NULL;
    int		i;
    char	*pe;
    char	*sp;
    void		(*save_signal)(int);

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Release Old memory */

    if (ExecuteFunctionList != (char **)NULL)
    {
	ReleaseMemoryCell (*ExecuteFunctionList);
	ReleaseMemoryCell (ExecuteFunctionList);
    }

    if (ExecutableList != (char **)NULL)
	ReleaseMemoryCell (ExecutableList);

/* No extensions ? */

    if ((pe = GetVariableAsString (PathExtsLiteral, FALSE)) == null)
    {
	for (i = 0; i < MAX_DEFAULT_EXTENSIONS; i++)
	    DList = AddWordToBlock (DefaultExtensions [i].Extension, DList);

	ExecutableList = GetWordList (AddWordToBlock ((char *)NULL, DList));
	ExecuteFunctionList = (char **)NULL;

/* Restore signals */

	signal (SIGINT, save_signal);

	return;
    }

/* OK - scan */

    pe = StringSave (pe);

/* Split out on semi-colons */

    do
    {
	if ((sp = strchr (pe, ';')) != (char *)NULL)
	    *(sp++) = 0;

	FList = AddWordToBlock (pe, FList);

/* Build the new order of default extensions */

	for (i = 0; i < MAX_DEFAULT_EXTENSIONS; i++)
	{
	    if (!NOCASE_COMPARE (pe, DefaultExtensions [i].Extension))
	    {
		DList = AddWordToBlock (pe, DList);
		break;
	    }
	}

	pe = sp;
    } while (pe != (char *)NULL);

/* Set up the two lists */

    ExecutableList = GetWordList (AddWordToBlock ((char *)NULL, DList));
    ExecuteFunctionList = GetWordList (AddWordToBlock ((char *)NULL, FList));

/* Restore signals */

    signal (SIGINT, save_signal);
}

/*
 * Find the location of an executable and return it's full path
 * name
 */

int	FindLocationOfExecutable (char *FullPath,
				  char *name)
{
    return ExtensionType (FindFileAndExtension (FullPath, name,
						ExecutableList));
}

/*
 * Return the extension type for an extension
 */

static int F_LOCAL ExtensionType (char *ext)
{
    int		i;

    if (ext == (char *)NULL)
	return EXTENSION_NOT_FOUND;
    
    for (i = 0; i < MAX_DEFAULT_EXTENSIONS; i++)
    {
	if (!NOCASE_COMPARE (ext, DefaultExtensions [i].Extension))
	    return (int)(DefaultExtensions [i].Type);
    }

    return EXTENSION_OTHER;
}

/*
 * Search the path for a file with the appropriate extension
 *
 * Returns a pointer to the extension.
 */

static char * F_LOCAL FindFileAndExtension (char *FullPath,
					    char *name,
					    char **Extensions)
{
    char	*sp;			/* Path pointers	*/
    char	*ep;
    char	*xp;			/* In file name pointers */
    char	*xp1;
    int		i, fp;
    int		pathlen;

/* No extensions - no file */

    if (Extensions == (char **)NULL)
    {
	errno = ENOENT;
	return (char *)NULL;
    }

/* Scan the path for an executable */

    sp = ((FindPathCharacter (name) != (char *)NULL) ||
	  (IsDriveCharacter (*(name + 1))))
	      ? null
	      : GetVariableAsString (PathLiteral, FALSE);

    do
    {
	sp = BuildNextFullPathName (sp, name, FullPath);
	ep = &FullPath[pathlen = strlen (FullPath)];

/* Get start of file name */

	if ((xp1 = FindLastPathCharacter (FullPath)) == (char *)NULL)
	    xp1 = FullPath;

	else
	    ++xp1;

/* Look up all 5 types */

	for (i = 0; Extensions[i] != (char *)NULL; i++)
	{
	    if (pathlen + strlen (Extensions[i]) > (size_t)(FFNAME_MAX - 1))
	        continue;

	    strcpy (ep, Extensions[i]);

	    if (S_access (FullPath, F_OK))
	    {

/* If no extension, .ksh or .sh extension, check for shell script */

		if (((xp = strrchr (xp1, CHAR_PERIOD)) == (char *)NULL) ||
		    (NOCASE_COMPARE (xp, SHELLExtension) == 0) ||
		    (NOCASE_COMPARE (xp, KSHELLExtension) == 0))
		{
		    if ((fp = CheckForScriptFile (FullPath, (char **)NULL,
						  (int *)NULL)) < 0)
			continue;

		    S_close (fp, TRUE);
		    return (xp == (char *)NULL) ? null : xp;
		}

		return xp;
	    }
	}
    } while (sp != (char *)NULL);

/* Not found */

    errno = ENOENT;
    return (char *)NULL;
}

/*
 * Execute a script file
 */

static int F_LOCAL ExecuteScriptFile (char *Fullpath,
				      char **argv,
				      char **envp,
				      int  ForkAction,
				      bool ShellScript)
{
    int		res;			/* Result		*/
    char	*params;		/* Script parameters	*/
    int		nargc = 0;		/* # script args	*/
    Word_B	*wb = (Word_B *)NULL;
#if (OS_TYPE == OS_DOS)
    union REGS	r;
#endif

/* Batfile - convert to DOS Format file name */

    if (!ShellScript)
    {
	PATH_TO_DOS (Fullpath);
	nargc = 0;
    }

    else if ((res = OpenForExecution (Fullpath, &params, &nargc)) >= 0)
	S_close (res, TRUE);

    else
    {
	errno = ENOENT;
	return -1;
    }

/* If BAT file, use command.com else use sh */

    if (!ShellScript)
    {
	if (!SetUpCLI (GetVariableAsString (ComspecVariable, FALSE), &wb))
	    return -1;
	
	wb = AddWordToBlock ("/c", wb);

/* Get the switch character */

#if (OS_TYPE == OS_DOS) && (OS_SIZE == OS_16)
	r.x.REG_AX = 0x3700;
	DosInterrupt (&r, &r);

	if ((r.h.al == 0) && (_osmajor < 4))
	    *(wb->w_words[wb->w_nword - 1]) = (char)(r.h.dl);
#endif
    }

/* Stick in the pre-fix arguments */

    else if (nargc)
	wb = SplitString (params, wb);

    else if (params != null)
	wb = AddWordToBlock (params, wb);

    else
	wb = AddWordToBlock (GetVariableAsString (ShellVariableName, FALSE),
			     wb);

/* Add the rest of the parameters, and execute */

    res = ExecuteSpecialProcessor (Fullpath, argv, envp, ForkAction, wb);

/* Release allocated space */

    if (params != null)
	ReleaseMemoryCell ((void *)params);

    return res;
}

/*
 * Convert errno to error message on execute
 */

static char * F_LOCAL ConvertErrorNumber (void)
{
    switch (errno)
    {
	case ENOMEM:
	    return strerror (ENOMEM);

	case ENOEXEC:
	    return "program corrupt";

	case E2BIG:
	    return AE2big;

	case ENOENT:
	    return NotFound;

	case 0:
	    return "No Shell";
    }

    return "cannot execute";
}

/*
 * Swap to disk error
 */

#ifdef OS_SWAPPING
static int F_LOCAL SwapToDiskError (int error, char *ErrorMessage)
{

/* Clean up */

    ClearSwapFile ();
    Swap_Mode &= (~SWAP_DISK);
    PrintErrorMessage (ErrorMessage);
    errno = error;
    return SetCommandReturnStatus (-1);
}

/*
 * Swap to memory
 */

static int F_LOCAL	SwapToMemory (int mode, char **envp)
{
    int		res;
    int		cr;

/* Swap and close memory handler */

    res = SpawnProcess (envp);

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
	return SetCommandReturnStatus (res);
    }

/* Failed - disabled */

    Swap_Mode &= (~mode);
    return res;
}
#endif

/*
 * Check the program type
 */

void CheckProgramMode (char *Pname, ExeMode *PMode)
{
    char		*sp, *sp1;		/* Line pointers	*/
    int			nFields;
    char		*SPname;
    int			builtin;		/* Builtin function	*/
    LineFields		LF;
    long		value;

/* Check for internal no-globbed commands */

    if ((IsCommandBuiltIn (Pname, &builtin) != (int (*)(int, char **))NULL) &&
	((builtin & BLT_SKIPGLOB) == BLT_SKIPGLOB))
    {
	PMode->Flags = EP_NOEXPAND | ((builtin & BLT_NOWORDS) ? EP_NOWORDS : 0);
	return;
    }

/* Set not found */

    PMode->Flags = EP_NONE;

/* Check not a function */

    if ((Pname == (char *)NULL) ||
	((sp = GetVariableAsString ("EXTENDED_LINE", FALSE)) == null))
        return;

/* Get some memory for the input line and the file name */

    sp1 = ((sp1 = FindLastPathCharacter (Pname)) == (char *)NULL)
		 ? Pname : sp1 + 1;

    if (IsDriveCharacter (*(sp1 + 1)))
	sp1 += 2;

    if ((SPname = StringCopy (sp1)) == null)
        return;

    if ((LF.Line = AllocateMemoryCell (LF.LineLength = 200)) == (char *)NULL)
    {
	ReleaseMemoryCell ((void *)SPname);
	return;
    }

/* Remove terminating .exe etc */

    if ((sp1 = strrchr (SPname, CHAR_PERIOD)) != (char *)NULL)
        *sp1 = 0;

/* Open the file */

    if ((LF.FP = FOpenFile (sp, sOpenReadMode)) == (FILE *)NULL)
    {
	ReleaseMemoryCell ((void *)LF.Line);
	ReleaseMemoryCell ((void *)SPname);
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

#if (OS_TYPE != OS_UNIX)
	if ((sp = strrchr (LF.Fields->w_words[0], CHAR_PERIOD)) != (char *)NULL)
	    *sp = 0;
#endif

        if (NOCASE_COMPARE (LF.Fields->w_words[0], SPname))
            continue;

/* What type? */

	if (NOCASE_COMPARE (LF.Fields->w_words[1], "unix") == 0)
	    PMode->Flags = (unsigned int )(EP_UNIXMODE |
				CheckForCommonOptions (&LF, 2));

	else if (NOCASE_COMPARE (LF.Fields->w_words[1], LIT_dos) == 0)
	    PMode->Flags = (unsigned int )(EP_DOSMODE |
				CheckForCommonOptions (&LF, 2));

/* Must have a valid name and we can get memory for it */

	else if ((NOCASE_COMPARE (LF.Fields->w_words[1], "environ") == 0) &&
		 (nFields >= 3) &&
		 (!IsValidVariableName (LF.Fields->w_words[2])) &&
		 ((PMode->Name =
		     StringCopy (LF.Fields->w_words[2])) != null))
	{
	    PMode->Flags = EP_ENVIRON;
	    PMode->FieldSep = 0;

	    if ((nFields >= 4) &&
		ConvertNumericValue (LF.Fields->w_words[3], &value, 0))
		PMode->FieldSep = (unsigned char)value;

	    if (!PMode->FieldSep)
		PMode->FieldSep = CHAR_SPACE;
	}

	else
	    PMode->Flags = CheckForCommonOptions (&LF, 1);

        break;
    }

    CloseFile (LF.FP);
    ReleaseMemoryCell ((void *)LF.Line);
    ReleaseMemoryCell ((void *)SPname);
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
	    if (!NOCASE_COMPARE (LF->Fields->w_words[i], CommonFields[j].Name))
	    {
		Flags |= CommonFields[j].Flag;
		break;
	    }
	}
    }

    return Flags;
}

/*
 * Convert UNIX format lines to DOS format if appropriate.
 * Build Environment variable for some programs.
 */

static int F_LOCAL EnvironExecute (char **argv, int ForkAction)
{
    int		s_errno;
    int		RetVal = 1;
    char	*NewArgs[3];
    char	*cp;

/* If this command does not pass the command string in the environment,
 * no action required
 */

    if (ExecProcessingMode.Flags != EP_ENVIRON)
        return 0;

    if ((cp = BuildOS2String (&argv[1], ExecProcessingMode.FieldSep))
		 == (char *)NULL)
    {
	ExecProcessingMode.Flags = EP_NONE;
        return 0;
    }

    SetVariableFromString (ExecProcessingMode.Name, cp);
    SetVariableStatus (ExecProcessingMode.Name, STATUS_EXPORT);

/* Build and execute the environment */

    NewArgs[0] = argv[0];
    NewArgs[1] = ExecProcessingMode.Name;
    NewArgs[2] = (char *)NULL;

    RetVal = LocalExecve (NewArgs, BuildCommandEnvironment (), ForkAction);
    s_errno = errno;
    UnSetVariable (ExecProcessingMode.Name, -1, FALSE);
    errno = s_errno;
    return RetVal;
}

/*
 * Set Interrupt handling vectors - moved from sh0.asm
 */

#ifdef OS_SWAPPING
static int F_LOCAL	SpawnProcess (char **envp)
{
    void	(interrupt far *SW_I00_V) (void);	/* Int 00 address */
    void	(interrupt far *SW_I23_V) (void);	/* Int 23 address*/
    int			res;
#if 0
    union REGS		r;
    unsigned char	Save;

    r.x.REG_AX = 0x3300;
    DosInterrupt (&r, &r);
    Save = r.h.al;
    fprintf (stderr, "Break Status: %s (%u)\n", Save ? "on" : "off", Save);

    r.x.REG_AX = 0x3301;
    r.h.dl = 1;
    DosInterrupt (&r, &r);
    fprintf (stderr, "Break Status: %s (%u)\n", r.h.al ? "on" : "off", r.h.al);
#endif

/*
 * Save current vectors
 */

    SW_I00_V = GetInterruptVector (0x00);
    SW_I23_V = GetInterruptVector (0x23);

/*
 * Set In shell flag for Interrupt 23, and set to new interrupts
 */

    SW_I23_InShell = 0;

    SetInterruptVector (0x23, SW_Int23);
    SetInterruptVector (0x00, SW_Int00);

    res = SA_spawn (envp);

/*
 * Restore interrupt vectors
 */

    SetInterruptVector (0x00, SW_I00_V);
    SetInterruptVector (0x23, SW_I23_V);

#if 0
    r.x.REG_AX = 0x3300;
    DosInterrupt (&r, &r);
    fprintf (stderr, "Break Status: %s (%u)\n", r.h.al ? "on" : "off", r.h.al);
    r.x.REG_AX = 0x3301;
    r.h.dl = Save;
    DosInterrupt (&r, &r);
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
	    if (strpbrk (*(argv++), WildCards) != (char *)NULL)
		return FALSE;
	}
    }

/*
 * Do any parameter conversion - adding quotes or backslashes, but don't
 * update argv.
 */

    CmdLineLength = CountNumberArguments (argv = SavedArgs);

    if ((SavedArgs = (char **)
		AllocateMemoryCell ((CmdLineLength + 1) * sizeof (char *)))
		== (char **)NULL)
	return FALSE;

/* Save a copy of the argument addresses */

    memcpy (SavedArgs, argv, (CmdLineLength + 1) * sizeof (char *));

/* Build the command line */

    if ((CommandLine = BuildOS2String (ProcessSpaceInParameters (SavedArgs),
					    CHAR_SPACE)) == (char *)NULL)
    {
	ReleaseMemoryCell (SavedArgs);
	return FALSE;
    }

/*
 * Check command line length. Under DOS, this is simple.  On OS/2, we have
 * to remember that we default to the EMX interface, which requires
 * twice as much space, plus some nulls.  Also remember start DOS programs
 * on OS/2 or NT have restricted space.
 */

    CmdLineLength = strlen (CommandLine);

    if ((ApplicationType & EXETYPE_DOS) &&
	(CmdLineLength >= DOS_CMD_LINE_MAX - 2))
    {
	errno = E2BIG;
	RetVal = FALSE;
    }

#if (OS_TYPE == OS_OS2)
    else if (CmdLineLength >= (((CMD_LINE_MAX - 2) / 2) +
				  CountNumberArguments (SavedArgs) + 3))
    {
	errno = E2BIG;
	RetVal = FALSE;
    }

#elif (OS_TYPE == OS_NT) 
    else if (CmdLineLength >= CMD_LINE_MAX - 2)
    {
	errno = E2BIG;
	RetVal = FALSE;
    }
#endif

/* Terminate the line */

    else
    {
#ifdef OS_SWAPPING
	strcpy (cmd_line + 1, CommandLine);
	cmd_line[CmdLineLength + 1] = CHAR_RETURN;
	cmd_line[0] = (char)CmdLineLength;
#endif
	RetVal = TRUE;
    }

    ReleaseMemoryCell (SavedArgs);
    ReleaseMemoryCell (CommandLine);

    return RetVal;
}

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
        if ((strpbrk (*argv, " \t") != (char *)NULL) ||
	    (strlen (*argv) == 0) ||
	    ((ExecProcessingMode.Flags & EP_QUOTEWILD) &&
	     (strpbrk (*argv, WildCards) != (char *)NULL)))
	{

/* Count number of Double quotes in the parameter */

	    Count = CountDoubleQuotes (*argv);

/* Get some memory - give up update if out of memory */

	    if ((new = GetAllocatedSpace (strlen (*argv) + (Count * 2) +
					  3)) == (char *)NULL)
	        return Start;

	    SetMemoryAreaNumber ((void *)new,
			         GetMemoryAreaNumber ((void *)*argv));
	    *new = CHAR_DOUBLE_QUOTE;

/* Escape any double quotes in the string */

	    cp = *argv;
	    sp = new + 1;

	    while (*cp)
	    {
		if (*cp == CHAR_DOUBLE_QUOTE)
		{
		    *(sp++) = CHAR_META;
		    *(sp++) = *(cp++);
		}

		else if (*cp != CHAR_META)
		    *(sp++) = *(cp++);

/* Handle escapes - count them */

		else
		{
		    *(sp++) = *(cp++);

		    if (*cp == CHAR_DOUBLE_QUOTE)
		    {
			*(sp++) = CHAR_META;
			*(sp++) = CHAR_META;
		    }

		    else if (*cp == 0)
		    {
			*(sp++) = CHAR_META;
			break;
		    }

		    *(sp++) = *(cp++);
		}
	    }

/* Append the terminating double quotes */

	    strcpy (sp, DoubleQuotes);
	    *argv = new;
	}

/* Check for any double quotes */

	else if ((Count = CountDoubleQuotes (*argv)))
	{

/* Got them - escape them */

	    if ((new = GetAllocatedSpace (strlen (*argv) + Count + 1))
			== (char *)NULL)
	        return Start;

	    SetMemoryAreaNumber ((void *)new,
			         GetMemoryAreaNumber ((void *)*argv));

/* Copy the string, escaping DoubleQuotes */

	    cp = *argv;
	    sp = new;

	    while ((*sp = *(cp++)))
	    {
		if (*sp == CHAR_DOUBLE_QUOTE)
		{
		    *(sp++) = CHAR_META;
		    *sp = CHAR_DOUBLE_QUOTE;
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
 * Count DoubleQuotes
 */

static int F_LOCAL CountDoubleQuotes (char *string)
{
    int		Count = 0;

    while ((string = strchr (string, CHAR_DOUBLE_QUOTE)) != (char *)NULL)
    {
	Count++;
	string++;
    }

    return Count;
}

/*
 * Save and Restore the Parameters array ($1, $2 etc)
 */

static void F_LOCAL SaveNumericParameters (char **wp, SaveParameters *SaveArea)
{
    SaveArea->Array = ParameterArray;
    SaveArea->Count = ParameterCount;

    ParameterArray = wp;
    for (ParameterCount = 0; ParameterArray[ParameterCount] != NOWORD;
	 ++ParameterCount)
	continue;

    SetVariableFromNumeric (ParameterCountVariable, (long)--ParameterCount);
}

static void F_LOCAL RestoreTheParameters (SaveParameters *SaveArea)
{
    ParameterArray = SaveArea->Array;
    ParameterCount = SaveArea->Count;
    SetVariableFromNumeric (ParameterCountVariable, (long)ParameterCount);
}

/*
 * Special OS/2 processing for execve and spawnve
 */

#if (OS_TYPE == OS_OS2)
static int F_LOCAL OS_DosExecProgram (int Mode, char *Program, char **argv,
				      char **envp)
{
    OSCALL_PARAM	fExecFlags;
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
    OS_DosExecPgmReturnCode = 0;

/* Convert spawn mode to DosExecPgm mode */

    switch (Mode)
    {
	case P_WAIT:
	case P_NOWAIT:
	    fExecFlags = EXEC_ASYNCRESULT;
	    break;

	case P_NOWAITO:
	case OLD_P_OVERLAY:
	    fExecFlags = EXEC_ASYNC;
	    break;

	case P_DETACH:
	    fExecFlags = EXEC_BACKGROUND;
	    break;
    }

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

    if ((SavedArgs = (char **)AllocateMemoryCell ((argc + 3) * sizeof (char *)))
	    == (char **)NULL)
	return -1;

    memcpy (SavedArgs + 2, argv, (argc + 1) * sizeof (char *));

/* Set program name at Offset 0 */

    SavedArgs[0] = *argv;

/* Build OS2 Argument string in Offset 1 */

    if ((SavedArgs[1] = BuildOS2String (ProcessSpaceInParameters (&argv[1]),
					CHAR_SPACE)) == (char *)NULL)
	return -1;

/* Set up the new arg 2 - ~ + programname */

    if ((SavedArgs[2] = InsertCharacterAtStart (*argv)) == (char *)NULL)
	return -1;

/* Convert zero length args and args starting with a ~ */

    for (argc = 3; SavedArgs[argc] != NOWORD; argc++)
    {
	if (strlen (SavedArgs[argc]) == 0)
	    SavedArgs[argc] = "~";

	else if ((*SavedArgs[argc] == CHAR_TILDE) &&
		 ((SavedArgs[argc] = InsertCharacterAtStart (SavedArgs[argc]))
			== (char *)NULL))
	    return -1;
    }

/* Build the full argument list */

    if ((OS2Arguments = BuildOS2String (SavedArgs, 0)) == (char *)NULL)
	return -1;

/* Build OS/2 environment string */

    if ((OS2Environment = BuildOS2String (envp, 0)) == (char *)NULL)
	return -1;

/* Change signal handling */

    if (fExecFlags == EXEC_ASYNCRESULT)
    {
	char	*cp;

/* Also change the window title to match the foreground program */

	if ((cp = strrchr (Program, CHAR_DOS_PATH)) == (char *)NULL)
	    cp = Program;

	else
	    cp++;

	strcpy (FailName, cp);

	if ((cp = strrchr (FailName, CHAR_PERIOD)) != (char *)NULL)
	    *cp = 0;

	SetWindowName (FailName);
	*FailName = 0;

	IgnoreInterrupts = TRUE;
	sig_int = signal (SIGINT, SIG_IGN);
    }

/* Exec it - with hard-error processing turned off */

    DISABLE_HARD_ERRORS;

    OS_DosExecPgmReturnCode = DosExecPgm (FailName, sizeof (FailName),
    					  fExecFlags, OS2Arguments,
					  OS2Environment, &rescResults,
					  Program);

    ENABLE_HARD_ERRORS;

    if (fExecFlags == EXEC_ASYNCRESULT)
    {
	signal (SIGINT, SIG_IGN);

/* If the process started OK, wait for it */

	if (((OS_DosExecPgmReturnCode == NO_ERROR) ||
	     (OS_DosExecPgmReturnCode == ERROR_INTERRUPT)) &&
	    (Mode == P_WAIT))
	{
	    pidWait = rescResults.codeTerminate;

/* Re-try on interrupted system calls - and kill the child.  Why, because
 * sometimes the kill does not go to the child
 */

	    while ((OS_DosExecPgmReturnCode = DosCwait (DCWA_PROCESSTREE,
	    						DCWW_WAIT,
							&rescResults,
							&pidProcess,
							pidWait))
					     == ERROR_INTERRUPT)
		DosKillProcess (DKP_PROCESS, pidWait);
	}

	IgnoreInterrupts = FALSE;
	signal (SIGINT, sig_int);
    }


/*
 * What happened ?.  OS/2 Error - Map to UNIX errno.  Why can't people
 * write libraries right??  Or provide access at a high level.  We could
 * call _dosret if the interface did not require me to write more
 * assembler.
 */

    if (OS_DosExecPgmReturnCode != 0)
    {
	switch (OS_DosExecPgmReturnCode)
	{
#ifdef EAGAIN
	    case ERROR_NO_PROC_SLOTS:
		errno = EAGAIN;
		return -1;
#endif

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

	    case ERROR_BAD_ARGUMENTS:
		errno = E2BIG;
		return -1;

	    default:
		errno = ENOEXEC;
		return -1;
	}
    }

/* If exec - exit */

    switch (Mode)
    {
	case OLD_P_OVERLAY:			/* exec - exit at once */
	    FinalExitCleanUp (0);

	case P_WAIT:				/* Get exit code	*/
	    return rescResults.codeResult;

	case P_NOWAIT:				/* Get PID or SID	*/
	case P_NOWAITO:
	case P_DETACH:
	    return rescResults.codeTerminate;
    }

    errno = EINVAL;
    return -1;
}

/*
 * Insert character at start of string
 *
 * Return NULL or new string
 */

static char	*InsertCharacterAtStart (char *string)
{
    char	*cp;

    if ((cp = (char *)AllocateMemoryCell (strlen (string) + 2)) == (char *)NULL)
	return (char *)NULL;

    *cp = CHAR_TILDE;
    strcpy (cp + 1, string);

    return cp;
}
#endif

/*
 * Special NT processing for execve and spawnve.  Not really sure what to
 * do here yet!
 */

#if (OS_TYPE == OS_NT)
static int F_LOCAL OS_DosExecProgram (int Mode, char *Program, char **argv,
				      char **envp)
{
    DWORD			dwLogicalDrives = GetLogicalDrives();
    char			szNewDrive[4];
    unsigned int		i;
    char			**new_envp;
    char			ldir[PATH_MAX + 6];
    char			*nt_ep;
    char			*temp;
    int				off = 0;
    STARTUPINFO			StartupInfo;
    PROCESS_INFORMATION		ProcessInformation;
    char			*Environment;
    char			*PgmInputs;
    BOOL			rv;
    DWORD			ExitCode;

/*
 * Set up NT directory variables
 */

    if ((new_envp = (char **)GetAllocatedSpace (GetMemoryCellSize (envp) +
						sizeof (char *) * 26))
		  == (char **)NULL)
    {
	errno = ENOMEM;
	return -1;
    }

    strcpy (szNewDrive, "x:");

    for (i = 0; i < 25; i++)
    {
	if (dwLogicalDrives & (1L << i))
	{
	    szNewDrive[0] = i + 'A';

/* If the drive does not exist - give up */

	    DISABLE_HARD_ERRORS;
	    ExitCode = GetFullPathName (szNewDrive, PATH_MAX + 6, ldir, &temp);
	    ENABLE_HARD_ERRORS;

	    if (!ExitCode)
	        continue;

	    if ((nt_ep = GetAllocatedSpace (strlen (ldir) + 5)) == (char *)NULL)
	    {
		errno = ENOMEM;
		return -1;
	    }

	    sprintf (nt_ep, "=%c:=%s", i + 'A', ldir);
	    new_envp[off++] = nt_ep;
	}
    }
   
    memcpy (&new_envp[off], envp, GetMemoryCellSize (envp));

/* Setup environment block */

    if ((Environment = BuildOS2String (new_envp, 0)) == (char *)NULL)
	return -1;

/* Setup parameter block */

    ProcessSpaceInParameters (argv);

    argv[0] = Program;

    if ((PgmInputs = BuildOS2String (argv, CHAR_SPACE)) == (char *)NULL)
	return -1;

/* Set up startup info */

    memset (&StartupInfo, 0, sizeof (StartupInfo));

    StartupInfo.cb = sizeof (StartupInfo);
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;

/* Change Window name ? */

    if (Mode == P_WAIT)
    {
	char	*cp;
	char	*Name;

/* Also change the window title to match the foreground program */

	if ((cp = strrchr (Program, CHAR_DOS_PATH)) == (char *)NULL)
	    cp = Program;

	else
	    cp++;

	Name = StringCopy (cp);

	if ((cp = strrchr (Name, CHAR_PERIOD)) != (char *)NULL)
	    *cp = 0;

	SetWindowName (Name);
	ReleaseMemoryCell (Name);
    }

/* Exec it - with hard-error processing turned off */

    DISABLE_HARD_ERRORS;
    rv = CreateProcess (Program, PgmInputs, NULL, NULL, TRUE,
			NORMAL_PRIORITY_CLASS,
			Environment, NULL, &StartupInfo, &ProcessInformation);
/*#define DETACHED_PROCESS            0x00000008*/

    ENABLE_HARD_ERRORS;

    if (!rv)
    {
	OS_DosExecPgmReturnCode = GetLastError ();
	errno = ENOENT;
	return -1;
    }
    
    if (Mode == P_WAIT)
    {
	if ((ExitCode = WaitForSingleObject (ProcessInformation.hProcess,
					     INFINITE)) == WAIT_OBJECT_0)
	    return ExitCode;

/* Wait failed */

	OS_DosExecPgmReturnCode = GetLastError ();
	errno = EINVAL;
	return -1;
    }

/* Otherwise, return the process ID. */

    return ProcessInformation.dwProcessId;
}
#endif

/*
 * Execute a Function
 */

static bool F_LOCAL ExecuteFunction (char **wp, int *RetVal, bool CGVLCalled)
{
    Break_C			*s_RList = Return_List;
    Break_C			*s_BList = Break_List;
    Break_C			*s_SList = SShell_List;
    Break_C			BreakContinue;
    C_Op			*New;
    FunctionList		*s_CurrentFunction = CurrentFunction;
    SaveParameters		s_Parameters;
    FunctionList		*fop;
    GetoptsIndex		GetoptsSave;
    bool			TrapZeroExists;
    char			*Ext;
    char			*FullPath;

    TrapZeroExists = C2bool (GetVariableAsString ("~0", FALSE) != null);

/* Find the extension type - if it exists */

    FullPath = GetAllocatedSpace (FFNAME_MAX);
    
/* Check for a psuedo-function */

    switch (ExtensionType (Ext = FindFileAndExtension (FullPath,
						       wp[0],
						       ExecuteFunctionList)))
    {
	case EXTENSION_OTHER:
	    if ((fop = LookUpFunction (Ext, TRUE)) == (FunctionList *)NULL)
		return FALSE;

	    wp[0] = FullPath;
	    break;

/* Common extension for all .sh, .ksh and "" scripts.  If we find one, use
 * it, otherwise, check for a function of this name
 */

	case EXTENSION_SHELL_SCRIPT:
	    Ext = ".ksh";

	case EXTENSION_BATCH:
	    if ((fop = LookUpFunction (Ext, TRUE)) != (FunctionList *)NULL)
	    {
		wp[0] = FullPath;
		break;
	    }

/* Check for a real function */

	case EXTENSION_NOT_FOUND:
	case EXTENSION_EXECUTABLE:
	default:
	    if ((fop = LookUpFunction (wp[0], FALSE)) == (FunctionList *)NULL)
		return FALSE;
    }

/* Ok, we really have a function to execute.
 * Save the current variable list
 */

    if (!CGVLCalled && (CreateGlobalVariableList (FLAGS_FUNCTION) == -1))
    {
	*RetVal =  -1;
	return TRUE;
    }

/* Set up $0..$n for the function */

    SaveNumericParameters (wp, &s_Parameters);

/* Save Getopts pointers */

    GetGetoptsValues (&GetoptsSave);

/* Process the function */

    if (setjmp (BreakContinue.CurrentReturnPoint) == 0)
    {
	CurrentFunction = fop;
	Break_List = (Break_C *)NULL;
	BreakContinue.NextExitLevel = Return_List;
	Return_List = &BreakContinue;
	New = CopyFunction (fop->tree->left);
	*RetVal = ExecuteParseTree (New, NOPIPE, NOPIPE, EXEC_FUNCTION);
    }

/* A return has been executed - Unlike, while and for, we just need to
 * restore the local execute stack level and the return will restore
 * the correct I/O.
 */

    else
	*RetVal = (int)GetVariableAsNumeric (StatusVariable);

    if (!TrapZeroExists)
	RunTrapCommand (0);		/* Exit trap			*/

/* Restore the old $0, and previous return address */

    SaveGetoptsValues (GetoptsSave.Index, GetoptsSave.SubIndex);
    Break_List  = s_BList;
    Return_List = s_RList;
    SShell_List = s_SList;
    CurrentFunction = s_CurrentFunction;
    RestoreTheParameters (&s_Parameters);

    return TRUE;
}

/*
 * Print Load error message
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL PrintLoadError (char *path)
{
    if (*FailName)
	PrintWarningMessage ("%s: %s\nSYS1804: Cannot find file - %s", path,
			     ConvertErrorNumber(), FailName);

    else if (OS_DosExecPgmReturnCode)
	PrintWarningMessage ("%s: %s\n%s", path, ConvertErrorNumber(),
		     GetOSSystemErrorMessage (OS_DosExecPgmReturnCode));

    else
	PrintWarningMessage ("%s: %s", path, ConvertErrorNumber());
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static void F_LOCAL PrintLoadError (char *path)
{
    if (OS_DosExecPgmReturnCode)
	PrintWarningMessage ("%s: %s\n%s", path, ConvertErrorNumber(),
		     GetOSSystemErrorMessage (OS_DosExecPgmReturnCode));

    else
	PrintWarningMessage ("%s: %s", path, ConvertErrorNumber());
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS)
static void F_LOCAL PrintLoadError (char *path)
{
    PrintWarningMessage (BasicErrorMessage, path, ConvertErrorNumber ());
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_UNIX)
static void F_LOCAL PrintLoadError (char *path)
{
    PrintWarningMessage (BasicErrorMessage, path, ConvertErrorNumber ());
}
#endif

/*
 * Make the exported environment from the exported names in the dictionary.
 * Keyword assignments will already have been done.  Convert to MSDOS
 * format if flag set and m enabled
 */

static char ** F_LOCAL BuildCommandEnvironment (void)
{
/* Update SECONDS and RANDOM */

    HandleSECONDandRANDOM ();

/* Build the environment by walking the tree */

    BCE_WordList = (Word_B *)NULL;
    BCE_Length   = 0;

    twalk (VariableTree, BuildEnvironmentEntry);

    if (BCE_Length >= 0x7f00)
        return (char **)NULL;

    return GetWordList (AddWordToBlock (NOWORD, BCE_WordList));
}

/*
 * TWALK Function - Build Export VARIABLE list from VARIABLE tree
 */

static void BuildEnvironmentEntry (const void *key, VISIT visit, int level)
{
    VariableList	*vp = *(VariableList **)key;
    char		*cp;
    char		*sp;
    int			tlen;

    if ((visit != postorder) && (visit != leaf))
	return;

    if ((vp->status & STATUS_EXPORT) && (vp->index == 0))
    {
	cp = GetVariableAsString (vp->name, TRUE);
	tlen = strlen (vp->name) + strlen (cp) + 2;

	if ((BCE_Length += tlen) >= 0x7f00)
	    return;

	strcpy ((sp = GetAllocatedSpace (tlen)), vp->name);
	SetMemoryAreaNumber ((void *)sp, MemoryAreaLevel);
	strcat (sp, "=");
	strcat (sp, cp);

	BCE_WordList = AddWordToBlock (sp, BCE_WordList);

/* If MSDOS mode, we need to copy the variable, convert / to \ and put
 * the copy in the environment list instead
 */

	if (((ShellGlobalFlags & FLAGS_MSDOS_FORMAT) ||
	     (ExecProcessingMode.Flags & EP_EXPORT)) &&
	    (vp->status & STATUS_CONVERT_MSDOS))
	{
	    cp = StringCopy (BCE_WordList->w_words[BCE_WordList->w_nword - 1]);
	    BCE_WordList->w_words[BCE_WordList->w_nword - 1] = PATH_TO_DOS (cp);
	}
    }
}

/*
 * Parse and Execute a command in the current shell
 */

int	RunACommand (Source *s, char **params)
{
    jmp_buf		erp;
    int			RetVal = -1;
    Break_C		*S_RList = Return_List;	/* Save loval links	*/
    Break_C		*S_BList = Break_List;
    int			LS_depth = Execute_stack_depth++;
    C_Op		*outtree;
    bool		s_ProcessingEXECCommand = ProcessingEXECCommand;
    SaveParameters	s_Parameters;

/* Create a new save area */

    MemoryAreaLevel++;

/* Set up $0..$n for the command if appropriate.  Note that $0 does not
 * change
 */

    if (params != NOWORDS)
    {
	SaveNumericParameters (params, &s_Parameters);
	ParameterArray[0] = s_Parameters.Array[0];
    }

/* Execute the command */

    CreateNewEnvironment ();

    Return_List = (Break_C *)NULL;
    Break_List  = (Break_C *)NULL;
    ProcessingEXECCommand = TRUE;
    e.ErrorReturnPoint = (ErrorPoint)NULL;

    if (SetErrorPoint (erp) == 0)
    {

/* Read Input until completed */

	while (TRUE)
	{
	    if (((outtree = BuildParseTree (s)) != (C_Op *)NULL) &&
	        (outtree->type == TEOF))
		break;

	    RetVal = ExecuteParseTree (outtree, NOPIPE, NOPIPE, 0);
	}
    }

    QuitCurrentEnvironment ();

/* Restore the environment */

    ClearExtendedLineFile ();
    Return_List = S_RList;
    Break_List = S_BList;
    ProcessingEXECCommand = s_ProcessingEXECCommand;

/* Restore $0..$n */

    if (params != NOWORDS)
	RestoreTheParameters (&s_Parameters);

    RestoreEnvironment (RetVal, LS_depth);
    ReleaseMemoryArea (MemoryAreaLevel--);
    return RetVal;
}

/*
 * Get the OS/2 Error message
 */

#if (OS_TYPE == OS_OS2) 
char	*GetOSSystemErrorMessage (OSCALL_RET code)
{
    static char		Buffer [FFNAME_MAX + 9];
    char		*Buffer1;
    OSCALL_PARAM	MsgLength;
    OSCALL_RET		rc;
    char		*ip;
    char		*op = Buffer;
    char		*sp;
    int			len;

/* For some reason DPATH does not work with the DosGetMessage API as the
 * spec say it should on OS/2 2.x.  Probably something we do.  It usually
 * is!  So emulate it!
 */

    if ((len = strlen (sp = GetVariableAsString ("DPATH", FALSE))) < FFNAME_MAX)
	len = FFNAME_MAX;

    if ((Buffer1 = GetAllocatedSpace (len)) == (char *)NULL)
    {
	sprintf (Buffer, "SYS%.4d: No memory to get message text", code);
	return Buffer;
    }

    sp = PATH_TO_UNIX (strcpy (Buffer1, sp));

    do
    {
	sp = BuildNextFullPathName (sp, "OSO001.MSG", Buffer);
    } while ((access (Buffer, F_OK) != 0) && (sp != (char *)NULL));

/* If not found - use the default */

    if (sp == (char *)NULL)
    {
	strcpy (Buffer, "c:/OS2/SYSTEM/OSO001.MSG");
	*Buffer = GetDriveLetter (GetRootDiskDrive ());
    }

/* Read the message */

    if ((rc = DosGetMessage (NULL, 0, ip = Buffer1, len, code, Buffer,
    			    &MsgLength)))
	sprintf (Buffer, "SYS%.4d: No error message available (%d)",
		 code, rc);

    else
    {
	if ((Buffer1[MsgLength - 1] == CHAR_NEW_LINE) &&
	    (Buffer1[MsgLength - 2] == CHAR_RETURN))
	    Buffer1[MsgLength - 2] = 0;

	else
	    Buffer1[MsgLength] = 0;

/* Check the error number is there */

	if (strncmp (Buffer1, "SYS", 3) != 0)
	{
	    sprintf (op, "SYS%.4d: ", code);
	    op += strlen (op);
	}

/* Remove interior CRs & NLs */

	while ((*(op) = *(ip++)))
	{
	    if (*op == CHAR_NEW_LINE)
		*(op++) = CHAR_SPACE;

	    else if (*op != CHAR_RETURN)
		op++;
	}
    }

    ReleaseMemoryCell (Buffer1);
    return Buffer;
}

#  ifdef __WATCOMC__

/*
 * A cheat for WATCOM which does not have the DosGetMessage API.  This
 * function is based an interpretation of the System message file and one or
 * two others.  The format of the header is not know except for the flag at
 * offset 0x0f.  It is not guaranteed to work.
 */

APIRET APIENTRY	DosGetMessage  (PCHAR *ppchVTable,
				ULONG usVCount,
				PCHAR pchBuf,
				ULONG cbBuf,
				ULONG usMsgNum,
				PSZ pszFileName,
				PULONG pcbMsg)
{
    int				fd;
#  pragma pack (1)
    union {
	struct {
	    USHORT		Start;
	    USHORT		End;
	}			ShortE;
	struct {
	    ULONG		Start;
	    ULONG 		End;
	}			LongE;
    }				Start, Current;
#  pragma pack ()
    char			Type;
    int				Len;
    unsigned long		Offset;
    APIRET			Res;

    if ((fd = open (pszFileName, O_RDONLY | O_BINARY)) < 0)
	return _doserrno;

/* Get the message file format */

    if ((lseek (fd, 0x0fL, SEEK_SET) != 0x0fL) ||
	(read (fd, &Type, 1) != 1) ||
	(lseek (fd, 0x1fL, SEEK_SET) != 0x1fL))
    {
	Res = _doserrno;
	close (fd);
	return Res;
    }

/* Read the start of message text location */

    Len = (Type) ? 4 : 8;

    if (read (fd, &Start, Len) != Len)
    {
	Res = _doserrno;
	close (fd);
	return Res;
    }

/* Check the offset to the message */

    Offset = 0x1fL + (usMsgNum * ((Type) ? 2L : 4L));

    if (((Type) && (Offset >= Start.ShortE.Start)) ||
	((!Type) && (Offset >= Start.LongE.Start)))
    {
	close (fd);
	return ERROR_MR_MID_NOT_FOUND;
    }

/* Get the message location */

    if ((lseek (fd, Offset, SEEK_SET) != Offset) ||
	(read (fd, &Current, Len) != Len))
    {
	Res = _doserrno;
	close (fd);
	return Res;
    }

    if (((Type) && (Offset == Start.ShortE.Start - 2)) ||
	((!Type) && (Offset == Start.LongE.Start - 4)))
    {
	if ((Offset = lseek (fd, 0L, SEEK_END)) == -1L)
	{
	    Res = _doserrno;
	    close (fd);
	    return Res;
	}

	else if (Type)
	    Current.ShortE.End = (USHORT)Offset;

	else
	    Current.LongE.End = Offset;
    }

/* Get the message length */

    if (Type)
    {
	*pcbMsg = Current.ShortE.End - Current.ShortE.Start;
	Offset = Current.ShortE.Start;
    }

    else
    {
	*pcbMsg = (USHORT)(Current.LongE.End - Current.LongE.Start);
	Offset = Current.LongE.Start;
    }
   
/* Check the message length */

    *pcbMsg += 8;

    if (*pcbMsg >= cbBuf)
	return ERROR_MR_MSG_TOO_LONG;
    
    sprintf (pchBuf, "SYS%.4d: ", usMsgNum);

/* Seek to the start of the message and read its type */

    if ((lseek (fd, Offset, SEEK_SET) != Offset) ||
	(read (fd, &Type, 1) != 1))
    {
	Res = _doserrno;
	close (fd);
	return Res;
    }
    
    if (Type == '?') 
    {
	close (fd);
	return ERROR_MR_MID_NOT_FOUND;
    }

/* Get the message itself */

    else if (read (fd, pchBuf + 9, *pcbMsg - 8) != *pcbMsg - 8)
    {
	Res = _doserrno;
	close (fd);
	return Res;
    }

    close (fd);
    *(pchBuf + *pcbMsg) = 0;
    return 0;
}
#  endif
#endif

/*
 * Get the Win NT Error message
 */

#if (OS_TYPE == OS_NT) 
char	*GetOSSystemErrorMessage (OSCALL_RET code)
{
    DWORD		Source = 0;
    static char		EBuffer[100];
    char		*OSBuffer;
    char		*Buffer1;
    char		*ip;
    char		*op;

/* Read the message */

    if (!FormatMessage ((FORMAT_MESSAGE_FROM_SYSTEM |
		         FORMAT_MESSAGE_ALLOCATE_BUFFER), &Source, code, 0,
		        (LPSTR)&OSBuffer, 100, NULL))
    {
	sprintf (EBuffer, "SYS%.4d: No error message available (%ld)",
		 code, GetLastError ());
	OSBuffer = EBuffer;
    }

    else
    {
	op = &OSBuffer[strlen (OSBuffer) - 1];

	while (isspace (*op) && (op != OSBuffer))
	    op--;
    }

/* Allocate local space.  If there isn't any give up and hope for the best */

    if ((Buffer1 = GetAllocatedSpace (strlen (OSBuffer) + 20)) == (char *)NULL)
	return OSBuffer;

/* Transfer the system buffer to a local buffer.
 * Check the error number is there
 */

    op = Buffer1;
    ip = OSBuffer;

    if (strncmp (OSBuffer, "SYS", 3) != 0)
    {
	sprintf (op, "SYS%.4d: ", code);
	op += strlen (op);
    }

/* Remove interior CRs & NLs */

    while (*(op) = *(ip++))
    {
	if (*op == CHAR_NEW_LINE)
	    *(op++) = CHAR_SPACE;

	else if (*op != CHAR_RETURN)
	    op++;
    }

    if (OSBuffer != EBuffer)
	LocalFree (OSBuffer);

    return Buffer1;
}
#endif

/*
 * Display a started job info
 */

#if (OS_TYPE != OS_DOS)
static void F_LOCAL PrintPidStarted (void)
{
    if (PidInfo.Valid)
	fprintf (stderr, "[%d] %d\n", PidInfo.JobNo, PidInfo.PidNo);

    PidInfo.Valid = FALSE;
}
#endif

/*
 * Print a list of names, with or without numbers
 */

void	PrintAList (int ArgCount, char **ArgList)
{
    char	**pp = ArgList;
    int		i, j;
    int		ix;
    int		MaxArgWidth = 0;
    int		NumberWidth = 0;
    int		ncols;
    int		nrows;

    if (!ArgCount)
        return;

/* get dimensions of the list */

    while (*pp != (char *)NULL)
    {
	i = strlen (*(pp++)) + 1;
	MaxArgWidth = (i > MaxArgWidth) ? i : MaxArgWidth;
    }

/*
 * We print an index of the form
 *	%d)
 * in front of each entry.  Get the max width of this
 */

    for (i = ArgCount, NumberWidth = 1; i >= 10; i /= 10)
	NumberWidth++;

/* In the case of numbered lists, we go down if less than screen length */

    if (ArgCount < (MaximumLines - 5))
    {
	ncols = 1;
	nrows = ArgCount;
    }

    else
    {
	ncols = MaximumColumns / (MaxArgWidth + NumberWidth + 3);
	nrows = ArgCount / ncols;

	if (ArgCount % ncols)
	    nrows++;

	if (!nrows)
	    nrows = 1;

	if (ncols > nrows)
	{
	    nrows = ncols;
	    ncols = 1;
	}
    }

/* Display the list */

    for (i = 0; i < nrows; i++)
    {
	for (j = 0; j < ncols; j++)
	{
	    if ((ix = j * nrows + i) < ArgCount)
	    {
		printf ("%*d) ", NumberWidth, ix + 1);

		if (j != (ncols - 1))
		    printf ("%-*.*s", MaxArgWidth, MaxArgWidth, ArgList[ix]);

		else
		    printf ("%-s", ArgList[ix]);
	    }
	}

	fputchar (CHAR_NEW_LINE);
    }
}

/* 
 * Are we tracking all commands?  Check there is no path in the command
 */

static void F_LOCAL TrackAllCommands (char *path, char *arg)
{
    if (FL_TEST (FLAG_TRACK_ALL) &&
	(FindPathCharacter (arg) == (char *)NULL) &&
	(!IsDriveCharacter (arg[1])))
	SaveAlias (arg, PATH_TO_UNIX (path), TRUE);
}


/*
 * Get the application type and get we can do it
 */

static bool F_LOCAL GetApplicationType (char *path)
{
    ApplicationType = QueryApplicationType (path);

/* Some type of error */

    if (ApplicationType & EXETYPE_ERROR)
    {
	if (ApplicationType == EXETYPE_UNKNOWN)
	{
	    if (!FL_TEST (FLAG_WARNING))
		fprintf (stderr, "sh: Cannot determine executable type <%s>\n",
			 path);

	    return TRUE;
	}
	
	else if (ApplicationType == EXETYPE_BAD_FILE)
	    errno = ENOENT;

	else
	    errno = ENOEXEC;

	return FALSE;
    }

/*
 * This is where it gets complicated - Sort out DOS!
 */

#if (OS_TYPE == OS_DOS)
    if (ExecProcessingMode.Flags & EP_IGNTYPE)
    {
	ApplicationType = EXETYPE_DOS_CUI;
	return TRUE;
    }

    else if (ApplicationType == EXETYPE_DOS_GUI)
    {
	if ((BaseOS == BASE_OS_WIN) &&
	    (GetVariableAsString (LIT_STARTWINP, FALSE) == null))
	{
	    if (!FL_TEST (FLAG_WARNING))
		feputs ("sh: Start this applications from Windows\n");

	    errno = ENOEXEC;
	    return FALSE;
	}
    }

    else if ((ApplicationType == EXETYPE_DOS_32) && (BaseOS == BASE_OS_NT))
	return BadApplication ("DOS 32-bit");

    else if (ApplicationType & EXETYPE_OS2)
	return BadApplication ("OS/2");

    else if (ApplicationType & EXETYPE_NT)
	return (BaseOS == BASE_OS_NT) ? TRUE : BadApplication ("Win NT");

#elif (OS_TYPE == OS_OS2)

/*
 * OK - now OS/2
 */

    if (ApplicationType & EXETYPE_NT)
	return BadApplication ("Win NT");

#  if (OS_SIZE == OS_16)
    if (OS_VERS_N < 2)
    {
	if (ApplicationType & EXETYPE_DOS)
	    return BadApplication (LIT_dos);

	else if (ApplicationType & EXETYPE_OS2_32) 
	    return BadApplication ("OS/2 32-bit");
    }
#  endif

    if (ApplicationType == EXETYPE_DOS_32)
	return BadApplication ("DOS 32-bit");

#elif (OS_TYPE == OS_NT)

/*
 * OK - now NT
 */

    if (ApplicationType == EXETYPE_DOS_32)
	return BadApplication ("DOS 32-bit");

    else if (ApplicationType & EXETYPE_OS2_32) 
	return BadApplication ("OS/2 32-bit");
#endif

/* In the end - execute it */

    return TRUE;
}

/*
 * Print Bad application warning
 */

static bool F_LOCAL BadApplication (char *mes)
{
    if (!FL_TEST (FLAG_WARNING))
	fprintf (stderr, "sh: Cannot execute %s applications\n", mes);

    errno = ENOEXEC;
    return FALSE;
}

/*
 * Handle a Windows Program
 */

#if (OS_TYPE != OS_UNIX)
static int F_LOCAL ExecuteWindows (char *Fullpath, char **argv, char **envp,
				   int  ForkAction)
{
    int		res;
#  if (OS_TYPE == OS_DOS)
    Word_B	*wb = (Word_B *)NULL;

    if (!SetUpCLI ((BaseOS != BASE_OS_WIN) 
			? "win"
			: GetVariableAsString (LIT_STARTWINP, FALSE), &wb))
	return -1;

#  elif (OS_TYPE == OS_OS2)
    Word_B	*wb = AddWordToBlock ("winos2", (Word_B *)NULL);
#  elif (OS_TYPE == OS_NT)
    Word_B	*wb = (Word_B *)NULL;
#  endif

/* Add the rest of the parameters and execute */

    ForkAction |= EXEC_WINDOWS;

    res = ExecuteSpecialProcessor (Fullpath, argv, envp, ForkAction, wb);

/* A cheat to stop us exec'ing the program again.  Normally,
 * ExecuteSpecialProcessor is call below ExecuteProgram.  However, in the
 * case of a Windows prog, its called above, so we set this to stop
 * Executeprogram invoking the program again, but telling it that no
 * swapping was required.
 */

    ExecProcessingMode.Flags = EP_NOSWAP;
    return res;
}
#endif

/*
 * Generic processing for script and windows programs
 */

static int F_LOCAL ExecuteSpecialProcessor (char *Fullpath, char **argv,
					    char **envp, int ForkAction,
					    Word_B *wb)
{
    char	**nargv;
    char	*p_name;		/* Program name		*/
    char	*cp;
    int		j;

/* Add the rest of the parameters */

    wb = AddWordToBlock (Fullpath, wb);

    j = 1;
    while (argv[j] != NOWORD)
	wb = AddWordToBlock (argv[j++], wb);

/* Execute the program */

    nargv = GetWordList (AddWordToBlock (NOWORD, wb));

/* Special for UNIX compatability, use ourselves */

    if ((strcmp (nargv[0], "/bin/sh") == 0) ||
	(strcmp (nargv[0], "/bin/ksh") == 0))
	nargv[0] = GetVariableAsString (ShellVariableName, FALSE);

/* See if the program exists.  If it doesn't, strip the path */

    else if (((cp = strrchr (nargv[0], CHAR_UNIX_DIRECTORY)) != (char *)NULL) &&
	     ((p_name = AllocateMemoryCell (FFNAME_MAX)) != (char *)NULL))
    {
	if (FindLocationOfExecutable (p_name, nargv[0]) == EXTENSION_NOT_FOUND)
	    strcpy (nargv[0], cp + 1);

	ReleaseMemoryCell (p_name);
    }

/* Get the new program mode */

    CheckProgramMode (*nargv, &ExecProcessingMode);

    j = EnvironExecute (nargv, ForkAction);

    if (ExecProcessingMode.Flags != EP_ENVIRON)
	j = LocalExecve (nargv, envp, ForkAction);

/* 0 is a special case - see ConvertErrorNumber */

    if (j == -1)
	errno = 0;

    return j;
}

/*
 * Parse a new CLI string
 */

static bool F_LOCAL SetUpCLI (char *string, Word_B **wb)
{
    char	*sp = StringCopy (string);

    if (sp == null)
	return FALSE;

    *wb = SplitString (sp, *wb);
    return TRUE;
}
