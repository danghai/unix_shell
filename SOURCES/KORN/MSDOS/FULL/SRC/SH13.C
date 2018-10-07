/*
 * MS-DOS SHELL - Command Line Editing
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Charles Forsyth
 *
 * This code is based on (in part) the EMACS and VI editing code from Simon
 * J. Gerraty's Public Domain Korn Shell and is subject to the following
 * copyright restrictions.  The VI Command Editing was originally based on
 * code written by John Rochester and modified by Larry Bouzane, Eric Gisin
 * and Mike Jetzer.  The EMACS Command Editing was originally based on code
 * written by Ron Natalie and modified by Doug Kingston, Doug Gwyn, Lou
 * Salkind, Eric Gisin and Kai Uwe Rommel.
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh13.c,v 1.10 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh13.c,v $
 * Revision 1.10  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 1.9  1994/02/01  10:25:20  istewart
 * Release 2.3 Beta 2, including first NT port
 *
 * Revision 1.8  1994/01/20  14:51:43  istewart
 * Release 2.3 Beta 1
 *
 * Revision 1.7  1994/01/11  17:55:25  istewart
 * Release 2.3 Beta 0 patches
 *
 * Revision 1.6  1993/12/02  09:29:04  istewart
 * Fix incorrect ifdef for EMACS/GMACS
 *
 * Revision 1.5  1993/11/09  10:39:49  istewart
 * Beta 226 checking
 *
 * Revision 1.4  1993/08/25  16:03:57  istewart
 * Beta 225 - see Notes file
 *
 * Revision 1.3  1993/07/02  10:21:35  istewart
 * 224 Beta fixes
 *
 * Revision 1.2  1993/06/14  11:01:44  istewart
 * More changes for 223 beta
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include "sh.h"

/*
 * VI Functions
 */

#ifdef FLAGS_VI
static int F_LOCAL	VI_MainLoop (void);
static int F_LOCAL	VI_GetNextCharacter (void);
static bool F_LOCAL	VI_StateMachine (int);
static int F_LOCAL	VI_GetNextState (int);
static int F_LOCAL	VI_InsertCharacter (int);
static int F_LOCAL	VI_ExecuteCommand (int, char *);
static int F_LOCAL	VI_ExecuteMove (int, char *, bool);
static int F_LOCAL	VI_RedoInsert (int);
static void F_LOCAL	VI_YankSelection (int, int);
static int F_LOCAL	VI_GetBracketType (int);
static void F_LOCAL	VI_Refresh (bool);
static void F_LOCAL	VI_CopyInput2Hold (void);
static void F_LOCAL	VI_CopyHold2Input (void);
static void F_LOCAL	VI_RedrawLine (void);
static void F_LOCAL	VI_CreateWindowBuffers (void);
static void F_LOCAL	VI_DeleteRange (int, int);
static bool F_LOCAL	VI_GetEventFromHistory (bool, int);
static int F_LOCAL	VI_FindEventFromHistory (bool, int, bool, char *);
static int F_LOCAL	VI_InsertIntoBuffer (char *, int, bool);
static bool F_LOCAL	VI_OutOfWindow (void);
static int F_LOCAL	VI_FindCharacter (int, int, bool, bool);
static void F_LOCAL	VI_ReWindowBuffer (void);
static void F_LOCAL	VI_YankDelete (int, int);
static int F_LOCAL	VI_AdvanceColumn (int, int);
static void F_LOCAL	VI_DisplayWindow (char *, char *, bool);
static void F_LOCAL	VI_MoveToColumn (int, char *);
static void F_LOCAL	VI_OutputPrompt (bool);
static bool F_LOCAL	VI_MoveThroughHistory (int);
static bool F_LOCAL	VI_EditLine (int);
static void F_LOCAL	VI_SaveUndoBuffer (int, char *);
static bool F_LOCAL	VI_ChangeCommand (int, char *);
static bool F_LOCAL	VI_CommandPut (int, char);
static void F_LOCAL	VI_UndoCommand (void);
static bool F_LOCAL	VI_ResetLineState (void);
static bool F_LOCAL	VI_ExecuteSearch (char *);
static bool F_LOCAL	VI_InsertWords (int);
static bool F_LOCAL	VI_ChangeCase (int);
static bool F_LOCAL	VI_ExecuteCompletion (char *);
static bool F_LOCAL	VI_HandleInputAlias (char *);

static int F_LOCAL	VI_ForwardWord (int);
static int F_LOCAL	VI_BackwardWord (int);
static int F_LOCAL	VI_EndofWord (int);
static int F_LOCAL	VI_ForwardToWhiteSpace (int);
static int F_LOCAL	VI_BackwardToWhiteSpace (int);
static int F_LOCAL	VI_ForwardToEndOfNonWhiteSpace (int);
#endif

/*
 * EMACS Edit Functions
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
static int F_LOCAL	EMACS_MainLoop (void);
static int F_LOCAL	EMACS_AutoInsert (int);
static int F_LOCAL	EMACS_InsertMacroString (int);
static void F_LOCAL	EMACS_InsertString (char *);
static int F_LOCAL	EMACS_DeleteCharacterBackwards (int);
static int F_LOCAL	EMACS_DeleteCurrentCharacter (int);
static int F_LOCAL	EMACS_DeleteString (int);
static int F_LOCAL	EMACS_DeletePreviousWord (int);
static int F_LOCAL	EMACS_MoveBackAWord (int);
static int F_LOCAL	EMACS_MoveForwardAWord (int);
static int F_LOCAL	EMACS_DeleteNextWord (int);
static int F_LOCAL	EMACS_GetPreviousWord (void);
static int F_LOCAL	EMACS_GetNextWord (void);
static int F_LOCAL	EMACS_GotoColumn (char *);
static int F_LOCAL	EMACS_GetDisplayStringSize (char *);
static int F_LOCAL	EMACS_PreviousCharacter (int);
static int F_LOCAL	EMACS_NextCharacter (int);
static int F_LOCAL	EMACS_FindCharacter (int, char *);
static int F_LOCAL	EMACS_ForwardToCharacter (int);
static int F_LOCAL	EMACS_BackwardToCharacter (int);
static int F_LOCAL	EMACS_NewLine (int);
static int F_LOCAL	EMACS_EndOfInput (int);
static int F_LOCAL	EMACS_GetFirstHistory (int);
static int F_LOCAL	EMACS_GetLastHistory (int);
static int F_LOCAL	EMACS_GetPreviousCommand (int);
static int F_LOCAL	EMACS_GetNextCommand (int);
static int F_LOCAL	EMACS_LoadFromHistory (int);
static int F_LOCAL	EMACS_OperateOnLine (int);
static int F_LOCAL	EMACS_SearchHistory (int);
static int F_LOCAL	EMACS_SearchMatch (char *, int, int);
static int F_LOCAL	EMACS_PatternMatch (char *, char *);
static int F_LOCAL	EMACS_EOTOrDelete (int);
static int F_LOCAL	EMACS_KillLine (int);
static int F_LOCAL	EMACS_GotoEnd (int);
static int F_LOCAL	EMACS_GotoStart (int);
static int F_LOCAL	EMACS_RedrawLine (int);
static int F_LOCAL	EMACS_Transpose (int);
static int F_LOCAL	EMACS_LiteralValue (int);
static int F_LOCAL	EMACS_Prefix1 (int);
static int F_LOCAL	EMACS_Prefix2 (int);
static int F_LOCAL	EMACS_Prefix3 (int);
static int F_LOCAL	EMACS_KillToEndOfLine (int);
static void F_LOCAL	EMACS_StackText (char *, int);
static void F_LOCAL	EMACS_ResetInput (void);
static int F_LOCAL	EMACS_YankText (int);
static int F_LOCAL	EMACS_PutText (int);
static int F_LOCAL	EMACS_Abort (int);
static int F_LOCAL	EMACS_Error (int);
static int F_LOCAL	EMACS_FullReset (int);
static void F_LOCAL	EMACS_MapInKeyStrokes (char *);
static void F_LOCAL	EMACS_MapOutKeystrokes (unsigned int);
static void F_LOCAL	EMACS_PrintMacros (int, int);
static int F_LOCAL	EMACS_SetMark (int);
static int F_LOCAL	EMACS_KillRegion (int);
static int F_LOCAL	EMACS_ExchangeCurrentAndMark (int);
static int F_LOCAL	EMACS_NoOp (int);
static int F_LOCAL	EMACS_CompleteFile (int);
static int F_LOCAL	EMACS_ListFiles (int);
static int F_LOCAL	EMACS_SubstituteFiles (int);
static int F_LOCAL	EMACS_FindLongestMatch (char *, char *);
static int F_LOCAL	EMACS_SetArgValue (int);
static int F_LOCAL	EMACS_Multiply (int);
static int F_LOCAL	EMACS_GetWordsFromHistory (int);
static int F_LOCAL	EMACS_FoldCase (int);
static int F_LOCAL	EMACS_ClearScreen (int);
static int F_LOCAL	EMACS_Comment (int);
static int F_LOCAL	EMACS_AliasInsert (int);
static int F_LOCAL	EMACS_GetNextCharacter (void);
static int F_LOCAL	EMACS_GetNonFunctionKey (void);
static int F_LOCAL	EMACS_FileCompletion (int);
static void F_LOCAL	EMACS_SaveFileName (char *, char *);
static void F_LOCAL	EMACS_ListSavedFileNames (void);
static int F_LOCAL	EMACS_YankPop (int);
static int F_LOCAL	EMACS_PushText (int);
static void F_LOCAL	EMACS_CheckArgCount (void);
static int F_LOCAL	EMACS_YankError (char *);

#  if (OS_TYPE != OS_DOS)
static int F_LOCAL	EMACS_DisplayJobList (int);
#  endif
#endif

/*
 * General Edit functions
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
static void F_LOCAL	GEN_BackspaceOver (int);
static int F_LOCAL	GEN_GetCharacterSize (int);
static char * F_LOCAL	GEN_FindLastVisibleCharacter (void);
static void F_LOCAL	GEN_OutputCharacterWithControl (int);
static void F_LOCAL	GEN_AdjustOutputString (char *);
static void F_LOCAL	GEN_Redraw (int);
static void F_LOCAL	GEN_PutAString (char *);
static void F_LOCAL	GEN_PutACharacter (int);
static void F_LOCAL	GEN_AdjustRedraw (void);
static char * F_LOCAL	GEN_FindAliasMatch (int);
#endif

/*
 * VI command types
 */

#if defined (FLAGS_VI)
#define VI_COMMAND	0x01
#define VI_C_MOVE	0x02
#define VI_C_EXTEND	0x04
#define VI_C_LONG	0x08
#define VI_C_NOTUNDO	0x10
#define VI_C_BAD	0x20
#define VI_C_META	0x40
#define VI_C_SEARCH	0x80
#define VI_C_COMMAND	(VI_C_MOVE | VI_C_EXTEND | VI_COMMAND | VI_C_NOTUNDO)

#define VI_IsBad(c)		(classify[c] & VI_C_BAD)
#define VI_IsCommand(c)		(classify[c] & VI_C_COMMAND)
#define VI_IsMove(c)		(classify[c] & VI_C_MOVE)
#define VI_IsExtend(c)		(classify[c] & VI_C_EXTEND)
#define VI_IsLong(c)		(classify[c] & VI_C_LONG)
#define VI_IsMeta(c)		(classify[c] & VI_C_META)
#define VI_IsUndoable(c)	(!(classify[c] & VI_C_NOTUNDO))
#define VI_IsSearch(c)		(classify[c] & VI_C_SEARCH)

static unsigned char	classify[256] = {
    VI_C_BAD,			/* Ctrl @ - 				*/
    0,				/* Ctrl A - 				*/
    0,				/* Ctrl B - 				*/
    0,				/* Ctrl C - Interrupt			*/
    0,				/* Ctrl D - EOF 			*/
    0,				/* Ctrl E - 				*/
    VI_C_META,			/* Ctrl F - 				*/
    0,				/* Ctrl G - 				*/
    VI_COMMAND | VI_C_MOVE,	/* Ctrl H - Delete previous char	*/
    0,				/* Ctrl I - 				*/
    VI_C_META,			/* Ctrl J - End input			*/
    0,				/* Ctrl K - 				*/
    VI_C_META | VI_C_NOTUNDO,	/* Ctrl L - Re-print line		*/
    VI_C_META,			/* Ctrl M - End input			*/
    VI_C_META,			/* Ctrl N - 				*/
    0,				/* Ctrl O - 				*/

    VI_C_META,			/* Ctrl P -				*/
    0,				/* Ctrl Q -				*/
    0,				/* Ctrl R -				*/
    0,				/* Ctrl S -				*/
    0,				/* Ctrl T -				*/
    0,				/* Ctrl U -				*/
    0,				/* Ctrl V - escape next char		*/
    0,				/* Ctrl W - delete previous space word	*/
    0,				/* Ctrl X -				*/
    0,				/* Ctrl Y -				*/
    VI_C_META,			/* Ctrl Z - EOF?			*/
    0,				/* Ctrl [ -				*/
    0,				/* Ctrl \ -				*/
    0,				/* Ctrl ] -				*/
    0,				/* Ctrl ^ -				*/
    0,				/* Ctrl _ -				*/

    VI_COMMAND | VI_C_MOVE,	/* [count]  - Move right		*/
    0,				/* ! -					*/
    0,				/* " -					*/
    VI_COMMAND,			/* # - Insert comment char		*/
    VI_C_MOVE,			/* $ - move to end of line		*/
    VI_COMMAND,			/* % - Match brackets			*/
    0,				/* & -					*/
    0,				/* ' -					*/
    0,				/* ( -					*/
    0,				/* ) -					*/
    VI_COMMAND,			/* * - File name substitution		*/
    VI_COMMAND,			/* [count] + - see j			*/
    VI_C_MOVE,			/* [count] , - repeat find		*/
    VI_COMMAND,			/* [count] - - see k			*/
    0,				/* [count] . - Redo			*/
    VI_COMMAND | VI_C_SEARCH,	/* [count] / - Def forward search	*/

    VI_C_MOVE,			/* 0 - move to start of line		*/
    0,				/* 1 - all digits are counts		*/
    0,				/* 2 -					*/
    0,				/* 3 -					*/
    0,				/* 4 -					*/
    0,				/* 5 -					*/
    0,				/* 6 -					*/
    0,				/* 7 -					*/
    0,				/* 8 -					*/
    0,				/* 9 -					*/
    0,				/* : -					*/
    VI_C_MOVE,			/* [count] ; - repeat find		*/
    0,				/* < -					*/
    VI_COMMAND,			/* = - Lists directory			*/
    0,				/* > -					*/
    VI_COMMAND | VI_C_SEARCH,	/* [count] ? - Def prev search string	*/

    VI_COMMAND | VI_C_LONG,	/* @ - Insert Alias			*/
    VI_COMMAND,			/* A - append to end of line		*/
    VI_C_MOVE,			/* [count] B - back a spaced word	*/
    VI_COMMAND,			/* C - change to end of line		*/
    VI_COMMAND,			/* D - delete to end of line		*/
    VI_C_MOVE,			/* [count] E - go to end of spaced word	*/
    VI_C_MOVE | VI_C_LONG,	/* [count] F - find previous char	*/
    VI_COMMAND,			/* G - Get history entry		*/
    0,				/* H -					*/
    VI_COMMAND,			/* I - Insert at start of line		*/
    0,				/* J -					*/
    0,				/* K -					*/
    0,				/* L -					*/
    0,				/* M -					*/
    VI_COMMAND,			/* N - Search backwards			*/
    0,				/* O -					*/

    VI_COMMAND,			/* P - Place previous mod before	*/
    0,				/* Q -					*/
    VI_COMMAND,			/* R - Replace til ESC			*/
    VI_COMMAND,			/* S - Substitute whole line		*/
    VI_C_MOVE | VI_C_LONG,	/* T - equiv to F l			*/
    VI_COMMAND,			/* U - Undo all				*/
    0,				/* V -					*/
    VI_C_MOVE,			/* [count] W - Move to next spaced word	*/
    VI_COMMAND,			/* [count] X - delete previous		*/
    VI_COMMAND,			/* Y - Yank rest of link		*/
    0,				/* Z -					*/
    0,				/* [ -					*/
    VI_COMMAND,			/* \ - File Name completion		*/
    0,				/* ] -					*/
    VI_C_MOVE,			/* ^ - Move to first non-blank in line	*/
    VI_COMMAND,			/* [count] _ - Get word from previous C	*/

    0,				/* ` -					*/
    VI_COMMAND,			/* a - insert after cursor		*/
    VI_C_MOVE,			/* [count] b - Move backward a word	*/
    VI_C_EXTEND,		/* [count] c - Change chars		*/
    VI_C_EXTEND,		/* [count] d - Delete chars		*/
    VI_C_MOVE,			/* [count] e - Move to end of word	*/
    VI_C_MOVE | VI_C_LONG,	/* [count] f - Find next char		*/
    0,				/* g -					*/
    VI_C_MOVE,			/* [count] h - Move left a char		*/
    VI_COMMAND,			/* i - insert before			*/
    VI_COMMAND,			/* [count] j - Get next history		*/
    VI_COMMAND,			/* [count] k - Get pre history		*/
    VI_C_MOVE,			/* [count] l - Move right a char	*/
    0,				/* m -					*/
    VI_COMMAND,			/* [count] n - Search next		*/
    0,				/* o -					*/

    VI_COMMAND,			/* p - Place previous mod after		*/
    0,				/* q -					*/
    VI_COMMAND,			/* [count] r - replace chars		*/
    VI_COMMAND,			/* [count] s - substitute chars		*/
    VI_C_MOVE | VI_C_LONG,	/* t - equiv to f l			*/
    VI_COMMAND | VI_C_NOTUNDO,	/* u - undo				*/
    VI_COMMAND | VI_C_NOTUNDO,	/* v - invoke editor			*/
    VI_C_MOVE,			/* [count] w - Move forward a word	*/
    VI_COMMAND,			/* [count] x - Delete current character	*/
    VI_C_EXTEND,		/* [count] y - Yank count		*/
    0,				/* z -					*/
    0,				/* { -					*/
    VI_C_MOVE,			/* [count] | - Move to column		*/
    0,				/* } -					*/
    VI_COMMAND,			/* [count] ~ - Change case		*/
    0				/* DEL -				*/
};

/*
 * Map ini keyboard functions to vi functions.  Some are not yet supported.
 */

static unsigned char	VI_IniMapping[] = {
    '?', 				/* Scan backwards in history	*/
    '/',				/* Scan forewards in history	*/
    'k',				/* Previous command		*/
    'j',				/* Next command			*/
    'h',				/* Left one character		*/
    'l',				/* Right one character		*/
    'w',				/* Right one word		*/
    'b',				/* Left one word		*/
    '0',				/* Move to start of line	*/
    'C',				/* Clear input line		*/
    'D',				/* Flush to end of line		*/
    CHAR_END_LINE,			/* End of line			*/
    'i',				/* Insert mode switch		*/
    'x',				/* Delete right character	*/
    'X',				/* Delete left character	*/
    CHAR_META,				/* Complete file name		*/
    '=',				/* Complete directory function	*/
    0,					/* Clear screen			*/
    0,					/* Print Job tree		*/
    0,					/* Transpose characters		*/
    0x016,				/* Quote character		*/
};

#define VI_MAX_CMD_LENGTH	3
#define VI_MAX_SRCH_LENGTH	40

#define VI_UNDEF_MODE		0	/* Undefined			*/
#define VI_INSERT_MODE		1	/* Insert mode			*/
#define VI_REPLACE_MODE		2	/* Replace mode			*/

#define VI_S_NORMAL		0	/* Normal input mode		*/
#define VI_S_ARG1		1	/* Getting argument 1		*/
#define VI_S_EXTENDED_CMD	2	/* Extended command		*/
#define VI_S_ARG2		3	/* Getting argument 2		*/
#define VI_S_EXCHANGE		4
#define VI_S_FAIL		5	/* Error state			*/
#define VI_S_EXECUTE		6	/* Execute command		*/
#define VI_S_REDO		7	/* Redo last command		*/
#define VI_S_LIT		8
#define VI_S_SEARCH		9	/* Search mode			*/
#define VI_S_REPLACE1CHAR	10	/* Replace single char		*/

/*
 * VI Editor status structure
 */

struct edstate {
    int		WindowLeftColumn;
    int		InputLength;
    int		CursorColumn;
};

/*
 * The VI editor status and undo buffer status
 */

static struct edstate	vi_EditorState;
static struct edstate	vi_UndoState;

#define VI_InputLength		vi_EditorState.InputLength
#define VI_CurrentColumn	vi_EditorState.CursorColumn

/*
 * The VI insert buffer
 */

static char	vi_InsertBuffer[LINE_MAX + 1];/* input buffer		*/
static int	vi_InsertBufferLength;	/* length of input buffer	*/

					/* last search pattern		*/
static char	vi_SearchPattern[VI_MAX_SRCH_LENGTH];
static int	vi_SearchPatternLength;	/* length of current search pattern */
					/* last search command		*/
static int	vi_LastSearchCommand = CHAR_SPACE;

					/* last find command		*/
static int	vi_LastFindCommand = CHAR_SPACE;
					/* character to find		*/
static int	vi_LastFindCharacter;
					/* The Yank buffer		*/
static char	*vi_YankBuffer = (char *)NULL;
					/* last non-move command	*/
static char	vi_PreviousCommand[VI_MAX_CMD_LENGTH];
static int	vi_PrevCmdArgCount;	/* argcnt for vi_PreviousCommand*/
static char	*vi_HoldBuffer = null;	/* last edit buffer		*/

static bool	vi_InputBufferChanged;	/* buffer has been "modified"	*/
static int	vi_Insert;		/* non-zero in insert mode	*/
static int	vi_State;
static int	vi_WhichWindow;		/* window buffer in use		*/
static char	vi_MoreIndicator;	/* more char at right of window	*/
					/* Alias input buffer		*/
static char	*vi_AliasBuffer = (char *)NULL;
					/* The undo save buffer		*/
static char	*vi_UndoBuffer = null;
#endif

/*
 * EMACS statics
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)

/* File Completion functions */

#define EMACS_FN_LIST		0
#define EMACS_FN_COMPLETE	1
#define EMACS_FN_SUBSTITUTE	2

/* Values returned by keyboard functions */

#define	EMACS_KEY_NORMAL	0
#define	EMACS_KEY_META		1		/* ^[, ^X */
#define	EMACS_KEY_EOL		2		/* ^M, ^J */
#define	EMACS_KEY_INTERRUPT	3		/* ^G, ^C */
#define	EMACS_KEY_NOOP		4

/* Function table structure */

typedef struct EMACS_FunctionMap  {
    int			(F_LOCAL * xf_func)(int);
    char		*emacs_FunctionName;
    char		emacs_TableNumber;
    unsigned char	emacs_KeyStroke;
    unsigned char	emacs_FunctionFlags;
} EMACS_FunctionMap;

/* emacs_FunctionFlags values */

#define	EMACS_MEMORY_ALLOC	0x40	/* ALlocate memory		*/
#define	EMACS_NO_BIND		0x80	/* No binding			*/
#define EMACS_INI_MASK		0x1f	/* bottom 5 bits		*/

					/* Check for word separator	*/
#define	EMACS_IS_SPACE(c)	(!(isalnum (c)|| c == '$'))

#define EMACS_KEYDEF_TABLES	4	/* number of keydef tables etc	*/
#define EMACS_KEYDEF_ENTRIES	256	/* size of keydef tables etc	*/
#define	EMACS_KILL_SIZE		20	/* Yank/Kill stack size		*/

static int	emacs_Prefix1 = CHAR_OPEN_BRACKETS & 0x01f;
static int	emacs_Prefix2 = 'X' & 0x01f;
static int	emacs_Prefix3 = 0xE0;
static char	*emacs_MarkPointer;		/* mark pointer		*/
static int	emacs_NextCommandIs = -1;	/* for newline-and-next	*/
static int	(F_LOCAL *emacs_LastCommand)(int) = (int (F_LOCAL *)(int))NULL;

static int	emacs_UnGetCharacter = -1;	/* Unget character	*/
static char	*emacs_NTY = "\nnothing to yank";

/*
 * Key and macro structures
 */

static EMACS_FunctionMap *(*emacs_KeyDefinitions)[EMACS_KEYDEF_ENTRIES] = NULL;
static char		 *(*emacs_MacroDefinitions)[EMACS_KEYDEF_ENTRIES] = NULL;

						/* Stack info		*/
static char		*emacs_Stack[EMACS_KILL_SIZE];
static int		emacs_StackPointer;
static int		emacs_TopOfStack;

static int		emacs_CurrentPrefix;
static char		*emacs_CurrentMacroString;
static int		emacs_MaxFilenameSize;	/* to determine column width */
static Word_B		*EMACS_Flist = (Word_B *)NULL;

static int		emacs_ArgumentCount = 0;/* general purpose arg	*/

/*
 * EMACS command table
 */

static EMACS_FunctionMap	EMACS_FunctionMaps[] = {
    {EMACS_AutoInsert,	"auto-insert",		0,	0,
		    	KF_INSERT },
    {EMACS_Error,	"error",		0,	0,		0 },
    {EMACS_InsertMacroString,
			"macro-string",		0,	0,
					EMACS_NO_BIND | EMACS_MEMORY_ALLOC },
    {EMACS_AliasInsert,	null,			0,	 0,		0 },

#define	EMACS_INSERT_MAP	&EMACS_FunctionMaps[0]
#define	EMACS_ERROR_MAP		&EMACS_FunctionMaps[1]
#define	EMACS_MACRO_MAP		&EMACS_FunctionMaps[2]
#define EMACS_ALIAS_MAP		&EMACS_FunctionMaps[3]

/* Do not move the above!!! */

/*
 * Movement and delete functions
 */

    {EMACS_GotoEnd,	"end-of-line",		0,	'E' & 0x01f,
    			KF_END },
    {EMACS_GotoStart,	"beginning-of-line",	0,	'A' & 0x01f,
    			KF_START },
    {EMACS_KillLine,	"kill-line",		0, 	'U' & 0x01f,
    			KF_CLEAR },
    {EMACS_KillToEndOfLine,
			"kill-to-eol",		0, 	'K' & 0x01f,
			KF_FLUSH },

    {EMACS_NextCharacter,
			"forward-char",		0,	'F' & 0x01f,
			KF_RIGHT },
    {EMACS_MoveForwardAWord,
			"forward-word",		1,	'F',
			KF_WORDRIGHT },
    {EMACS_PreviousCharacter,
			"backward-char",	0,	'B' & 0x01f,
			KF_LEFT },
    {EMACS_MoveBackAWord,
			"backward-word", 	1,	'B',
			KF_WORDLEFT },

    {EMACS_DeleteCurrentCharacter,
			"delete-char-forward",	0,	0,
			KF_DELETERIGHT },
    {EMACS_DeleteNextWord,
			"delete-word-forward", 	1,	'D',		0 },

    {EMACS_DeleteCharacterBackwards,
			"delete-char-backward",	0,	CHAR_BACKSPACE,
			KF_DELETELEFT },
    {EMACS_DeletePreviousWord,
			"delete-word-backward",	1,	CHAR_BACKSPACE,	0 },
    {EMACS_DeletePreviousWord,
			"delete-word-backward",	1,	'H',		0 },

/*
 * Search character functions
 */

    {EMACS_ForwardToCharacter,
			"search-char-forward",	0,	']' & 0x01f,	0 },
    {EMACS_BackwardToCharacter,
			"search-char-backward", 1,	']' & 0x01f,	0 },

/*
 * End of text functions
 */

    {EMACS_NewLine,	"newline",		0,	CHAR_RETURN,	0 },
    {EMACS_NewLine,	"newline",		0,	CHAR_NEW_LINE,	0 },
    {EMACS_EndOfInput,	"eot",			0,	'_' & 0x01f,	0 },
    {EMACS_Abort,	"abort",		0,	'G' & 0x01f,	0 },
    {EMACS_NoOp,	"no-op",		0,	0,		0 },
    {EMACS_EOTOrDelete, "eot-or-delete",	0,	'D' & 0x01f,	0 },

/*
 * History functions
 */

    {EMACS_GetPreviousCommand,
			"up-history",		0,	'P' & 0x01f,
			KF_PREVIOUS },
    {EMACS_GetNextCommand,
			"down-history",		0,	'N' & 0x01f,
			KF_NEXT},
    {EMACS_SearchHistory,
			"search-history",	0,	'R' & 0x01f,
			KF_SCANFOREWARD },
    {EMACS_GetFirstHistory,
			"beginning-of-history",	1,	'<',		0 },
    {EMACS_GetLastHistory,
			"end-of-history",	1,	'>',		0 },
    {EMACS_OperateOnLine,
			"operate",		0, 	'O' & 0x01f,	0 },
    {EMACS_GetWordsFromHistory,
			"prev-hist-word", 	1,	CHAR_PERIOD,	0 },
    {EMACS_GetWordsFromHistory,
			"copy-last-arg", 	1,	'_',		0 },

    {EMACS_RedrawLine,	"redraw",		0, 	'L' & 0x01f,	0 },
    {EMACS_ClearScreen,	"clear-screen",		0,	0,
			KF_CLEARSCREEN },
    {EMACS_Prefix1,	"prefix-1",		0,	CHAR_ESCAPE,	0 },
    {EMACS_Prefix2,	"prefix-2",		0,	'X' & 0x01f,	0 },
    {EMACS_Prefix3,	"prefix-3",		0, 	0xE0,		0 },
    {EMACS_LiteralValue,
			"quote",		0, 	'^' & 0x01f,	0 },
    {EMACS_LiteralValue,
			"quote",		0, 	CHAR_META,
			KF_QUOTE },

    {EMACS_PushText,	"push-text",		1, 	'p',		0 },
    {EMACS_YankText,	"yank-text",		1, 	'P',		0 },
    {EMACS_PutText,	"pop-text", 		0,	'Y' & 0x01f,	0 },
    {EMACS_YankPop,	"yank-pop", 		1,	'y',		0 },
    {EMACS_Transpose,	"transpose-chars",	0, 	'T' & 0x01f,
			KF_TRANSPOSE },
    {EMACS_SetMark,	"set-mark",		1,	CHAR_SPACE,	0 },
    {EMACS_KillRegion,	"kill-region",		0, 	'W' & 0x01f,	0 },
    {EMACS_ExchangeCurrentAndMark,
			"exchange-point-and-mark", 2,	'X' & 0x01f,	0 },

    {EMACS_FullReset, 	"reset",		0,	 0,		0 },

    {EMACS_CompleteFile,
			"complete",		1, 	CHAR_ESCAPE,
			KF_COMPLETE },
    {EMACS_SubstituteFiles,
			"complete-list",	1,	'*',		0 },
    {EMACS_ListFiles,	"list",			1,	'=',
			KF_DIRECTORY },

#  if (OS_TYPE != OS_DOS)
    {EMACS_DisplayJobList,
			"jobs",		        2,	'j',
			KF_JOBS },
#  endif

    {EMACS_Comment,	"comment-execute",      1,	'#',    	0 },

    {EMACS_SetArgValue,	null,			1,	'0',		0 },
    {EMACS_SetArgValue,	null,			1,	'1',		0 },
    {EMACS_SetArgValue,	null,			1,	'2',		0 },
    {EMACS_SetArgValue,	null,			1,	'3',		0 },
    {EMACS_SetArgValue,	null,			1,	'4',		0 },
    {EMACS_SetArgValue,	null,			1,	'5',		0 },
    {EMACS_SetArgValue,	null,			1,	'6',		0 },
    {EMACS_SetArgValue,	null,			1,	'7',		0 },
    {EMACS_SetArgValue,	null,			1,	'8',		0 },
    {EMACS_SetArgValue,	null,			1,	'9',		0 },
    {EMACS_Multiply,	"multiply",		1, 	'M',		0 },

    {EMACS_FoldCase,	"upcase-word",		1,	'U',		0 },
    {EMACS_FoldCase,	"downcase-word",	1,	'L',		0 },
    {EMACS_FoldCase,	"capitalise-word",	1,	'C',		0 },
    {EMACS_FoldCase,	"upcase-char",		1,	'u',		0 },
    {EMACS_FoldCase,	"downcase-char",	1,	'l',		0 },
    {EMACS_FoldCase,	"capitalise-char",	1,	'c',		0 },
    { NULL }
};

#endif

/*
 * General statics
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
static bool	LastVisibleCharValid = FALSE;
static int	CurrentScreenPosition;	/* current column on line	*/
static int	PromptWidth;		/* width of prompt		*/
static int	WindowWidth;		/* width of window		*/
					/* The window buffers		*/
static char	*WindowBuffer[] = { (char *)NULL, (char *)NULL };
static int	CurrentHistoryEvent;	/* position in history		*/

static char	*emacs_CurrentPosition;	/* current position		*/
static char	*emacs_EndOfLine;	/* current end of line		*/
static char	*emacs_StartVisible;	/* start of visible portion of	*/
					/* input buffer			*/
					/* last char visible on screen	*/
static char	*emacs_LastVisibleCharacter;

/*
 * we use AdjustDone so that functions can tell
 * whether GEN_AdjustRedraw () has been called while they are active.
 */

static int	AdjustDone = 0;
static bool	AdjustOK = TRUE;
static int	CurrentScreenColumn = 0;
static int	DisplayWidth;
#endif

/*
 * VI mode functions
 *
 * The VI State machine
 */

#if defined (FLAGS_VI)
static bool F_LOCAL VI_StateMachine (int ch)
{
    static char		curcmd[VI_MAX_CMD_LENGTH];
    static char		locpat[VI_MAX_SRCH_LENGTH];
    static int		cmdlen;
    static int		argc1, argc2;

    if ((vi_State != VI_S_SEARCH) &&
        ((ch == CHAR_RETURN || ch == CHAR_NEW_LINE)))
    {
	GEN_PutACharacter (CHAR_RETURN);
	GEN_PutACharacter (CHAR_NEW_LINE);
	FlushStreams ();
	return TRUE;
    }

    switch (vi_State)
    {
	case VI_S_REPLACE1CHAR:
	    curcmd[cmdlen++] = (char)ch;
	    vi_State = VI_S_EXECUTE;
	    break;

	case VI_S_NORMAL:
	    if (vi_Insert != VI_UNDEF_MODE)
	    {
		if ((ch == ('V' & 0x01f)) || (ch == CHAR_META))
		{
		    vi_State = VI_S_LIT;
		    ch = (ch == ('V' & 0x01f)) ? '^' : CHAR_META;
		}

		if (VI_InsertCharacter (ch) != 0)
		{
		    RingWarningBell ();
		    vi_State = VI_S_NORMAL;
		}

		else if (vi_State == VI_S_LIT)
		{
		    VI_CurrentColumn--;
		    VI_Refresh (FALSE);
		}

		else
		    VI_Refresh (C2bool (vi_Insert != VI_UNDEF_MODE));
	    }

	    else
	    {
		cmdlen = 0;
		argc1 = 0;

		if ((ch != '0') && isdigit (ch))
		{
		    argc1 = ch - '0';
		    vi_State = VI_S_ARG1;
		}

		else
		{
		    curcmd[cmdlen++] = (char)ch;
		    vi_State = VI_GetNextState (ch);

		    if (vi_State == VI_S_SEARCH)
		    {
			VI_CopyInput2Hold ();
			VI_CurrentColumn = 0;
   			ConsoleLineBuffer[VI_InputLength = 0] = 0;

			if (ch == '/')
			{
			    if (VI_InsertIntoBuffer (DirectorySeparator, 1,
			    			     FALSE) != 0)
				return TRUE;
			}

			else if (VI_InsertIntoBuffer ("?", 1, FALSE) != 0)
			    return TRUE;

			VI_Refresh (FALSE);
		    }
		}
	    }

	    break;

	case VI_S_LIT:
	    if (VI_IsBad (ch))
	    {
		VI_DeleteRange (VI_CurrentColumn, VI_CurrentColumn + 1);
		RingWarningBell ();
	    }

/* If control V - then replace ^ with character */

	    else if (ConsoleLineBuffer[VI_CurrentColumn] == '^')
		ConsoleLineBuffer[VI_CurrentColumn++] = (char)ch;

/* If \, the check for specials and replace \ with special.  Otherwise,
 * include the \ as well
 */

	    else
	    {
	        switch (ch)
		{
		    case CHAR_ESCAPE:
		    case 0x7f:
		    case CHAR_BACKSPACE:
		    case 'U' & 0x01f:
		    case 'W' & 0x01f:
		    case CHAR_META:
			ConsoleLineBuffer[VI_CurrentColumn++] = (char)ch;
			break;

/* Insert the real character */

		    default:
			VI_CurrentColumn++;
			if (VI_InsertCharacter (ch) != 0)
			    RingWarningBell ();
	    	}
	    }

	    VI_Refresh (TRUE);
	    vi_State = VI_S_NORMAL;
	    break;

	case VI_S_ARG1:
	    if (IS_Numeric (ch))
		argc1 = argc1 * 10 + ch - '0';

	    else
	    {
		curcmd[cmdlen++] = (char)ch;
		vi_State = VI_GetNextState (ch);
	    }

	    break;

	case VI_S_EXTENDED_CMD:
	    argc2 = 0;

	    if (ch >= '1' && ch <= '9')
	    {
		argc2 = ch - '0';
		vi_State = VI_S_ARG2;
		return FALSE;
	    }

	    else
	    {
		curcmd[cmdlen++] = (char)ch;

		if (ch == curcmd[0])
		    vi_State = VI_S_EXECUTE;

		else if (VI_IsMove (ch))
		    vi_State = VI_GetNextState (ch);

		else
		    vi_State = VI_S_FAIL;
	    }

	    break;

	case VI_S_ARG2:
	    if (IS_Numeric (ch))
		argc2 = argc2 * 10 + ch - '0';

	    else
	    {
		if (argc1 == 0)
		    argc1 = argc2;

		else
		    argc1 *= argc2;

		curcmd[cmdlen++] = (char)ch;

		if (ch == curcmd[0])
		    vi_State = VI_S_EXECUTE;

		else if (VI_IsMove (ch))
		    vi_State = VI_GetNextState (ch);

		else
		    vi_State = VI_S_FAIL;
	    }

	    break;

	case VI_S_EXCHANGE:
	    if (ch == CHAR_ESCAPE)
		vi_State = VI_S_NORMAL;

	    else
	    {
		curcmd[cmdlen++] = (char)ch;
		vi_State = VI_S_EXECUTE;
	    }
	    break;

	case VI_S_SEARCH:
	    switch (ch)
	    {
		case CHAR_RETURN:
		case CHAR_NEW_LINE:
		    locpat[vi_SearchPatternLength] = 0;
		    strcpy (vi_SearchPattern, locpat);
		    /* VI_RedrawLine(); */
		    vi_State = VI_S_EXECUTE;
		    break;

		case 0x7f:
		case 0x08:
		    if (vi_SearchPatternLength == 0)
		    {
			VI_CopyHold2Input ();
			vi_State = VI_S_NORMAL;
		    }

		    else
		    {
			vi_SearchPatternLength--;

			if ((locpat[vi_SearchPatternLength] < CHAR_SPACE) ||
			    (locpat[vi_SearchPatternLength] == 0x7f))
			    VI_InputLength--;

			VI_InputLength--;
   			ConsoleLineBuffer[VI_CurrentColumn =
							VI_InputLength] = 0;

			VI_Refresh (FALSE);
			return FALSE;
		    }

		    VI_Refresh (FALSE);
		    break;

		case ('U' & 0x01f):
		    vi_SearchPatternLength = 0;
		    ConsoleLineBuffer[1] = 0;
		    VI_InputLength = 1;
		    VI_CurrentColumn = 1;
		    VI_Refresh (FALSE);
		    return FALSE;

		default:
		    if (vi_SearchPatternLength == VI_MAX_SRCH_LENGTH - 1)
			RingWarningBell ();

		    else
		    {
			locpat[vi_SearchPatternLength++] = (char)ch;

			if ((ch < CHAR_SPACE) || (ch == 0x7f))
			{
			    ConsoleLineBuffer[VI_InputLength++] = '^';
			    ConsoleLineBuffer[VI_InputLength++] =
						(char)(ch ^ '@');
			}

			else
			    ConsoleLineBuffer[VI_InputLength++] = (char)ch;

   			ConsoleLineBuffer[VI_CurrentColumn =
					VI_InputLength] = 0;

			VI_Refresh (FALSE);
		    }

		    return FALSE;
	    }

	    break;
    }

    switch (vi_State)
    {
	case VI_S_EXECUTE:
	    vi_State = VI_S_NORMAL;

	    switch (VI_ExecuteCommand (argc1, curcmd))
	    {
		case -1:
		    RingWarningBell ();
		    break;

		case 0:
		    if (vi_Insert != VI_UNDEF_MODE)
			vi_InsertBufferLength = 0;

		    VI_Refresh (C2bool (vi_Insert != VI_UNDEF_MODE));
		    break;

		case 1:
		    VI_Refresh (FALSE);
		    GEN_PutACharacter (CHAR_RETURN);
		    GEN_PutACharacter (CHAR_NEW_LINE);
		    FlushStreams ();
		    return TRUE;
	    }

	    break;

	case VI_S_REDO:
	    vi_State = VI_S_NORMAL;

	    if (argc1 != 0)
		vi_PrevCmdArgCount = argc1;

	    switch (VI_ExecuteCommand (vi_PrevCmdArgCount, vi_PreviousCommand))
	    {
		case -1:
		    RingWarningBell ();
		    VI_Refresh (FALSE);
		    break;

		case 0:
		    if (vi_Insert != VI_UNDEF_MODE)
		    {
			if ((vi_PreviousCommand[0] == 's') ||
			    (vi_PreviousCommand[0] == 'c') ||
			    (vi_PreviousCommand[0] == 'C'))
			{
			    if (VI_RedoInsert (1) != 0)
				RingWarningBell ();
			}

			else if (VI_RedoInsert (vi_PrevCmdArgCount) != 0)
			    RingWarningBell ();
		    }

		    VI_Refresh (FALSE);
		    break;

		case 1:
		    VI_Refresh (FALSE);
		    GEN_PutACharacter (CHAR_RETURN);
		    GEN_PutACharacter (CHAR_NEW_LINE);
		    FlushStreams ();
		    return TRUE;
	    }

	    break;

	case VI_S_FAIL:
	    vi_State = VI_S_NORMAL;
	    RingWarningBell ();
	    break;
    }

    return FALSE;
}

/* Probably could have been done more elegantly than by creating a new
 * state, but it works
 */

static int F_LOCAL VI_GetNextState (int ch)
{
    if (ch == 'r')
	return VI_S_REPLACE1CHAR;

    else if (VI_IsExtend (ch))
	return VI_S_EXTENDED_CMD;

    else if (VI_IsSearch (ch))
	return VI_S_SEARCH;

    else if (VI_IsLong (ch))
	return VI_S_EXCHANGE;

    else if (ch == CHAR_PERIOD)
	return VI_S_REDO;

    else if (VI_IsCommand (ch))
	return VI_S_EXECUTE;

    else
	return VI_S_FAIL;
}

/*
 * Insert a character into the VI line buffer
 */

static int F_LOCAL VI_InsertCharacter (int ch)
{
    int		tcursor;

    switch (ch)
    {
	case 0:
	    return -1;

	case CHAR_ESCAPE:
	    if ((vi_PreviousCommand[0] == 's') ||
	        (vi_PreviousCommand[0] == 'c') ||
		(vi_PreviousCommand[0] == 'C'))
		return VI_RedoInsert (0);

	    else
		return VI_RedoInsert (vi_PrevCmdArgCount - 1);

	case 0x7f:			/* delete */
	case CHAR_BACKSPACE:		/* delete */
	    if (VI_CurrentColumn != 0)
	    {
		if (vi_InsertBufferLength > 0)
		    vi_InsertBufferLength--;

		VI_CurrentColumn--;

		if (vi_Insert != VI_REPLACE_MODE)
		{
		    memmove (&ConsoleLineBuffer[VI_CurrentColumn],
		    	     &ConsoleLineBuffer[VI_CurrentColumn+1],
			     VI_InputLength - VI_CurrentColumn);

		    VI_InputLength--;
		    ConsoleLineBuffer[VI_InputLength] = 0;
		}
	    }

	    break;

	case ('U' & 0x01f):
	    if (VI_CurrentColumn != 0)
	    {
		vi_InsertBufferLength = 0;
		memmove (ConsoleLineBuffer,
			 &ConsoleLineBuffer[VI_CurrentColumn],
			 VI_InputLength - VI_CurrentColumn);

		VI_InputLength -= VI_CurrentColumn;
		ConsoleLineBuffer[VI_InputLength] = 0;
		VI_CurrentColumn = 0;
	    }

	    break;

	case ('W' & 0x01f):
	    if (VI_CurrentColumn != 0)
	    {
		tcursor = VI_BackwardWord(1);
		memmove (&ConsoleLineBuffer[tcursor],
			 &ConsoleLineBuffer[VI_CurrentColumn],
			 VI_InputLength - VI_CurrentColumn);

		VI_InputLength -= VI_CurrentColumn - tcursor;
		ConsoleLineBuffer[VI_InputLength] = 0;

		if (vi_InsertBufferLength < VI_CurrentColumn - tcursor)
		    vi_InsertBufferLength = 0;

		else
		    vi_InsertBufferLength -= VI_CurrentColumn - tcursor;

		VI_CurrentColumn = tcursor;
	    }

	    break;

	default:
	    if (VI_InputLength == LINE_MAX)
		return -1;

	    vi_InsertBuffer[vi_InsertBufferLength++] = (char)ch;

	    if (vi_Insert == VI_INSERT_MODE)
	    {
		memmove (&ConsoleLineBuffer[VI_CurrentColumn + 1],
			 &ConsoleLineBuffer[VI_CurrentColumn],
			 VI_InputLength - VI_CurrentColumn);

		VI_InputLength++;
	    }

	    ConsoleLineBuffer[VI_CurrentColumn++] = (char)ch;

	    if ((vi_Insert == VI_REPLACE_MODE) &&
	        (VI_CurrentColumn > VI_InputLength))
		VI_InputLength++;

	    ConsoleLineBuffer[VI_InputLength] = 0;
    }

    return 0;
}

/*
 * Process the current command
 */

static int F_LOCAL VI_ExecuteCommand (int argcnt, char *cmd)
{
    int			cur;

    if ((argcnt == 0) && (*cmd != '_') && (*cmd != 'v'))
    {
	if (*cmd == 'G')
	    argcnt = GetLastHistoryEvent () + 1;

	else
	    argcnt = 1;
    }

    if (VI_IsMove ((int)*cmd))
    {
	if ((cur = VI_ExecuteMove (argcnt, cmd, FALSE)) >= 0)
	{
	    if ((cur == VI_InputLength) && cur)
		cur--;

	    VI_CurrentColumn = cur;
	}

	else
	    return -1;
    }

    else
    {
	if (VI_IsUndoable ((int)*cmd))
	    VI_SaveUndoBuffer (argcnt, cmd);

	switch (*cmd)
	{
	    case 'v':
	    {
	        bool	res = VI_EditLine (argcnt);

		VI_RedrawLine ();

		if (!res)
		    return -1;

		break;
	    }

	    case ('L' & 0x01f):
		VI_RedrawLine ();
		break;

	    case 'a':
		vi_InputBufferChanged = TRUE;

		if (VI_InputLength != 0)
		    VI_CurrentColumn++;

		vi_Insert = VI_INSERT_MODE;

		break;

	    case 'A':
		vi_InputBufferChanged = TRUE;
		VI_DeleteRange (0, 0);
		VI_CurrentColumn = VI_InputLength;
		vi_Insert = VI_INSERT_MODE;
		break;

	    case 'c':
	    case 'd':
	    case 'y':
		if (!VI_ChangeCommand (argcnt, cmd))
		    return -1;

		break;

	    case 'p':
	    case 'P':
		if (!VI_CommandPut (argcnt, *cmd))
		    return -1;

		break;

	    case 'Y':			/* Yank to end of line		*/
		VI_YankSelection (VI_CurrentColumn, VI_InputLength);
		break;

	    case 'S':			/* Substitute the whole line	*/
		VI_YankSelection (0, VI_InputLength);
	        VI_CurrentColumn = 0;

	    case 'C':			/* Change to the end of line	*/
		vi_InputBufferChanged = TRUE;
		VI_DeleteRange (VI_CurrentColumn, VI_InputLength);
		vi_Insert = VI_INSERT_MODE;
		break;

	    case 'D': 			/* Delete to the end of line	*/
		VI_YankDelete (VI_CurrentColumn, VI_InputLength);

		if (VI_CurrentColumn != 0)
		    VI_CurrentColumn--;

		break;

	    case 'G':			/* Go to history event		*/
		if (!VI_GetEventFromHistory (vi_InputBufferChanged, argcnt))
		    return -1;

		else
		{
		    vi_InputBufferChanged = FALSE;
		    CurrentHistoryEvent = argcnt;
		}

		break;

	    case 'I':			/* Insert at beginning of line	*/
		VI_CurrentColumn = 0;

	    case 'i':			/* Insert			*/
		vi_InputBufferChanged = TRUE;
		vi_Insert = VI_INSERT_MODE;
		break;

	    case CHAR_PLUS:
	    case 'j':
		if (!VI_MoveThroughHistory (argcnt))
		    return -1;

		break;

	    case CHAR_HYPHEN:
	    case 'k':
		if (!VI_MoveThroughHistory (-argcnt))
		    return -1;

		break;

	    case 'r':
		if (VI_InputLength == 0)
		    return -1;

		vi_InputBufferChanged = TRUE;
		ConsoleLineBuffer[VI_CurrentColumn] = cmd[1];
		break;

	    case 'R':
		vi_InputBufferChanged = TRUE;
		vi_Insert = VI_REPLACE_MODE;
		break;

	    case 's':
		if (VI_InputLength == 0)
		    return -1;

		vi_InputBufferChanged = TRUE;

		if (VI_CurrentColumn + argcnt > VI_InputLength)
		    argcnt = VI_InputLength - VI_CurrentColumn;

		VI_DeleteRange (VI_CurrentColumn, VI_CurrentColumn + argcnt);
		vi_Insert = VI_INSERT_MODE;
		break;

	    case 'x':
		if (VI_InputLength == 0)
		    return -1;

		vi_InputBufferChanged = TRUE;

		if (VI_CurrentColumn + argcnt > VI_InputLength)
		    argcnt = VI_InputLength - VI_CurrentColumn;

		VI_YankDelete (VI_CurrentColumn, VI_CurrentColumn + argcnt);
		break;

	    case 'X':
		if (VI_CurrentColumn <= 0)
		    return -1;

		vi_InputBufferChanged = TRUE;

		if (VI_CurrentColumn < argcnt)
		    argcnt = VI_CurrentColumn;

		VI_YankDelete (VI_CurrentColumn - argcnt, VI_CurrentColumn);
		VI_CurrentColumn -= argcnt;
		break;

/* This is not as simple as it looks, because the current State always uses
 * the ConsoleLineBuffer
 */

	    case 'u':
		VI_UndoCommand ();
		break;

/* Restore the current history event or the null line */

	    case 'U':
		VI_ResetLineState ();
		break;

/* Search commands */

	    case '?':
	    case '/':
	    case 'n':
	    case 'N':
		if (!VI_ExecuteSearch (cmd))
		    return -1;

		break;

	    case CHAR_TILDE:
		if (!VI_ChangeCase (argcnt))
		    return -1;

		break;

	    case '@':
		if (!VI_HandleInputAlias (cmd))
		    return -1;

		break;

	    case '_':
		if (!VI_InsertWords (argcnt))
		    return -1;

		break;

	    case CHAR_COMMENT:
		VI_CurrentColumn = 0;

		if (VI_InsertIntoBuffer ("#", 1, FALSE) != 0)
		    return -1;

		return 1;

	    case '*':
	    case '=':
	    case '\\':
		if (!VI_ExecuteCompletion (cmd))
		    return -1;

		break;
	}

	if ((vi_Insert == VI_UNDEF_MODE) && (VI_CurrentColumn != 0) &&
	    (VI_CurrentColumn >= VI_InputLength))
	    VI_CurrentColumn--;
    }

    return 0;
}

/*
 * Handle an Input alias
 */

static bool F_LOCAL VI_HandleInputAlias (char *cmd)
{
    return C2bool ((vi_AliasBuffer = GEN_FindAliasMatch (cmd[1]))
				   != (char *)NULL);
}


/*
 * Yank and delete some text
 */

static void F_LOCAL VI_YankDelete (int start, int end)
{
    VI_YankSelection (start, end);
    VI_DeleteRange (start, end);
}

/*
 * Undo the previous command
 */

static void F_LOCAL VI_UndoCommand (void)
{
    char	*cp;
    int		InputLength = VI_InputLength;
    int		cursor  = VI_CurrentColumn;
    int		WindowLeftColumn = vi_EditorState.WindowLeftColumn;

/* Save the current input line and the editor state */

    ConsoleLineBuffer[VI_InputLength] = 0;
    cp = StringCopy (ConsoleLineBuffer);

/* Move to the undo information */

    vi_EditorState = vi_UndoState;

/* If the undo state has a buffer, restore that buffer to the Console
 * line buffer
 */

    if (vi_UndoBuffer != null)
    {
	strcpy (ConsoleLineBuffer, vi_UndoBuffer);
	ReleaseMemoryCell (vi_UndoBuffer);
    }

    else
	memset (ConsoleLineBuffer, 0, LINE_MAX + 1);

/* Set up the undo status information */

    vi_UndoBuffer		  = cp;
    vi_UndoState.InputLength      = InputLength;
    vi_UndoState.CursorColumn     = cursor;
    vi_UndoState.WindowLeftColumn = WindowLeftColumn;
}

/*
 * Reset the line to its original state
 */

static bool F_LOCAL VI_ResetLineState (void)
{
    char	*hptr = null;
    int		lasthistory;

    if ((CurrentHistoryEvent < 0) ||
	(CurrentHistoryEvent > (lasthistory = GetLastHistoryEvent ())))
	return FALSE;

    if ((lasthistory != CurrentHistoryEvent) &&
	((hptr = GetHistoryRecord (CurrentHistoryEvent)) == (char *)NULL))
	return FALSE;

    strcpy (ConsoleLineBuffer, hptr);
    VI_InputLength = strlen (hptr);
    VI_CurrentColumn = 0;
    vi_InputBufferChanged = FALSE;
    return TRUE;
}

/*
 * Execute a search for a string
 */

static bool F_LOCAL VI_ExecuteSearch (char *cmd)
{
    int		NewCE;
    bool	Direction;

/* Set start position */

    if (*cmd == '?')
	CurrentHistoryEvent = -1;

/* Reset save info for next search command */

    if (tolower (*cmd) != 'n')
    {
	vi_SearchPatternLength = 0;
	vi_LastSearchCommand = *cmd;
    }

/* Check we know which direction to go */

    if (vi_LastSearchCommand == CHAR_SPACE)
	return FALSE;

    Direction = C2bool (vi_LastSearchCommand == '?');

    if (*cmd == 'N')
	Direction = (bool)!Direction;

    if ((NewCE = VI_FindEventFromHistory (vi_InputBufferChanged,
					  CurrentHistoryEvent, Direction,
					  vi_SearchPattern)) < 0)
    {
	if (tolower (*cmd) != 'n')
	{
	    VI_CopyHold2Input ();
	    VI_Refresh (FALSE);
	}

	return FALSE;
    }

/* Found match !! */

    vi_InputBufferChanged = FALSE;
    CurrentHistoryEvent = NewCE;
    return TRUE;
}

/*
 * Insert words from previous command into the buffer
 */

static bool F_LOCAL VI_InsertWords (int argcnt)
{
    int		space;
    char	*p, *sp;

    if ((p = GetHistoryRecord (GetLastHistoryEvent () - 1)) == (char *)NULL)
	return FALSE;

    if (argcnt)
    {
	while (*p && isspace (*p))
	    p++;

	while (*p && --argcnt)
	{
	    p = SkipToWhiteSpace (p);

	    while (*p && isspace (*p))
		p++;
	}

	if (!*p)
	    return FALSE;

	sp = p;
    }

    else
    {
	sp = p;
	space = 0;

	while (*p)
	{
	    if (isspace (*p))
		space = 1;

	    else if (space)
	    {
		space = 0;
		sp = p;
	    }

	    p++;
	}

	p = sp;
    }

    vi_InputBufferChanged = TRUE;

    if (VI_InputLength != 0)
	VI_CurrentColumn++;

    while (*p && !isspace (*p))
    {
	argcnt++;
	p++;
    }

    if (VI_InsertIntoBuffer (" ", 1, FALSE) != 0)
	argcnt = -1;

    else if (VI_InsertIntoBuffer (sp, argcnt, FALSE) != 0)
	argcnt = -1;

    if (argcnt < 0)
    {
	if (VI_CurrentColumn != 0)
	    VI_CurrentColumn--;

	return FALSE;
    }

    vi_Insert = VI_INSERT_MODE;
    return TRUE;
}

/*
 * Change case of characters
 */

static bool F_LOCAL VI_ChangeCase (int argcnt)
{
    char	*p;

    if (VI_InputLength == 0)
	return FALSE;

    p = &ConsoleLineBuffer[VI_CurrentColumn];

    while (argcnt--)
    {
	if (islower (*p))
	{
	    vi_InputBufferChanged = TRUE;
	    *p = (char)toupper (*p);
	}

	else if (isupper (*p))
	{
	    vi_InputBufferChanged = TRUE;
	    *p = (char)tolower (*p);
	}

	if (VI_CurrentColumn < VI_InputLength - 1)
	{
	    VI_CurrentColumn++;
	    p++;
	}

	else
	    break;
    }

    return TRUE;
}

/*
 * Completion functions
 */

static bool F_LOCAL VI_ExecuteCompletion (char *cmd)
{
    int		rval = 0;
    int		start, end;
    char	**FileList;
    char	**ap;
    int		Count;

    if (isspace (ConsoleLineBuffer[VI_CurrentColumn]))
	return FALSE;

    start = VI_CurrentColumn;

    while (start > -1 && !isspace (ConsoleLineBuffer[start]))
	start--;

/* Get the file name */

    start++;
    end = VI_CurrentColumn;

    while ((end < VI_InputLength) && !isspace (ConsoleLineBuffer[end]))
	end++;

/* Build the list of file names */

    if ((FileList = BuildCompletionList (&ConsoleLineBuffer[start],
					 end - start,
    					 &Count, C2bool (*cmd != '=')))
		  == (char **)NULL)
	return FALSE;

/* Display the directory contents */

    if (*cmd == '=')
    {
	feputc (CHAR_NEW_LINE);
	PrintAList (Count, FileList);
	ReleaseAList (FileList);
	FlushStreams ();

	if (VI_InputLength != 0)
	    VI_CurrentColumn++;

	VI_RedrawLine ();
	vi_Insert = VI_INSERT_MODE;
	vi_State = VI_S_NORMAL;
	return TRUE;
    }

/* Insert a list of files or the completion */

    ap = FileList;

/*
* If completion, get the common part and check the length to see if
* we've go some new data
*/

    if ((*cmd == '\\') && (Count > 1) &&
    	(GetCommonPartOfFileList (FileList) <= (size_t)(end - start)))
    {
	ReleaseAList (FileList);
	return FALSE;
    }

/* Delete the old name and insert the new */

    VI_DeleteRange (start, end);
    VI_CurrentColumn = start;

/* For each file name, insert it into the command line.  In the case of
 * completion, we only use the first name, because that has the common
 * part
 */

    while (TRUE)
    {
	if (VI_InsertIntoBuffer (*ap, strlen (*ap), FALSE) != 0)
	{
	    rval = -1;
	    break;
	}

/* If there was only 1 match on completion and is a directory, append a
 * directory
 */
	if ((*cmd == '\\') && (Count == 1) && IsDirectory (*ap) &&
	    (VI_InsertIntoBuffer (DirectorySeparator, 1, FALSE) != 0))
	    rval = -1;

/* If no more or completion - stop */

	if ((*cmd == '\\') || (*(++ap) == (char *)NULL))
	    break;

	if (VI_InsertIntoBuffer (" ", 1, FALSE) != 0)
	{
	    rval = -1;
	    break;
	}
    }

    ReleaseAList (FileList);
    vi_InputBufferChanged = TRUE;
    vi_Insert = VI_INSERT_MODE;
    VI_Refresh (FALSE);

    return C2bool (rval == 0);
}

/*
 * Handle the change, delete and yank commands (c, d, y)
 */

static bool F_LOCAL VI_ChangeCommand (int argcnt, char *cmd)
{
    int		c2;
    int		c3 = 0;
    int		ncursor;

    if (*cmd == cmd[1])
	c2 = VI_InputLength;

    else if (!VI_IsMove ((int)cmd[1]))
	return FALSE;

    else
    {
	if ((ncursor = VI_ExecuteMove (argcnt, &cmd[1], TRUE)) < 0)
	    return FALSE;

	if ((*cmd == 'c') && ((cmd[1] == 'w') || (cmd[1] == 'W')) &&
	    !isspace (ConsoleLineBuffer[VI_CurrentColumn]))
	{
	    while (isspace (ConsoleLineBuffer[--ncursor]))
		continue;

	    ncursor++;
	}

	if (ncursor > VI_CurrentColumn)
	{
	    c3 = VI_CurrentColumn;
	    c2 = ncursor;
	}

	else
	{
	    c3 = ncursor;
	    c2 = VI_CurrentColumn;
	}
    }

    if ((*cmd != 'c') && (c3 != c2))
	VI_YankSelection (c3, c2);

    if (*cmd != 'y')
    {
	VI_DeleteRange (c3, c2);
	VI_CurrentColumn = c3;
    }

    if (*cmd == 'c')
    {
	vi_InputBufferChanged = TRUE;
	vi_Insert = VI_INSERT_MODE;
    }

    return TRUE;
}

/*
 * Handle the Put commands (p, P)
 */

static bool F_LOCAL VI_CommandPut (int argcnt, char cmd)
{
    int	YBLen;

    if ((cmd == 'p') && (VI_InputLength != 0))
	VI_CurrentColumn++;

    if (vi_YankBuffer == (char *)NULL)
	return FALSE;

    vi_InputBufferChanged = TRUE;
    YBLen = strlen (vi_YankBuffer);

    while ((VI_InsertIntoBuffer (vi_YankBuffer, YBLen, FALSE) == 0) &&
	   (--argcnt > 0))
	continue;

    if (VI_CurrentColumn != 0)
	VI_CurrentColumn--;

    return C2bool (argcnt == 0);
}

/*
 * Save undo buffer
 */

static void F_LOCAL VI_SaveUndoBuffer (int argcnt, char *cmd)
{
    if (vi_UndoBuffer != null)
	ReleaseMemoryCell (vi_UndoBuffer);

    ConsoleLineBuffer[VI_InputLength] = 0;

    vi_UndoBuffer = StringSave (ConsoleLineBuffer);
    vi_UndoState  = vi_EditorState;

    vi_PrevCmdArgCount = argcnt;
    memmove (vi_PreviousCommand, cmd, VI_MAX_CMD_LENGTH);
}


/*
 * Edit the line using VI
 */

static bool F_LOCAL VI_EditLine (int argcnt)
{
    char		*Temp;
    char		*NewArg[3];
    char		*hptr;
    FILE		*fp;
    int			fd;
    void		(*save_signal)(int);

/* Get the current signal setting */

    save_signal = signal (SIGINT, SIG_IGN);
    signal (SIGINT, save_signal);

    if ((hptr = (argcnt) ? GetHistoryRecord (argcnt) : ConsoleLineBuffer)
	      == (char *)NULL)
        return FALSE;

    fputchar (CHAR_NEW_LINE);

/* Check status */

    if ((fp = FOpenFile ((Temp = GenerateTemporaryFileName ()),
			sOpenWriteBinaryMode)) == (FILE *)NULL)
    {
	PrintWarningMessage ("cannot create %s", Temp);
	return FALSE;
    }

    fputs (hptr, fp);
    fputc (CHAR_NEW_LINE, fp);
    CloseFile (fp);

/* Invoke the editor */

    if (((NewArg[0] = GetVariableAsString (VisualVariable, FALSE)) == null) &&
	((NewArg[0] = GetVariableAsString (EditorVariable, FALSE)) == null))
	NewArg[0] = "vi";

    NewArg[1] = Temp;
    NewArg[2] = (char *)NULL;

    if (ExecuteACommand (NewArg, 0) == -1)
    {
	unlink (Temp);
	signal (SIGINT, save_signal);
	return FALSE;
    }

/* Restore signals which are changed by ExecuteACommand */

    signal (SIGINT, save_signal);

/* Now execute it */

    if ((fd = S_open (TRUE, Temp, O_RMASK)) < 0)
    {
	unlink (Temp);
	PrintWarningMessage ("cannot re-open edit file (%s)", Temp);
	return FALSE;
    }

    argcnt = read (fd, ConsoleLineBuffer, LINE_MAX - 1);
    S_close (fd, TRUE);

    if (argcnt <= 0)
	return FALSE;

/* Strip off trailing EOFs and EOLs */

    CleanUpBuffer (argcnt, ConsoleLineBuffer, 0x1a);
    VI_InputLength		    = strlen (ConsoleLineBuffer);
    VI_CurrentColumn		    = 0;
    vi_EditorState.WindowLeftColumn = 0;
    vi_InputBufferChanged = TRUE;
    return TRUE;
}

/*
 * Move through the history file
 */

static bool F_LOCAL VI_MoveThroughHistory (int argcnt)
{
    if (!VI_GetEventFromHistory (vi_InputBufferChanged,
				 CurrentHistoryEvent + argcnt))
	return FALSE;

    vi_InputBufferChanged = FALSE;
    CurrentHistoryEvent += argcnt;
    return TRUE;
}

/*
 * Execute a move
 */

static int F_LOCAL VI_ExecuteMove (int argcnt, char *cmd, bool sub)
{
    int		ncursor = 0;

    switch (*cmd)
    {
	case '|':
	    if (argcnt > VI_InputLength)
	        return -1;

	    ncursor = argcnt;
	    break;

	case 'b':
	    if ((!sub) && (VI_CurrentColumn == 0))
		return -1;

	    ncursor = VI_BackwardWord (argcnt);
	    break;

	case 'B':
	    if ((!sub) && (VI_CurrentColumn == 0))
		return -1;

	    ncursor = VI_BackwardToWhiteSpace (argcnt);
	    break;

	case 'e':
	    if ((!sub) &&
	        (VI_CurrentColumn + 1 >= VI_InputLength))
		return -1;

	    ncursor = VI_EndofWord (argcnt);

	    if (sub)
		ncursor++;

	    break;

	case 'E':
	    if ((!sub) &&
	        (VI_CurrentColumn + 1 >= VI_InputLength))
		return -1;

	    ncursor = VI_ForwardToEndOfNonWhiteSpace (argcnt);

	    if (sub)
		ncursor++;

	    break;

	case 'f':
	case 'F':
	case 't':
	case 'T':
	    vi_LastFindCommand = *cmd;
	    vi_LastFindCharacter = cmd[1];
		/* drop through */

/* XXX -- should handle \^ escape? */
	case ';':
        {
	    bool	incr;
	    bool	forward;

	    if (vi_LastFindCommand == CHAR_SPACE)
		return -1;

	    incr = C2bool ((vi_LastFindCommand == 'f') ||
	    		   (vi_LastFindCommand == 'F'));
	    forward = C2bool (vi_LastFindCommand > 'a');

	    if (*cmd == ',')
		forward = (bool)!forward;

	    if ((ncursor = VI_FindCharacter (vi_LastFindCharacter,
	    			   	     argcnt, forward, incr)) < 0)
		return -1;

	    if (sub && forward)
		ncursor++;

	    break;
	}

	case 'h':
		/* tmp fix */
	case CHAR_BACKSPACE:
	    if (!sub && (VI_CurrentColumn == 0))
		return -1;

	    if ((ncursor = VI_CurrentColumn - argcnt) < 0)
		ncursor = 0;

	    break;

	case CHAR_SPACE:
	case 'l':
	    if (!sub && (VI_CurrentColumn + 1 >= VI_InputLength))
		return -1;

	    if (VI_InputLength != 0)
	    {
		ncursor = VI_CurrentColumn + argcnt;

		if (ncursor >= VI_InputLength)
		    ncursor = VI_InputLength - 1;
	    }

	    break;

	case 'w':
	    if (!sub && VI_CurrentColumn + 1 >= VI_InputLength)
		return -1;

	    ncursor = VI_ForwardWord (argcnt);
	    break;

	case 'W':
	    if (!sub && (VI_CurrentColumn + 1 >= VI_InputLength))
		return -1;

	    ncursor = VI_ForwardToWhiteSpace (argcnt);
	    break;

	case '0':
	    ncursor = 0;
	    break;

	case CHAR_BEGIN_LINE:
	    ncursor = 0;
	    while ((ncursor < VI_InputLength - 1) &&
	           isspace (ConsoleLineBuffer[ncursor]))
		ncursor++;

	    break;

	case CHAR_END_LINE:
	    if (VI_InputLength != 0)
		ncursor = VI_InputLength;

	    else
		ncursor = 0;

	    break;

	case '%':
        {
	    int		bcount;
	    int		i;
	    int		t;

	    ncursor = VI_CurrentColumn;

	    while ((ncursor < VI_InputLength) &&
		   (i = VI_GetBracketType (ConsoleLineBuffer[ncursor])) == 0)
		ncursor++;

	    if (ncursor == VI_InputLength)
		return -1;

	    bcount = 1;

	    do
	    {
		if (i > 0)
		{
		    if (++ncursor >= VI_InputLength)
			return -1;
		}

		else if (--ncursor < 0)
		    return -1;

		if ((t = VI_GetBracketType (ConsoleLineBuffer[ncursor]))
		       == 1)
		    bcount++;

		else if (t == -i)
		    bcount--;

	    } while (bcount != 0);

	    if (sub)
		ncursor++;

	    break;
	}

	default:
	    return -1;
    }

    return ncursor;
}

static int F_LOCAL VI_RedoInsert (int count)
{
    while (count-- > 0)
    {
	if (VI_InsertIntoBuffer (vi_InsertBuffer, vi_InsertBufferLength,
		    C2bool (vi_Insert == VI_REPLACE_MODE)) != 0)
	    return -1;
    }

    if (VI_CurrentColumn > 0)
	VI_CurrentColumn--;

    vi_Insert = VI_UNDEF_MODE;
    return 0;
}

static void F_LOCAL VI_YankSelection (int a, int b)
{
    int		len = b - a;

    if (!len)
        return;

    if (vi_YankBuffer != (char *)NULL)
	ReleaseMemoryCell (vi_YankBuffer);

    vi_YankBuffer = GetAllocatedSpace (len + 1);
    SetMemoryAreaNumber (vi_YankBuffer, 0);
    memcpy (vi_YankBuffer, &ConsoleLineBuffer[a], len);
    vi_YankBuffer[len] = 0;
}

/*
 * Get the Bracket type
 */

static int F_LOCAL VI_GetBracketType (int ch)
{
    switch (ch)
    {
	case CHAR_OPEN_PARATHENSIS:
	    return 1;

	case CHAR_OPEN_BRACKETS:
	    return 2;

	case CHAR_OPEN_BRACES:
	    return 3;

	case CHAR_CLOSE_PARATHENSIS:
	    return -1;

	case CHAR_CLOSE_BRACKETS:
	    return -2;

	case CHAR_CLOSE_BRACES:
	    return -3;

	default:
	    return 0;
    }
}

/*
 * Save and Restore the Input line in the Hold buffer
 */

static void F_LOCAL VI_CopyInput2Hold (void)
{
    if (vi_HoldBuffer != null)
	ReleaseMemoryCell (vi_HoldBuffer);

    vi_HoldBuffer = null;
    vi_HoldBuffer = StringSave (ConsoleLineBuffer);
}

static void F_LOCAL VI_CopyHold2Input (void)
{
    VI_CurrentColumn = 0;
    strcpy (ConsoleLineBuffer, vi_HoldBuffer);
    VI_InputLength = strlen (ConsoleLineBuffer);
}

/*
 * Insert the String into the input buffer
 */

static int F_LOCAL VI_InsertIntoBuffer (char *buf, int len, bool repl)
{
    if (len == 0)
	return 0;

    if (repl)
    {
	if ((VI_CurrentColumn + len) >= LINE_MAX)
	    return -1;

	if ((VI_CurrentColumn + len) > VI_InputLength)
	    VI_InputLength = VI_CurrentColumn + len;
    }

    else
    {
	if ((VI_InputLength + len) >= LINE_MAX)
	    return -1;

	memmove (&ConsoleLineBuffer[VI_CurrentColumn + len],
		 &ConsoleLineBuffer[VI_CurrentColumn],
		 VI_InputLength - VI_CurrentColumn);

	VI_InputLength += len;
    }

    memmove (&ConsoleLineBuffer[VI_CurrentColumn], buf, len);
    VI_CurrentColumn += len;
    ConsoleLineBuffer[VI_InputLength] = 0;
    return 0;
}

/*
 * Delete a range of characters from the input buffer
 */

static void F_LOCAL VI_DeleteRange (int a, int b)
{
    if (VI_InputLength != b)
	memmove (&ConsoleLineBuffer[a],
		 &ConsoleLineBuffer[b],
		 VI_InputLength - b);

     ConsoleLineBuffer[VI_InputLength -= b - a] = 0;
}

static int F_LOCAL VI_FindCharacter (int ch, int cnt, bool forw, bool incl)
{
    int		ncursor;

    if (VI_InputLength == 0)
	return -1;

    ncursor = VI_CurrentColumn;

    while (cnt--)
    {
	do
	{
	    if (forw)
	    {
		if (++ncursor == VI_InputLength)
		    return -1;
	    }

	    else if (--ncursor < 0)
		return -1;

	} while (ConsoleLineBuffer[ncursor] != (char)ch);
    }

    if (!incl)
    {
	if (forw)
	    ncursor--;

	else
	    ncursor++;
    }

    return ncursor;
}

/*
 * Move forward to next white space character
 */

static int F_LOCAL VI_ForwardToWhiteSpace (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while ((ncursor < VI_InputLength) && argcnt--)
    {
	while (!isspace (ConsoleLineBuffer[ncursor]) &&
	       (++ncursor < VI_InputLength))
	    continue;

	while (isspace (ConsoleLineBuffer[ncursor]) &&
	       (++ncursor < VI_InputLength))
	    continue;
    }

    return ncursor;
}

/*
 * Move forward to start of next word
 */

static int F_LOCAL VI_ForwardWord (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while (ncursor < VI_InputLength && argcnt--)
    {
	if (IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]))
	{
	    while (IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]) &&
	           (++ncursor < VI_InputLength))
		continue;
	}

	else if (!isspace (ConsoleLineBuffer[ncursor]))
	{
	    while (!IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]) &&
	           !isspace (ConsoleLineBuffer[ncursor]) &&
		   (++ncursor < VI_InputLength))
		continue;
	}

	while (isspace (ConsoleLineBuffer[ncursor]) &&
	       (++ncursor < VI_InputLength))
	    continue;
    }

    return ncursor;
}

/*
 * Move backward to start of word
 */

static int F_LOCAL VI_BackwardWord (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while (ncursor > 0 && argcnt--)
    {
	while ((--ncursor > 0) && isspace (ConsoleLineBuffer[ncursor]))
	    continue;

	if (ncursor > 0)
	{
	    if (IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]))
	    {
		while ((--ncursor >= 0) &&
		       IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]))
		    continue;
	    }

	    else
	    {
		while ((--ncursor >= 0) &&
		       !IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]) &&
		       !isspace (ConsoleLineBuffer[ncursor]))
		    continue;
	    }

	    ncursor++;
	}
    }

    return ncursor;
}

/*
 * Move to the end of the word
 */

static int F_LOCAL VI_EndofWord (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while ((ncursor < VI_InputLength) && argcnt--)
    {
	while ((++ncursor < VI_InputLength - 1) &&
	       isspace (ConsoleLineBuffer[ncursor]))
	    continue;

	if (ncursor < VI_InputLength - 1)
	{
	    if (IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]))
	    {
		while ((++ncursor < VI_InputLength) &&
		       IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]))
		    continue;
	    }

	    else
	    {
		while ((++ncursor < VI_InputLength) &&
		       !IS_AlphaNumeric ((int)ConsoleLineBuffer[ncursor]) &&
		       !isspace (ConsoleLineBuffer[ncursor]))
		    continue;
	    }

	    ncursor--;
	}
    }

    return ncursor;
}

/*
 * Move backward to previous white space
 */

static int F_LOCAL VI_BackwardToWhiteSpace (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while ((ncursor > 0) && argcnt--)
    {
	while ((--ncursor >= 0) && isspace (ConsoleLineBuffer[ncursor]))
	    continue;

	while ((ncursor >= 0) && !isspace (ConsoleLineBuffer[ncursor]))
	    ncursor--;

	ncursor++;
    }

    return ncursor;
}

/*
 * Move to end of non-white space
 */

static int F_LOCAL VI_ForwardToEndOfNonWhiteSpace (int argcnt)
{
    int		 ncursor = VI_CurrentColumn;

    while (ncursor < VI_InputLength - 1 && argcnt--)
    {
	while ((++ncursor < VI_InputLength - 1) &&
	       isspace (ConsoleLineBuffer[ncursor]))
	    continue;

	if (ncursor < VI_InputLength - 1)
	{
	    while (++ncursor < VI_InputLength &&
	           !isspace (ConsoleLineBuffer[ncursor]))
		continue;

	    ncursor--;
	}
    }

    return ncursor;
}

/*
 * Get a specific history event
 */

static bool F_LOCAL VI_GetEventFromHistory (bool save, int n)
{
    char	*hptr;
    int		lasthistory;

    if ((n < 0) || (n > (lasthistory = GetLastHistoryEvent ())))
	return FALSE;

    if (n == lasthistory)
    {
	VI_CopyHold2Input ();
	return TRUE;
    }

    if ((hptr = GetHistoryRecord (n)) == (char *)NULL)
	return FALSE;

    if (save)
	VI_CopyInput2Hold ();

    strcpy (ConsoleLineBuffer, hptr);
    VI_InputLength = strlen (hptr);
    VI_CurrentColumn = 0;
    return TRUE;
}

/*
 * Search the history for an event
 */

static int F_LOCAL VI_FindEventFromHistory (bool save, int start, bool fwd,
					    char *pat)
{
    char	*hptr;
    int		lev = GetLastHistoryEvent ();
    int		dir = (fwd) ? 1 : -1;

/* If we are going backwards through the history (from the current event),
 * check 1. that we are not at the end of events (last) and 2) not at the
 * next.
 */

    if (!fwd)
    {
        if (start == 0)
	    return -1;
    }

/* Otherwise, going forward to the future.  If the future is not here, give
 * up
 */

    else if (start >= lev)
        return -1;

    start += dir;

/* Search for match */

    while ((hptr = GetHistoryRecord (start)) != (char *)NULL)
    {

/* If ^, then search for line beginning with string */

	if (*pat == CHAR_BEGIN_LINE)
	{
	   if (strcmp (hptr, pat + 1) == 0)
	       break;
	}

/* Else just check for the string */

	else if (strstr (hptr, pat) != (char *)NULL)
	    break;

	start += dir;
    }

    if (hptr == (char *)NULL)
    {
	if ((start != 0) && fwd && strcmp (vi_HoldBuffer, pat) >= 0)
	{
	    VI_CopyHold2Input ();
	    return 0;
	}

	else
	    return -1;
    }

    if (save)
	VI_CopyInput2Hold ();

    memcpy (ConsoleLineBuffer, hptr, (VI_InputLength = strlen (hptr)));
    ConsoleLineBuffer[VI_InputLength] = 0;
    VI_CurrentColumn = 0;
    return start;
}

/*
 * Redraw the current line
 */

static void F_LOCAL VI_RedrawLine (void)
{
    GEN_PutACharacter (CHAR_NEW_LINE);
    VI_OutputPrompt (TRUE);

    vi_MoreIndicator = CHAR_SPACE;
    VI_CreateWindowBuffers ();
}

/*
 * Re-create the WindowBuffers
 */

static void F_LOCAL VI_CreateWindowBuffers (void)
{
    int		c;

    GetScreenParameters ();
    PromptWidth = (StartCursorPosition = ReadCursorPosition ()) %
			MaximumColumns;
    CurrentScreenPosition = PromptWidth;
    WindowWidth = MaximumColumns - PromptWidth - 3;
    
    for (c = 0; c < 2; c++)
    {
	if (WindowBuffer[c] != (char *)NULL)
	    ReleaseMemoryCell (WindowBuffer[c]);

	WindowBuffer[c] = GetAllocatedSpace (MaximumColumns + 1);
	SetMemoryAreaNumber (WindowBuffer[c], 0);
	memset (WindowBuffer[c], CHAR_SPACE, MaximumColumns + 1);
    }
}

/*
 * Redisplay line
 */

static void F_LOCAL VI_OutputPrompt (bool Prompt)
{
    GEN_PutACharacter (CHAR_RETURN);
    FlushStreams ();

    if (Prompt)
    {
	OutputUserPrompt (LastUserPrompt);
	StartCursorPosition = ReadCursorPosition ();
    }

    else
	SetCursorPosition (StartCursorPosition);

    PromptWidth = StartCursorPosition % MaximumColumns;
    CurrentScreenPosition = PromptWidth;
}

/*
 * Refresh the current line on the screen
 */

static void F_LOCAL VI_Refresh (bool leftside)
{
    if (VI_OutOfWindow ())
	VI_ReWindowBuffer ();

    VI_DisplayWindow (WindowBuffer[1 - vi_WhichWindow],
		      WindowBuffer[vi_WhichWindow], leftside);
    vi_WhichWindow = 1 - vi_WhichWindow;
}

/*
 * Check to see if we are outside the current window
 */
static bool F_LOCAL VI_OutOfWindow (void)
{
    int	cur, col;

    if (VI_CurrentColumn < vi_EditorState.WindowLeftColumn)
	return TRUE;

    col = 0;
    cur = vi_EditorState.WindowLeftColumn;

    while (cur < VI_CurrentColumn)
	col = VI_AdvanceColumn (ConsoleLineBuffer[cur++], col);

    return (col > WindowWidth) ? TRUE : FALSE;
}

static void F_LOCAL VI_ReWindowBuffer (void)
{
    int		tcur = 0;
    int		tcol = 0;
    int		holdcur1 = 0;
    int		holdcol1 = 0;
    int		holdcur2 = 0;
    int		holdcol2 = 0;

    while (tcur < VI_CurrentColumn)
    {
	if (tcol - holdcol2 > WindowWidth / 2)
	{
	    holdcur1 = holdcur2;
	    holdcol1 = holdcol2;
	    holdcur2 = tcur;
	    holdcol2 = tcol;
	}

	tcol = VI_AdvanceColumn (ConsoleLineBuffer[tcur++], tcol);
    }

    while (tcol - holdcol1 > WindowWidth / 2)
	holdcol1 = VI_AdvanceColumn (ConsoleLineBuffer[holdcur1++], holdcol1);

    vi_EditorState.WindowLeftColumn = holdcur1;
}

/*
 * Advance to column n
 */

static int F_LOCAL VI_AdvanceColumn (int ch, int col)
{
    if ((ch >= CHAR_SPACE) && (ch < 0x7f))
	return col + 1;

    else if (ch == CHAR_TAB)
	return (col | 7) + 1;

    else
	return col + 2;
}

static void F_LOCAL VI_DisplayWindow (char *wb1, char *wb2, bool leftside)
{
    char	*twb1 = wb1;
    char	*twb2;
    char	mc;
    int		cur = vi_EditorState.WindowLeftColumn;
    int		col = 0;
    int		cnt;
    int		ncol = 0;
    int		moreright = 0;

    while ((col < WindowWidth) && (cur < VI_InputLength))
    {
	if ((cur == VI_CurrentColumn) && leftside)
	    ncol = col + PromptWidth;

	if ((ConsoleLineBuffer[cur] < CHAR_SPACE) ||
	    (ConsoleLineBuffer[cur] == 0x7f))
	{
	    if (ConsoleLineBuffer[cur] == CHAR_TAB)
	    {
		do
		{
		    *(twb1++) = CHAR_SPACE;
		} while ((++col < WindowWidth) && ((col & 7) != 0));
	    }

	    else
	    {
		*(twb1++) = '^';

		if (++col < WindowWidth)
		{
		    *(twb1++) = (char)(ConsoleLineBuffer[cur] ^ '@');
		    col++;
		}
	    }
	}

	else
	{
	    *(twb1++) = ConsoleLineBuffer[cur];
	    col++;
	}

	if ((cur == VI_CurrentColumn) && !leftside)
	    ncol = col + PromptWidth - 1;

	cur++;
    }

    if (cur == VI_CurrentColumn)
	ncol = col + PromptWidth;

    if (col < WindowWidth)
    {
	while (col < WindowWidth)
	{
	    *(twb1++) = CHAR_SPACE;
	    col++;
	}
    }

    else
	moreright++;

    *twb1 = CHAR_SPACE;

    col = PromptWidth;
    cnt = WindowWidth;
    twb1 = wb1;
    twb2 = wb2;

    while (cnt--)
    {
	if (*twb1 != *twb2)
	{
	    if (CurrentScreenPosition != col)
		VI_MoveToColumn (col, wb1);

	    GEN_PutACharacter (*twb1);
	    CurrentScreenPosition++;
	}

	twb1++;
	twb2++;
	col++;
    }

    if ((vi_EditorState.WindowLeftColumn > 0) && moreright)
	mc = CHAR_PLUS;

    else if (vi_EditorState.WindowLeftColumn > 0)
	mc = '<';

    else if (moreright)
	mc = '>';

    else
	mc = CHAR_SPACE;

    if (mc != vi_MoreIndicator)
    {
	VI_MoveToColumn (MaximumColumns - 2, wb1);
	GEN_PutACharacter (mc);
	CurrentScreenPosition++;
	vi_MoreIndicator = mc;
    }

#if 0
/*
 * Hack to fix the ^r redraw problem, but it redraws way too much.
 * Probably unacceptable at low baudrates.  Someone please fix this
 */
    else
    {
	VI_MoveToColumn (MaximumColumns - 2, wb1);
    }
#endif

    if (CurrentScreenPosition != ncol)
	VI_MoveToColumn (ncol, wb1);
}

/*
 * Move to a specific column
 */

static void F_LOCAL VI_MoveToColumn (int col, char *wb)
{
    if (col < CurrentScreenPosition)
    {
	if (col + 1 < CurrentScreenPosition - col)
	{
	    VI_OutputPrompt (FALSE);

	    while (CurrentScreenPosition++ < col)
		GEN_PutACharacter (*(wb++));
	}

	else
	{
	    while (CurrentScreenPosition-- > col)
		GEN_PutACharacter (CHAR_BACKSPACE);
	}
    }

    else
    {
	wb = &wb[CurrentScreenPosition - PromptWidth];

	while (CurrentScreenPosition++ < col)
	    GEN_PutACharacter (*(wb++));
    }

    CurrentScreenPosition = col;
}

/*
 * Main loop for VI editing
 */

static int F_LOCAL VI_MainLoop (void)
{
    int			c;

/* Initialise */

    vi_State = VI_S_NORMAL;
    vi_Insert = VI_INSERT_MODE;

    vi_PreviousCommand[0] = 'a';
    vi_PrevCmdArgCount = 1;
    vi_InsertBufferLength = 0;
    vi_InputBufferChanged = TRUE;

/* Initialise Yank Buffer */

    if (vi_YankBuffer != (char *)NULL)
	ReleaseMemoryCell (vi_YankBuffer);

    vi_YankBuffer = (char *)NULL;

    if (vi_HoldBuffer != null)
	ReleaseMemoryCell (vi_HoldBuffer);

    vi_HoldBuffer = null;

/* Release Alias input */

    vi_AliasBuffer = (char *)NULL;

/* Reset the VI edit information */

    VI_InputLength		    = 0;
    VI_CurrentColumn		    = 0;
    vi_EditorState.WindowLeftColumn = 0;

    if (vi_UndoBuffer != null)
	ReleaseMemoryCell (vi_UndoBuffer);

    vi_UndoBuffer		  = null;
    vi_UndoState.InputLength      = 0;
    vi_UndoState.CursorColumn     = 0;
    vi_UndoState.WindowLeftColumn = 0;

    /* docap(CLR_EOL, 0); */

    vi_WhichWindow = 0;
    vi_MoreIndicator = CHAR_SPACE;

/* Initialise the window buffers */

    VI_CreateWindowBuffers ();

/* Get the input from the user */

    FlushStreams ();

    while ((c = VI_GetNextCharacter ()) != -1)
    {
	if (VI_StateMachine (c))
	    break;

	FlushStreams ();
    }

    SetCursorShape (FALSE);

/* Check for error */

    if (c == -1)
	return -1;

/* Ensure line is terminated */

    ConsoleLineBuffer[VI_InputLength] = 0;
    return VI_InputLength;
}

/*
 * Get next character
 */

static int F_LOCAL VI_GetNextCharacter (void)
{
    unsigned char	a_key;
    unsigned char	f_key;
    int			i;

    SetCursorShape (C2bool ((vi_State == VI_S_NORMAL) &&
			    (vi_Insert == VI_INSERT_MODE)));

    do
    {
	if (vi_AliasBuffer != (char *)NULL)
	{
	    f_key = 0;

	    if ((a_key = *(vi_AliasBuffer++)) == 0)
		vi_AliasBuffer = (char *)NULL;
	}

	if (vi_AliasBuffer == (char *)NULL)
	    a_key = ReadKeyBoard (&f_key);

/* Only map when we are not inserting or replacing */

	if ((vi_Insert == VI_UNDEF_MODE) && (vi_State != VI_S_SEARCH))
	{
	    if (!(i = LookUpKeyBoardFunction (a_key, f_key)))
		continue;

	    a_key = (i > 0) ? (unsigned char) i : VI_IniMapping[(-i) - 1];
	}

/* Treate function keys are bad */

	else if (a_key == KT_ALTFUNCTION)
	    a_key = 0;

	if (a_key == KT_FUNCTION)
	    RingWarningBell ();

    } while (!a_key);

    return (a_key == (unsigned char)GetEOFKey ()) ? -1 : a_key;
}
#endif

/*
 * Read an edited command line.  This is only called from SH9 if emacs or
 * vi mode is set.
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)
int	EditorInput (void)
{

/*
 * Check that we have set up (EMACS only)
 */

#  if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
    if (emacs_KeyDefinitions == NULL)
	EMACS_Initialisation ();
#  endif

/* Initialise history pointer */

    CurrentHistoryEvent = GetLastHistoryEvent ();

/* EMACS editing ? */

#  if defined (FLAGS_GMACS)
    if (ShellGlobalFlags & FLAGS_GMACS)
	return EMACS_MainLoop ();
#  endif


/* GMACS editing ? */

#  if defined (FLAGS_EMACS)
    if (ShellGlobalFlags & FLAGS_EMACS)
	return EMACS_MainLoop ();
#  endif

/* VI editing ? */

#  ifdef FLAGS_VI
    if (ShellGlobalFlags & FLAGS_VI)
	return VI_MainLoop ();
#  endif

    return -1;
}
#endif


/*
 * EMACS Functions
 *
 * EMACS Keyboard Input
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
static int F_LOCAL EMACS_GetNextCharacter (void)
{
    static unsigned char	LastFkey = 0;
    unsigned char		f_key;
    unsigned char		a_key;

    a_key = (emacs_UnGetCharacter != -1)
		? (unsigned char)emacs_UnGetCharacter
		: ((LastFkey)
		    ? LastFkey
		    : ReadKeyBoard (&f_key));

    emacs_UnGetCharacter = -1;

/* If we got a function key, return 0xE0 and save the function key id */

    if ((a_key == KT_FUNCTION) || (a_key == KT_ALTFUNCTION))
    {
	LastFkey = f_key;
	return 0xE0;
    }

/* If we ungot 0xE0, return it again! */

    else if (a_key == 0xE0)
	return 0xE0;

/* Otherwise, return the key and clear the saved function key id */

    else
    {
	LastFkey = 0;
	return a_key;
    }
}

/*
 * Get next non-function keycode
 */

static int F_LOCAL EMACS_GetNonFunctionKey (void)
{
    int		c;

    SetCursorShape (FALSE);

    while ((c = EMACS_GetNextCharacter ()) == 0xE0)
    {
	EMACS_GetNextCharacter ();
	RingWarningBell ();
    }

    return c;
}

/*
 * The EMACS Main Loop
 */

static int F_LOCAL EMACS_MainLoop (void)
{
    int		c;
    int		i;
    int 	(F_LOCAL *func)(int);

    EMACS_ResetInput ();

    emacs_MarkPointer = (char *)NULL;
    emacs_CurrentPrefix = 0;
    emacs_CurrentMacroString = null;
    emacs_UnGetCharacter = -1;
    emacs_ArgumentCount = 0;

    if (emacs_NextCommandIs != -1)
    {
	EMACS_LoadFromHistory (emacs_NextCommandIs);
	emacs_NextCommandIs = -1;
    }

    CurrentScreenColumn = ReadCursorPosition () % MaximumColumns;

    AdjustOK = TRUE;
    DisplayWidth = MaximumColumns - 2 - CurrentScreenColumn;
    AdjustDone = 0;

    while (1)
    {
	FlushStreams ();

	if (*emacs_CurrentMacroString)
	{
	    c = *(emacs_CurrentMacroString++);

	    if (*emacs_CurrentMacroString == 0)
		emacs_CurrentMacroString = null;
	}

	else
        {
	    SetCursorShape (TRUE);
	    c = EMACS_GetNextCharacter ();
	}

	func = (emacs_CurrentPrefix == -1)
		? EMACS_AutoInsert
		: emacs_KeyDefinitions[emacs_CurrentPrefix][c & 0x0ff]->xf_func;

	if (func == NULL)
	    func = EMACS_Error;

	i = c | (emacs_CurrentPrefix << 8);

	emacs_CurrentPrefix = 0;

	switch (i = (*func)(i))
	{
	    case EMACS_KEY_NORMAL:
		emacs_LastCommand = func;

	    case EMACS_KEY_META:
	    case EMACS_KEY_NOOP:
		break;

	    case EMACS_KEY_EOL:
		i = emacs_EndOfLine - ConsoleLineBuffer;
		emacs_LastCommand = (int (F_LOCAL *)(int))NULL;
		return i;

	    case EMACS_KEY_INTERRUPT:	/* special case for interrupt */
		raise (SIGINT);
		return -1;
	}
    }
}

/*
 * Simply causes the character to appear as literal input.  (Most ordinary
 * characters are bound to this.)
 */

static int F_LOCAL EMACS_AutoInsert (int c)
{
    char	str[2];

/* Should allow tab and control chars.  */

    if (c == 0)
	return EMACS_Error (0);

    str[0] = (char)c;
    str[1] = 0;
    EMACS_InsertString (str);

    return EMACS_KEY_NORMAL;
}

/*
 * Insert macro
 */

static int F_LOCAL EMACS_InsertMacroString (int c)
{
    if (*emacs_CurrentMacroString)
	return EMACS_Error (0);

    emacs_CurrentMacroString = emacs_MacroDefinitions[c>>8][c & 0x0ff];
    return EMACS_KEY_NORMAL;
}

static void F_LOCAL EMACS_InsertString (char *cp)
{
    int		count = strlen(cp);
    int		adj = AdjustDone;

    if ((emacs_EndOfLine + count) >= (ConsoleLineBuffer + LINE_MAX))
    {
	RingWarningBell ();
	return;
    }

    if (emacs_CurrentPosition != emacs_EndOfLine)
	memmove (emacs_CurrentPosition + count, emacs_CurrentPosition,
		 emacs_EndOfLine - emacs_CurrentPosition + 1);

    else
	emacs_CurrentPosition[count] = 0;

    memmove (emacs_CurrentPosition, cp, count);

/*
 * GEN_AdjustOutputString() may result in a call to GEN_AdjustRedraw ()
 * we want emacs_CurrentPosition to reflect the new position.
 */
    cp = emacs_CurrentPosition;
    emacs_CurrentPosition += count;
    *(emacs_EndOfLine += count) = 0;
    LastVisibleCharValid = FALSE;
    GEN_FindLastVisibleCharacter ();
    AdjustOK = C2bool (emacs_CurrentPosition >= emacs_LastVisibleCharacter);
    GEN_AdjustOutputString (cp);

    if (adj == AdjustDone)	/* has GEN_AdjustRedraw () been called? */
    {
      /* no */
	for (cp = emacs_LastVisibleCharacter; cp > emacs_CurrentPosition; )
	    GEN_BackspaceOver (*--cp);
    }

    AdjustOK = TRUE;
}

/*
 * Check the argument count against either the start or end of line
 */

static int F_LOCAL EMACS_RetreatNCharacters (void)
{
    EMACS_CheckArgCount ();

    if ((emacs_CurrentPosition - ConsoleLineBuffer) < emacs_ArgumentCount)
	 return emacs_CurrentPosition - ConsoleLineBuffer;

    return emacs_ArgumentCount;
}

static int F_LOCAL EMACS_AdvanceNCharacters (void)
{
    EMACS_CheckArgCount ();

    if ((emacs_EndOfLine - emacs_CurrentPosition) < emacs_ArgumentCount)
	 return emacs_EndOfLine - emacs_CurrentPosition;

    return emacs_ArgumentCount;
}

/*
 * Deletes the previous character.
 */

static int F_LOCAL EMACS_DeleteCharacterBackwards (int c)
{
    int		count = EMACS_RetreatNCharacters ();

    if (emacs_CurrentPosition == ConsoleLineBuffer)
	return EMACS_Error (0);

    EMACS_GotoColumn (emacs_CurrentPosition - count);
    return EMACS_DeleteString (count);
}

/*
 * Deletes the character after the cursor.
 */

static int F_LOCAL EMACS_DeleteCurrentCharacter (int c)
{
    if (emacs_CurrentPosition == emacs_EndOfLine)
	return EMACS_Error (0);

    return EMACS_DeleteString (EMACS_AdvanceNCharacters ());
}

static int F_LOCAL EMACS_DeleteString (int nc)
{
    int		i,j;
    char	*cp;

    emacs_ArgumentCount = 0;

    if (nc == 0)
	return EMACS_KEY_NORMAL;

    if (emacs_MarkPointer != (char *)NULL)
    {
	if (emacs_CurrentPosition + nc > emacs_MarkPointer)
	    emacs_MarkPointer = emacs_CurrentPosition;

	else if (emacs_MarkPointer > emacs_CurrentPosition)
	    emacs_MarkPointer -= nc;
    }

/* This lets us yank a word we have deleted.  */

    if (nc > 1)
	EMACS_StackText (emacs_CurrentPosition, nc);

    emacs_EndOfLine -= nc;
    cp = emacs_CurrentPosition;
    j = 0;
    i = nc;

    while (i--)
	j += GEN_GetCharacterSize (*(cp++));

/* Copy including the null */

    memmove (emacs_CurrentPosition, emacs_CurrentPosition + nc,
	     emacs_EndOfLine - emacs_CurrentPosition + 1);

    AdjustOK = FALSE;				/* don't redraw */
    GEN_AdjustOutputString (emacs_CurrentPosition);

/* if we are already filling the line, there is no need to ' ','\b'.   But if
 * we must, make sure we do the minimum.
 */

    if ((i = MaximumColumns - 2 - CurrentScreenColumn) > 0)
    {
	j = (j < i) ? j : i;
	i = j;

	while (i--)
	    GEN_PutACharacter (CHAR_SPACE);

	i = j;

	while (i--)
	    GEN_PutACharacter (CHAR_BACKSPACE);
    }

/* EMACS_GotoColumn (emacs_CurrentPosition); */

    AdjustOK = TRUE;
    LastVisibleCharValid = FALSE;

    for (cp = GEN_FindLastVisibleCharacter (); cp > emacs_CurrentPosition; )
	GEN_BackspaceOver (*(--cp));

    return EMACS_KEY_NORMAL;
}

/*
 * Deletes the previous word.
 */

static int F_LOCAL EMACS_DeletePreviousWord (int c)
{
    return EMACS_DeleteString (EMACS_GetPreviousWord ());
}

/*
 * Moves the cursor backward one word.
 */

static int F_LOCAL EMACS_MoveBackAWord (int c)
{
    EMACS_GetPreviousWord ();
    return EMACS_KEY_NORMAL;
}

/*
 * Moves the cursor forward one word (a string of characters consisting of only
 * letters, digits, and underscores).
 */

static int F_LOCAL EMACS_MoveForwardAWord (int c)
{
    return EMACS_GotoColumn (emacs_CurrentPosition + EMACS_GetNextWord ());
}

/*
 * Deletes the current word.
 */

static int F_LOCAL EMACS_DeleteNextWord (int c)
{
    return EMACS_DeleteString (EMACS_GetNextWord ());
}

static int F_LOCAL EMACS_GetPreviousWord (void)
{
    int		nc = 0;
    char	*cp = emacs_CurrentPosition;

    if (cp == ConsoleLineBuffer)
    {
	RingWarningBell ();
	return 0;
    }

    EMACS_CheckArgCount ();

    while (emacs_ArgumentCount--)
    {
	while ((cp != ConsoleLineBuffer) && EMACS_IS_SPACE (cp[-1]))
	{
	    cp--;
	    nc++;
	}

	while ((cp != ConsoleLineBuffer) && !EMACS_IS_SPACE (cp[-1]))
	{
	    cp--;
	    nc++;
	}
    }

    EMACS_GotoColumn (cp);
    return nc;
}

/*
 * Find the end of the next word
 */

static int F_LOCAL EMACS_GetNextWord (void)
{
    int		nc = 0;
    char	*cp = emacs_CurrentPosition;

    if (cp == emacs_EndOfLine)
    {
	RingWarningBell ();
	return 0;
    }

    EMACS_CheckArgCount ();

    while (emacs_ArgumentCount--)
    {
	while ((cp != emacs_EndOfLine) && !EMACS_IS_SPACE (*cp))
	{
	    cp++;
	    nc++;
	}

	while ((cp != emacs_EndOfLine) && EMACS_IS_SPACE (*cp))
	{
	    cp++;
	    nc++;
	}
    }

    emacs_ArgumentCount = 0;
    return nc;
}

static int F_LOCAL EMACS_GotoColumn (char *cp)
{
    if (cp < emacs_StartVisible || cp >= (emacs_StartVisible + DisplayWidth))
    {

/* we are heading off screen */

	emacs_CurrentPosition = cp;
	GEN_AdjustRedraw ();
    }

    else if (cp < emacs_CurrentPosition)		/* move back */
    {
	while (cp < emacs_CurrentPosition)
	    GEN_BackspaceOver (*--emacs_CurrentPosition);
    }

    else if (cp > emacs_CurrentPosition) 		/* move forward */
    {
	while (cp > emacs_CurrentPosition)
	    GEN_OutputCharacterWithControl (*(emacs_CurrentPosition++));
    }

    emacs_ArgumentCount = 0;
    return EMACS_KEY_NORMAL;
}

static int F_LOCAL EMACS_GetDisplayStringSize (char *cp)
{
    int		size = 0;

    while (*cp)
	size += GEN_GetCharacterSize (*(cp++));

    return size;
}

/*
 * Moves the cursor backward (left) one character.
 */

static int F_LOCAL EMACS_PreviousCharacter (int c)
{
    if (emacs_CurrentPosition == ConsoleLineBuffer)
	return EMACS_Error (0);

    return EMACS_GotoColumn (emacs_CurrentPosition -
			     EMACS_RetreatNCharacters ());
}

/*
 * Moves the cursor forward one position.
 */

static int F_LOCAL EMACS_NextCharacter (int c)
{
    if (emacs_CurrentPosition == emacs_EndOfLine)
	return EMACS_Error (0);

    return EMACS_GotoColumn (emacs_CurrentPosition +
			     EMACS_AdvanceNCharacters ());
}

/*
 * Find character functions
 *
 * Moves the cursor forward on the current line to the indicated character.
 */

static int F_LOCAL EMACS_ForwardToCharacter (int c)
{
    return EMACS_FindCharacter (1, emacs_EndOfLine);
}

/* Search for a match */
/*
 * Search backwards in the current line for the next keyboard character.
 * Moves the cursor backword on the current line to the indicated character.
 */

static int F_LOCAL EMACS_BackwardToCharacter (int c)
{
    return EMACS_FindCharacter (-1, ConsoleLineBuffer);
}

static int F_LOCAL EMACS_FindCharacter (int direction, char *end)
{
    char	*cp = emacs_CurrentPosition;
    int		c;

    EMACS_CheckArgCount ();
    *emacs_EndOfLine = 0;

    if (emacs_CurrentPosition == end)
	return EMACS_Error (0);

    c = EMACS_GetNonFunctionKey ();

/* Search for a match */

    do
    {
	cp += direction;

	if ((*cp == (char)c) && (--emacs_ArgumentCount == 0))
	    return EMACS_GotoColumn (cp);

    } while (cp != end);

    return EMACS_Error (0);
}

/*
 * New line character - execute the line
 */

static int F_LOCAL EMACS_NewLine (int c)
{
    GEN_PutACharacter (CHAR_NEW_LINE);
    FlushStreams ();
    *(emacs_EndOfLine++) = CHAR_NEW_LINE;
    *emacs_EndOfLine = 0;
    return EMACS_KEY_EOL;
}

/*
 * Acts as an end-of-file.
 */

static int F_LOCAL EMACS_EndOfInput (int c)
{
    GEN_PutACharacter (CHAR_NEW_LINE);
    FlushStreams ();
    *(emacs_EndOfLine++) = (char)GetEOFKey ();
    *emacs_EndOfLine = 0;
    return EMACS_KEY_EOL;
}

/*
 * History processing
 *
 * Fetches the least recent (oldest) history line.
 */

static int F_LOCAL EMACS_GetFirstHistory (int c)
{
    return EMACS_LoadFromHistory (GetFirstHistoryEvent ());
}

/*
 * Fetches the most recent (youngest) history line.
 */

static int F_LOCAL EMACS_GetLastHistory (int c)
{
    return EMACS_LoadFromHistory (GetLastHistoryEvent () - 1);
}

/*
 * Fetches the previous command.  Each time Ctrl-P is entered, the previous
 * command back in time is accessed.  Moves back one line when not on the
 * first line of a multiple line command.
 */

static int F_LOCAL EMACS_GetPreviousCommand (int c)
{
    EMACS_CheckArgCount ();
    return EMACS_LoadFromHistory (CurrentHistoryEvent - emacs_ArgumentCount);
}

/*
 * Fetches the next command line.  Each time Ctrl-N is entered, the next
 * command line forward in time is accessed.
 */

static int F_LOCAL EMACS_GetNextCommand (int c)
{
    EMACS_CheckArgCount ();
    return EMACS_LoadFromHistory (CurrentHistoryEvent + emacs_ArgumentCount);
}

/*
 * Load the requested history record
 */

static int F_LOCAL EMACS_LoadFromHistory (int event)
{
    int		oldsize;
    char	*hp;

    if ((event < 0) || (event > GetLastHistoryEvent ()) ||
	((hp = GetHistoryRecord (event)) == (char *)NULL))
	return EMACS_Error (0);

    CurrentHistoryEvent = event;

    oldsize = EMACS_GetDisplayStringSize (ConsoleLineBuffer);
    strcpy (ConsoleLineBuffer, hp);

    emacs_StartVisible = ConsoleLineBuffer;
    emacs_CurrentPosition = ConsoleLineBuffer + strlen (hp);
    *(emacs_EndOfLine = emacs_CurrentPosition) = 0;
    LastVisibleCharValid = FALSE;

    if (emacs_EndOfLine > GEN_FindLastVisibleCharacter ())
	EMACS_GotoColumn (emacs_EndOfLine);

    else
	GEN_Redraw (oldsize);

    return EMACS_KEY_NORMAL;
}

/*
 * Operate - Executes the current line and fetches the next line relative to
 * the current line from the history file.
 */

static int F_LOCAL EMACS_OperateOnLine (int c)
{
    emacs_NextCommandIs = CurrentHistoryEvent + 1;
    return (EMACS_NewLine (c));
}

/*
 * Acts as end-of-file if alone on a line; otherwise deletes current
 * character.
 */

static int F_LOCAL EMACS_EOTOrDelete (int c)
{
    return (emacs_EndOfLine == ConsoleLineBuffer)
		? EMACS_EndOfInput (c)
		: EMACS_DeleteCurrentCharacter (c);
}

/*
 * Reverses search history for a previous command line containing the string
 * specified by the String parameter.  If a value of zero is given, the
 * search is forward.  The specified string is terminated by an Enter
 * or new-line character.  If the string is preceded by a ^ (caret character),
 * the matched line must begin with String.  If the String parameter is
 * omitted, then the next command line containing the most recent String is
 * accessed.  In this case, a value of zero reverses the direction of the
 * search.
 *
 * ARG COUNT not implemented
 */

static int F_LOCAL EMACS_SearchHistory (int c)
{
    int			offset = -1;	/* offset of match in		*/
					/* ConsoleLineBuffer, else -1	*/
    char		pat [256 + 1];	/* pattern buffer */
    char		*p = pat;
    int			(F_LOCAL *func)(int);
    int			direction = -1;

    *p = 0;

    if ((emacs_LastCommand == EMACS_SetArgValue) &&
        (!emacs_ArgumentCount))
	direction = 1;

    while (1)
    {
	if (offset < 0)
	{
	    GEN_PutAString ("\nI-search: ");
	    GEN_PutAString (pat);
	    GEN_AdjustOutputString (pat);
	}

	FlushStreams ();

	c = EMACS_GetNonFunctionKey ();

	func = emacs_KeyDefinitions[0][c & 0x0ff]->xf_func;

	if (c == CHAR_ESCAPE)
	    break;

	else if (func == EMACS_SearchHistory)
	    offset = EMACS_SearchMatch (pat, offset, direction);

/* Add / Delete a character to / from the string */

	else if ((func == EMACS_DeleteCharacterBackwards) ||
		 (func == EMACS_AutoInsert))
	{
	    if (func == EMACS_DeleteCharacterBackwards)
	    {
		if (p == pat)
		{
		    RingWarningBell ();		/* Empty string */
		    continue;
		}

		*(--p) = 0;

/* Empty string - no search - restart */

		if (p == pat)
		{
		    offset = -1;
		    continue;
		}
	    }

/* Add character to string */

	    else if (p >= pat + 256)
	    {
		RingWarningBell ();		/* Too long */
		continue;
	    }

/* add char to pattern */

	    else
	    {
		*(p++) = (char)c;
		*p = 0;
	    }

/* Search */

	    if (offset >= 0)
	    {

/* already have partial match */

		if ((offset = EMACS_PatternMatch (ConsoleLineBuffer, pat)) >= 0)
		{
		    EMACS_GotoColumn (ConsoleLineBuffer + offset + (p - pat) -
			    (*pat == '^'));
		    continue;
		}
	    }

	    offset = EMACS_SearchMatch (pat, offset, direction);
	}

/* other command */

	else
	{
	    static char push[2];

	    push[0] = (char)c;
	    push[1] = 0;
	    emacs_CurrentMacroString = push; /* push command */
	    break;
	}
    }

    if (offset < 0)
	GEN_Redraw (-1);

    return EMACS_KEY_NORMAL;
}

/*
 * search backward from current line
 */

static int F_LOCAL EMACS_SearchMatch (char *pat, int offset, int direction)
{
    int		event = CurrentHistoryEvent + direction;
    char	*hp;
    int		i;

    while ((hp = GetHistoryRecord (event)) != (char *)NULL)
    {
	if ((i = EMACS_PatternMatch (hp, pat)) >= 0)
	{
	    if (offset < 0)
		GEN_PutACharacter (CHAR_NEW_LINE);

	    EMACS_LoadFromHistory (event);
	    EMACS_GotoColumn (ConsoleLineBuffer + i + strlen (pat) -
			      (*pat == '^'));
	    return i;
	}

	event += direction;
    }

    RingWarningBell ();
    CurrentHistoryEvent = GetLastHistoryEvent ();
    return -1;
}

/*
 * Return position of first match of pattern in string, else -1
 */

static int F_LOCAL EMACS_PatternMatch (char *str, char *pat)
{
    if (*pat == '^')
	return (strncmp (str, pat + 1, strlen (pat + 1)) == 0) ? 0 : -1;

    else
    {
	char *q = strstr (str, pat);

	return (q == (char *)NULL) ? -1 : q - str;
    }
}

/*
 * Kill the current line
 */

static int F_LOCAL EMACS_KillLine (int c)
{
    int		i, j;

    *emacs_EndOfLine = 0;
    i = emacs_EndOfLine - ConsoleLineBuffer;
    j = EMACS_GetDisplayStringSize (ConsoleLineBuffer);
    EMACS_StackText (emacs_CurrentPosition = ConsoleLineBuffer, i);

    EMACS_ResetInput ();
    emacs_MarkPointer = (char *)NULL;

    if (c != -1)
	GEN_Redraw (j);

    return EMACS_KEY_NORMAL;
}

/*
 * Move to the end of the line
 */

static int F_LOCAL EMACS_GotoEnd (int c)
{
    return EMACS_GotoColumn (emacs_EndOfLine);
}

/*
 * Move to the start of the line
 */

static int F_LOCAL EMACS_GotoStart (int c)
{
    return EMACS_GotoColumn (ConsoleLineBuffer);
}

/*
 * Redraw the line
 */

static int F_LOCAL EMACS_RedrawLine (int c)
{
    GEN_Redraw (-1);
    return EMACS_KEY_NORMAL;
}

/*
 * Transposes the current character with the next character in emacs mode.
 * Transposes the two previous characters in gmacs mode.
 */

static int F_LOCAL EMACS_Transpose (int c)
{
    char	tmp;

    if (emacs_CurrentPosition == ConsoleLineBuffer)
	return EMACS_Error (0);

    else if ((emacs_CurrentPosition == emacs_EndOfLine)
#  if defined (FLAGS_GMACS)
	     || (ShellGlobalFlags & FLAGS_GMACS)
#  endif
	    )
    {
	if (emacs_CurrentPosition - ConsoleLineBuffer == 1)
	    return EMACS_Error (0);

	tmp = emacs_CurrentPosition[-1];
	emacs_CurrentPosition[-1] = emacs_CurrentPosition[-2];
	emacs_CurrentPosition[-2] = tmp;

	GEN_BackspaceOver (tmp);
	GEN_BackspaceOver (emacs_CurrentPosition[-1]);
	GEN_OutputCharacterWithControl (tmp);
	GEN_OutputCharacterWithControl (emacs_CurrentPosition[-1]);
    }

/* Transpose the current and next characters */

    else if ((emacs_CurrentPosition + 1) == emacs_EndOfLine)
	return EMACS_Error (0);

    else
    {
	tmp = emacs_CurrentPosition[0];
	emacs_CurrentPosition[0] = emacs_CurrentPosition[1];
	emacs_CurrentPosition[1] = tmp;
	GEN_OutputCharacterWithControl (emacs_CurrentPosition[0]);
	GEN_OutputCharacterWithControl (tmp);
	GEN_BackspaceOver (tmp);
	emacs_CurrentPosition++;
    }

    return EMACS_KEY_NORMAL;
}

/*
 * Escapes the next character.  Editing characters can be entered in a command
 * line or in a search string if preceded by a quote command.  The escape
 * removes the next character's editing features, if any.
 */

static int F_LOCAL EMACS_LiteralValue (int c)
{
    emacs_CurrentPrefix = -1;
    return EMACS_KEY_NORMAL;
}

/*
 * Change the prefix values
 *
 * Introduces a 2-character command sequence.
 */

static int F_LOCAL EMACS_Prefix1 (int c)
{
    emacs_CurrentPrefix = 1;
    return EMACS_KEY_META;
}

/*
 * Introduces a 2-character command sequence.
 */

static int F_LOCAL EMACS_Prefix2 (int c)
{
    emacs_CurrentPrefix = 2;
    return EMACS_KEY_META;
}

/* Introduces a 2-character command sequence.  This prefix allows the user to
 * map PC function keys onto commands.  The second character is the IBM scan
 * code value of the function key to be assigned.
 */

static int F_LOCAL EMACS_Prefix3 (int c)
{
    emacs_CurrentPrefix = 3;
    return EMACS_KEY_META;
}

/*
 * Deletes from the cursor to the end of the line.  If preceded by a numerical
 * parameter whose value is less than the current cursor position, this editing
 * command deletes from the given position up to the cursor.  If preceded by a
 * numerical parameter whose value is greater than the current cursor position,
 * this editing command deletes from the cursor up to given cursor position.
 */

static int F_LOCAL EMACS_KillToEndOfLine (int c)
{
    int		i = emacs_EndOfLine - emacs_CurrentPosition;
    char	*cp;

/* If a count is provided */

    if (emacs_LastCommand == EMACS_SetArgValue)
    {
	if ((cp = ConsoleLineBuffer + emacs_ArgumentCount) > emacs_EndOfLine)
	    cp = emacs_EndOfLine;

	if (cp > emacs_CurrentPosition)
	    i = cp - emacs_CurrentPosition;

	else if (cp < emacs_CurrentPosition)
	{
	    i = emacs_CurrentPosition - cp;
	    EMACS_GotoColumn (cp);
	}
    }

    emacs_LastVisibleCharacter = emacs_CurrentPosition;
    LastVisibleCharValid = TRUE;

/* only stack text if DeleteString doesn't */

    if (i <= 1)
	EMACS_StackText (emacs_CurrentPosition, i);

    return EMACS_DeleteString (i);
}

/*
 * Push a text string on to the circular stack
 */

static void F_LOCAL EMACS_StackText (char *start, int nchars)
{
    char	*cp;

    SetMemoryAreaNumber (cp = GetAllocatedSpace ((size_t)(nchars + 1)), 0);

    memmove (cp, start, nchars);
    cp[nchars] = 0;

    if (emacs_Stack[emacs_StackPointer] != (char *)NULL)
	ReleaseMemoryCell ((void *)emacs_Stack[emacs_StackPointer]);

    emacs_Stack[emacs_StackPointer] = cp;
    emacs_StackPointer = (emacs_StackPointer + 1) % EMACS_KILL_SIZE;
}

/*
 * Pushes the region from the cursor to the mark on the stack.
 */

static int F_LOCAL EMACS_PushText (int c)
{
    if (emacs_MarkPointer == (char *)NULL)
	return EMACS_Error (c);

    if (emacs_MarkPointer > emacs_CurrentPosition)
	EMACS_StackText (emacs_CurrentPosition,
			 emacs_MarkPointer - emacs_CurrentPosition);

    else
	EMACS_StackText (emacs_MarkPointer,
			 emacs_CurrentPosition - emacs_MarkPointer);

    return EMACS_KEY_NORMAL;

}

/*
 * Restores the last item removed from line.  (Yanks the item back to the line.)
 */

static int F_LOCAL EMACS_PutText (int c)
{
    emacs_TopOfStack = (emacs_StackPointer == 0) ? EMACS_KILL_SIZE - 1
						 : emacs_StackPointer - 1;

    if (emacs_Stack[emacs_TopOfStack] == (char *)NULL)
	return EMACS_YankError (emacs_NTY);

    emacs_MarkPointer = emacs_CurrentPosition;
    EMACS_InsertString (emacs_Stack[emacs_TopOfStack]);
    return EMACS_KEY_NORMAL;
}

/*
 * Yank the text - remove top stack item
 */

static int F_LOCAL EMACS_YankText (int c)
{
    emacs_TopOfStack = (emacs_StackPointer == 0) ? EMACS_KILL_SIZE
						 : emacs_StackPointer;

    if (emacs_Stack[--emacs_TopOfStack] == (char *)NULL)
	return EMACS_YankError (emacs_NTY);

    emacs_MarkPointer = emacs_CurrentPosition;
    EMACS_InsertString (emacs_Stack[emacs_TopOfStack]);
    return EMACS_KEY_NORMAL;
}

/*
 * Immediately after a yank, replaces the inserted text string with the
 * next previous killed text string.
 */

static int F_LOCAL EMACS_YankPop (int c)
{
    int		len;
    char	*err = (char *)NULL;
    int		previous = (emacs_TopOfStack == 0) ? EMACS_KILL_SIZE - 1
						   : emacs_TopOfStack - 1;

/* Check that there are enough items on the stack */

    if ((emacs_LastCommand != EMACS_YankText) &&
	(emacs_LastCommand != EMACS_PutText))
	err = "\nyank something first";

    else if (emacs_Stack[previous] == (char *)NULL)
	err = "\nonly one item on stack";

    if (err != (char *)NULL)
	return EMACS_YankError (err);

/* Remove the top of stack */

    len = strlen (emacs_Stack[emacs_TopOfStack]);
    EMACS_GotoColumn (emacs_CurrentPosition - len);
    EMACS_DeleteString (len);

/* Insert the previous string */

    EMACS_InsertString (emacs_Stack[emacs_TopOfStack = previous]);
    return EMACS_KEY_NORMAL;
}

/*
 * Yank error
 */

static int F_LOCAL EMACS_YankError (char *message)
{
    EMACS_Error (0);
    GEN_PutAString (message);
    GEN_Redraw (-1);
    return EMACS_KEY_NORMAL;
}

/*
 * Error - ring the bell
 */

static int F_LOCAL EMACS_Error (int c)
{
    RingWarningBell ();
    emacs_ArgumentCount = 0;
    return EMACS_KEY_NORMAL;
}

/*
 * Reset input, clearing the current line and yank buffers.
 */

static int F_LOCAL EMACS_FullReset (int c)
{
    GEN_OutputCharacterWithControl (c);

    EMACS_ResetInput ();
    GEN_Redraw (-1);
    return EMACS_KEY_NORMAL;
}

/*
 * Reset the input pointers
 */

static void F_LOCAL EMACS_ResetInput (void)
{
    emacs_StartVisible	       = ConsoleLineBuffer;
    emacs_CurrentPosition      = ConsoleLineBuffer;
    emacs_EndOfLine	       = ConsoleLineBuffer;
    emacs_LastVisibleCharacter = ConsoleLineBuffer;

    LastVisibleCharValid = TRUE;
    *emacs_CurrentPosition = 0;
    emacs_ArgumentCount = 0;
}

/*
 * Abort the edit - Useful as a response to a request for a search-history
 * pattern in order to abort the search.
 */

static int F_LOCAL EMACS_Abort (int c)
{
    /* GEN_OutputCharacterWithControl(c); */
    EMACS_ResetInput ();
    EMACS_KillLine (-1);
    return EMACS_KEY_INTERRUPT;
}

/*
 * Translate special characters in the keystroke macro to binary
 */

static void F_LOCAL EMACS_MapInKeyStrokes (char *cp)
{
    unsigned char	*op = (unsigned char *)cp;

    while (*cp)
    {

/* XXX -- should handle \^ escape? */

	if (*cp == '^')
	{
	    cp++;

	    if (*cp == '0')
		*(op++) = 0xE0;

	    else if (*cp >= '?')	/* includes '?'; ASCII */
		*(op++) = (char)(*cp == '?' ? 0x07f : *cp & 0x1F);

	    else
	    {
		*(op++) = '^';
		cp--;
	    }
	}

	else
	    *(op++) = *cp;

	cp++;
    }

    *op = 0;
}

/*
 * Convert Macro keystrokes to display characters and display it
 */

static void F_LOCAL EMACS_MapOutKeystrokes (unsigned int c)
{

/* ASCII? */

    if ((c < CHAR_SPACE) || (c == 0x7F))
    {
	fputchar ('^');
	c = (c == 0x7F) ? '?' : (c | 0x40);
    }

    else if (c == 0xE0)
    {
	fputchar ('^');
	c = '0';
    }

    fputchar (c);
}

/*
 * Print a macro value
 */

static void F_LOCAL EMACS_PrintMacros (int prefix, int key)
{
    bool	Quotes = FALSE;

    if (prefix == 1)
	EMACS_MapOutKeystrokes (emacs_Prefix1);

    else if (prefix == 2)
	EMACS_MapOutKeystrokes (emacs_Prefix2);

    else if (prefix == 3)
	EMACS_MapOutKeystrokes (emacs_Prefix3);

    EMACS_MapOutKeystrokes (key);
    foputs (" = ");

    if (emacs_KeyDefinitions[prefix][key]->xf_func != EMACS_InsertMacroString)
    {
	Quotes = TRUE;
	fputchar (CHAR_SINGLE_QUOTE);
    }

    foputs (emacs_KeyDefinitions[prefix][key]->emacs_FunctionName);

    if (Quotes)
	fputchar (CHAR_SINGLE_QUOTE);

    fputchar (CHAR_NEW_LINE);
}

/*
 * Bind string to macro
 */

int	BindKeyStroke (char *keystrokes, char *EditCommand, bool macro)
{
    EMACS_FunctionMap	*fp;
    int			prefix, key;
    char		*sp = (char *)NULL;

    if (emacs_KeyDefinitions == NULL)
	return PrintWarningMessage ("bind: only available in interactive mode");

    if (keystrokes == (char *)NULL)
    {
	for (prefix = 0; prefix < EMACS_KEYDEF_TABLES; prefix++)
	{
	    for (key = 0; key < EMACS_KEYDEF_ENTRIES; key++)
	    {
		if (((fp = emacs_KeyDefinitions[prefix][key]) == NULL) ||
		    (fp->xf_func == EMACS_AutoInsert) ||
		    (fp->xf_func == EMACS_Error) ||
		    (fp->emacs_FunctionName == null))
			continue;

		EMACS_PrintMacros (prefix, key);
	    }
	}

	return 0;
    }

    EMACS_MapInKeyStrokes (keystrokes);
    prefix = key = 0;

    for (;; keystrokes++)
    {
	key = *keystrokes;

	if (emacs_KeyDefinitions[prefix][key]->xf_func == EMACS_Prefix1)
	    prefix = 1;

	else if (emacs_KeyDefinitions[prefix][key]->xf_func == EMACS_Prefix2)
	    prefix = 2;

	else if (emacs_KeyDefinitions[prefix][key]->xf_func == EMACS_Prefix3)
	    prefix = 3;

	else
	    break;
    }

    if (EditCommand == (char *)NULL)
    {
	EMACS_PrintMacros (prefix, key);
	return 0;
    }

    if (*EditCommand == 0)
	fp = ((prefix == 1) && ((isalpha (key)) || (key == ']' & 0x1f)))
		? EMACS_ALIAS_MAP : EMACS_INSERT_MAP;

    else if (!macro)
    {
	for (fp = EMACS_FunctionMaps; fp->xf_func; fp++)
	{
	    if (strcmp(fp->emacs_FunctionName, EditCommand) == 0)
		break;
	}

	if (fp->xf_func == NULL || (fp->emacs_FunctionFlags & EMACS_NO_BIND))
	    return PrintWarningMessage ("%s: no such function", EditCommand);

	if (fp->xf_func == EMACS_Prefix1)
	    emacs_Prefix1 = key;

	if (fp->xf_func == EMACS_Prefix2)
	    emacs_Prefix2 = key;

	if (fp->xf_func == EMACS_Prefix3)
	    emacs_Prefix3 = key;
    }

    else
    {
	fp = EMACS_MACRO_MAP;
	EMACS_MapInKeyStrokes (EditCommand);
	sp = StringSave (EditCommand);
    }

    if ((emacs_KeyDefinitions[prefix][key]->emacs_FunctionFlags &
		EMACS_MEMORY_ALLOC) &&
	(emacs_MacroDefinitions[prefix][key] != (char *)NULL))
	ReleaseMemoryCell ((void *)emacs_MacroDefinitions[prefix][key]);

    emacs_KeyDefinitions[prefix][key] = fp;
    emacs_MacroDefinitions[prefix][key] = sp;
    return 0;
}

/*
 * Initialise Emacs
 */

void	EMACS_Initialisation (void)
{
    int			i, j;
    unsigned char	a_key, f_key;
    EMACS_FunctionMap	*fp;

    emacs_KeyDefinitions = (EMACS_FunctionMap *(*)[EMACS_KEYDEF_ENTRIES])
		GetAllocatedSpace (sizeof (*emacs_KeyDefinitions) *
				   EMACS_KEYDEF_TABLES);
    SetMemoryAreaNumber (emacs_KeyDefinitions, 0);

/* Set everything to either insert character or error */

    for (j = 0; j < EMACS_KEYDEF_ENTRIES; j++)
	emacs_KeyDefinitions[0][j] = EMACS_INSERT_MAP;

    for (i = 1; i < EMACS_KEYDEF_TABLES; i++)
    {
	for (j = 0; j < EMACS_KEYDEF_ENTRIES; j++)
	    emacs_KeyDefinitions[i][j] = EMACS_ERROR_MAP;
    }

/* Establish Prefix 1 aliasing ESC-letter or Esc Ctrl-] letter */

    emacs_KeyDefinitions[1][']' & 0x01f] = EMACS_ALIAS_MAP;

    for (i = 'A'; i <= 'Z'; i++)
    {
	emacs_KeyDefinitions[1][1] = EMACS_ALIAS_MAP;
	emacs_KeyDefinitions[1][tolower(i)] = EMACS_ALIAS_MAP;
    }

/* Load the default values */

    for (fp = EMACS_FunctionMaps; fp->xf_func; fp++)
    {
	if ((fp->emacs_KeyStroke) || (fp->emacs_TableNumber))
	    emacs_KeyDefinitions[fp->emacs_TableNumber][fp->emacs_KeyStroke]
				= fp;

/* Load .ini function ? */

	if ((j = fp->emacs_FunctionFlags & EMACS_INI_MASK))
	{
	    if (((a_key = GetFunctionKeyMap (j, &f_key)) == KT_FUNCTION) ||
		 (a_key == KT_ALTFUNCTION))
		emacs_KeyDefinitions[3][f_key] = fp;

	    else if (a_key != KT_RESIZE)
		emacs_KeyDefinitions[0][a_key] = fp;


/* Handle special case of scan forwards and backwards in history */

	    if (j == KF_SCANFOREWARD)
	    {
	        if (((a_key = GetFunctionKeyMap (KF_SCANBACKWARD,
						 &f_key)) == KT_FUNCTION) ||
		     (a_key == KT_ALTFUNCTION))
		    emacs_KeyDefinitions[3][f_key] = fp;

		else if (a_key != KT_RESIZE)
		    emacs_KeyDefinitions[0][a_key] = fp;
	    }
	}
    }

/* Set up macro definitions */

    emacs_MacroDefinitions = (char *(*)[EMACS_KEYDEF_ENTRIES])
    		GetAllocatedSpace (sizeof (*emacs_MacroDefinitions) *
				   EMACS_KEYDEF_TABLES);

    SetMemoryAreaNumber (emacs_MacroDefinitions, 0);

    for (i = 1; i < EMACS_KEYDEF_TABLES; i++)
    {
	for (j = 0; j < EMACS_KEYDEF_ENTRIES; j++)
	    emacs_MacroDefinitions[i][j] = NULL;
    }
}

/*
 * Clear the screen and print the current line.
 */

static int F_LOCAL EMACS_ClearScreen (int c)
{
    ClearScreen ();
    GEN_Redraw (0);
    return EMACS_KEY_NORMAL;
}

/*
 * Set a mark
 */

static int F_LOCAL EMACS_SetMark (int c)
{
    emacs_MarkPointer = emacs_CurrentPosition;
    return EMACS_KEY_NORMAL;
}

/*
 * Kills from the cursor to the mark.
 */

static int F_LOCAL EMACS_KillRegion (int c)
{
    int		rsize;
    char	*xr;

    if (emacs_MarkPointer == (char *)NULL)
	return EMACS_Error (c);

    if (emacs_MarkPointer > emacs_CurrentPosition)
    {
	rsize = emacs_MarkPointer - emacs_CurrentPosition;
	xr = emacs_CurrentPosition;
    }

    else
    {
	rsize = emacs_CurrentPosition - emacs_MarkPointer;
	xr = emacs_MarkPointer;
    }

    EMACS_GotoColumn (xr);
    EMACS_StackText (emacs_CurrentPosition, rsize);
    EMACS_DeleteString (rsize);
    emacs_MarkPointer = xr;
    return EMACS_KEY_NORMAL;
}

/*
 * Exchange the current cursor position and the mark
 */

static int F_LOCAL EMACS_ExchangeCurrentAndMark (int c)
{
    char	*tmp;

    if (emacs_MarkPointer == (char *)NULL)
	return EMACS_Error (c);

    tmp = emacs_MarkPointer;
    emacs_MarkPointer = emacs_CurrentPosition;
    return EMACS_GotoColumn (tmp);
}

/*
 * No operation!
 */

static int F_LOCAL EMACS_NoOp (int c)
{
    return EMACS_KEY_NOOP;
}

/*
 * File/command name completion routines
 *
 * Save the full file name in a list
 */

static void F_LOCAL EMACS_SaveFileName (char *dirnam, char *name)
{
    char	*cp;
    int		type = 0;		/* '*' if executable,		*/
    					/* '/' if directory,		*/
					/* else 0			*/
    int		len = strlen (name);

    /* determine file type */

    if (dirnam != (char *)NULL)
    {
	struct stat	statb;
	char		*buf = GetAllocatedSpace ((size_t)(strlen (dirnam) +
							   len + 2));

	if (strcmp (dirnam, CurrentDirLiteral) == 0)
	    *buf = 0;

	else if (strcmp (dirnam, DirectorySeparator) == 0)
	    strcpy (buf, DirectorySeparator);

	else
	    strcat (strcpy (buf, dirnam), DirectorySeparator);

	strcat (buf, name);

	if (S_stat (buf, &statb))
	{
	    if (S_ISDIR (statb.st_mode))
		type = CHAR_UNIX_DIRECTORY;

	    else if (S_ISREG (statb.st_mode) && (statb.st_mode & S_IEXEC) != 0)
		type = '*';
	}

	if (type)
	    ++len;

	ReleaseMemoryCell ((void *)buf);
    }

    if (len > emacs_MaxFilenameSize)
	emacs_MaxFilenameSize = len;

/* stash name for later sorting */

    cp = strcpy (GetAllocatedSpace ((size_t)(len + 1)), name);

/* append file type indicator */

    if (dirnam && type)
    {
	cp[len - 1] = (char)type;
	cp[len] = 0;
    }

    EMACS_Flist = AddWordToBlock (cp, EMACS_Flist);
}

/*
 * List saved filenames
 */

static void F_LOCAL EMACS_ListSavedFileNames (void)
{
    int		items;
    char	**array;

    if ((array = GetWordList (AddWordToBlock (NOWORD, EMACS_Flist)))
	       == (char **)NULL)
	return;

    if ((items = CountNumberArguments (array)) > 1)
	qsort (array, items, sizeof (char *), SortCompare);

    feputc (CHAR_NEW_LINE);
    PrintAList (items, array);
    ReleaseAList (array);
    FlushStreams ();

    GEN_Redraw (-1);
}

/*
 * Display job list - only available for OS/2
 */

#  if (OS_TYPE != OS_DOS) 
static int F_LOCAL EMACS_DisplayJobList (int c)
{
    fputchar (CHAR_NEW_LINE);
#    if (OS_TYPE == OS_NT)
    PrintJobs (TRUE);
#    else
    PrintProcessTree (getpid ());
#    endif
    GEN_Redraw (-1);
    return EMACS_KEY_NORMAL;
}
#  endif


/*
 * File name completion functions
 *
 * Prints a sorted, columnated list of file names (if any) that can complete
 * the partial word containing the cursor.  Directory names have / postpended
 * to them, and executable file names are followed by *.
 */

static int F_LOCAL EMACS_ListFiles (int c)
{
    return EMACS_FileCompletion (EMACS_FN_LIST);
}

/* File-name completion.  Replaces the current word with the longest common
 * prefix of all file names that match the current word with an asterisk
 * appended.  If the match is unique, a \fB/\fR (slash) is appended if the
 * file is a directory and a space is appended if the file is not a directory.
 */

static int F_LOCAL EMACS_CompleteFile (int c)
{
    return EMACS_FileCompletion (EMACS_FN_COMPLETE);
}

/*
 * Attempts file name substitution on the current word.  An asterisk is
 * appended if the word doesn't match any file or contain any special pattern
 * characters.
 */

static int F_LOCAL EMACS_SubstituteFiles (int c)
{
    return EMACS_FileCompletion (EMACS_FN_SUBSTITUTE);
}

static int F_LOCAL EMACS_FileCompletion (int type)
{
    char		buf [FFNAME_MAX];
    char		bug [FFNAME_MAX];
    char		*cp = buf;
    char		*xp = emacs_CurrentPosition;
    char		*lastp;
    char		*dirnam;
    DIR			*dirp;
    struct dirent	*dp;
    long		loc = -1;
    int			len;
    int			multi = 0;
#  if (OS_TYPE == OS_UNIX)
    int			(*Compare)(const char *,
    				   const char *, size_t) = strncmp;
#  else
    int			(*Compare)(const char *,
    				   const char *, size_t) = strnicmp;
#  endif

    /*
     * type ==
     *		0 for list
     *		1 for complete
     *		2 for complete-list
     */

    while (xp != ConsoleLineBuffer)
    {
	--xp;

	if (isspace (*xp))
	{
	    xp++;
	    break;
	}
    }

    if (IS_Numeric ((int)*xp) && ((xp[1] == '<') || (xp[1] == '>')))
	xp++;

    while ((*xp == '<') || (*xp == '>'))
	xp++;

    if (type != EMACS_FN_LIST)		/* for complete */
    {
	while (*emacs_CurrentPosition && !isspace (*emacs_CurrentPosition))
	    GEN_OutputCharacterWithControl (*(emacs_CurrentPosition++));
    }

    if (type != EMACS_FN_COMPLETE)			/* for list */
    {
	emacs_MaxFilenameSize = 0;
	EMACS_Flist = (Word_B *)NULL;
    }

    while (*xp && !isspace (*xp))
	*(cp++) = *(xp++);

    *cp = 0;
    strcpy (buf, cp = substitute (buf, EXPAND_TILDE));
    ReleaseMemoryCell (cp);

    if ((lastp = FindLastPathCharacter (buf)) != (char *)NULL)
	*lastp = 0;

    dirnam = (lastp == (char *)NULL) ? CurrentDirLiteral
				     : (lastp == buf) ? DirectorySeparator
						      : buf;
    if ((dirp = opendir (dirnam)) == (DIR *)NULL)
	return EMACS_Error (0);

    if (IsHPFSFileSystem (dirnam) && (!(ShellGlobalFlags & FLAGS_NOCASE)))
	Compare = strncmp;

    if (lastp == (char *)NULL)
	lastp = buf;

    else
	lastp++;

    len = strlen (lastp);

    while ((dp = readdir (dirp)) != (struct dirent *)NULL)
    {
	cp = dp->d_name;

/* always ignore . and .. */

	if ((cp[0] == CHAR_PERIOD) &&
	    ((cp[1] == 0)  || ((cp[1] == CHAR_PERIOD) && (cp[2] == 0))))
	    continue;

	if ((*Compare) (lastp, cp, len) == 0)
	{

/* Complete ? */

	    if (type != EMACS_FN_LIST)
	    {
		if (loc == -1)
		{
		    (void)strcpy (bug, cp);
		    loc = strlen (cp);
		}

		else
		{
		    multi = 1;
		    loc = EMACS_FindLongestMatch (bug, cp);
		    bug[loc] = 0;
		}
	    }

/* List? */

	    if (type != EMACS_FN_COMPLETE)
		EMACS_SaveFileName (dirnam, cp);
	}
    }

/* Close up the directory */

    closedir (dirp);

/* Complete ? */

    if (type != EMACS_FN_LIST)
    {
	if ((loc < 0) || ((loc == 0) && (type != EMACS_FN_SUBSTITUTE)) ||
	    (strlen (cp = bug + len) == 0))
	    return EMACS_Error (0);

	EMACS_InsertString (cp);

	if (!multi)
	{
	    if (lastp == buf)
		buf[0] = 0;

	    else if (lastp == buf + 1)
	    {
		buf[1] = 0;
		buf[0] = CHAR_UNIX_DIRECTORY;
	    }

	    else
		strcat (buf, DirectorySeparator);

	    strcat (buf, bug);

	    if (IsDirectory (buf))
		EMACS_InsertString (DirectorySeparator);

	    else
		EMACS_InsertString (" ");
	}
    }

/* List or complete-list and ambiguous */

    if ((type == EMACS_FN_LIST) || ((type == EMACS_FN_SUBSTITUTE) && multi))
	EMACS_ListSavedFileNames ();

    return EMACS_KEY_NORMAL;
}

/*
 * Find longest match in two strings
 */

static int F_LOCAL EMACS_FindLongestMatch (char *s1, char *s2)
{
    char	*p = s1;

    while ((*p == *(s2++)) && *p)
	p++;

    return p - s1;
}

/*
 * EMACS_SetArgValue - set an arg value for next function.
 *
 * Defines the numeric parameter.  The digits are taken as a parameter to
 * the next command.  The commands that accept a parameter are forward-char,
 * backward-char, backward-word, forward-word, delete-word-forward,
 * delete-char-forward, delete-word-backward, delete-char-backward,
 * prev-hist-word, copy-last-arg, up-history, down-history, search-history,
 * upcase-word, downcase-word, capitalise-word, upcase-char, downcase-char,
 * capitalise-char, kill-to-eol, search-char-forward and search-char-backward.
 */

static int F_LOCAL EMACS_SetArgValue (int c)
{
    emacs_ArgumentCount = 0;

/* Read all digits */

    while (IS_Numeric (c & 0x0ff))
    {
	emacs_ArgumentCount = (emacs_ArgumentCount * 10) + (c & 0x0f);
	c = EMACS_GetNonFunctionKey ();
    }

/* Save the bad key as the unget */

    emacs_UnGetCharacter = c & 0x0ff;
    return EMACS_KEY_NORMAL;
}

/*
 * Multiplies the parameter of the next command by 4.
 */

static int F_LOCAL EMACS_Multiply (int c)
{
    if (!emacs_ArgumentCount)
	emacs_ArgumentCount = 1;

    emacs_ArgumentCount *= 4;
    emacs_LastCommand = EMACS_SetArgValue;

/* Not really a no-op, but we don't want emacs_LastCommand reset */

    return EMACS_KEY_NOOP;
}

/*
 * EMACS_GetWordsFromHistory - recover word from prev command.  This
 * function recovers the last word from the previous command and inserts it
 * into the current edit line.  If a numeric arg is supplied then the n'th
 * word from the start of the previous command is used.
 */

static int F_LOCAL EMACS_GetWordsFromHistory (int c)
{
    char	*rcp;
    char	*cp;

    if ((cp = GetHistoryRecord (CurrentHistoryEvent - 1)) == (char *)NULL)
	return EMACS_Error (0);

    if (emacs_LastCommand != EMACS_SetArgValue)
    {
	rcp = &cp[strlen(cp) - 1];

/* ignore white-space after the last word */

	while (rcp > cp && isspace (*rcp))
	    rcp--;

	while (rcp > cp && !isspace (*rcp))
	    rcp--;

	if (isspace (*rcp))
	    rcp++;

	EMACS_InsertString (rcp);
    }

    else
    {
	int c;

	rcp = cp;

/* ignore white-space at start of line */

	while (*rcp && isspace (*rcp))
	    rcp++;

	while (emacs_ArgumentCount-- > 1)
	{
	    while (*rcp && !isspace (*rcp))
		rcp++;

	    while (*rcp && isspace (*rcp))
		rcp++;
	}

	cp = rcp;

	while (*rcp && !isspace (*rcp))
	    rcp++;

	c = *rcp;
	*rcp = 0;
	EMACS_InsertString (cp);
	*rcp = (char)c;
    }

    emacs_ArgumentCount = 0;
    return EMACS_KEY_NORMAL;
}

/*
 * Inserts a # (pound sign) at the beginning of the line and then execute
 * the line.  This causes a comment to be inserted in the history file.
 */

static int F_LOCAL EMACS_Comment (int c)
{
    EMACS_GotoColumn (ConsoleLineBuffer);
    EMACS_InsertString ("#");
    return EMACS_NewLine (c);
}

/*
 * Search the alias list for an alias named \fI_Letter\fR.  If an alias of
 * this name is defined, its value is placed into the input queue.
 */

static int F_LOCAL	EMACS_AliasInsert (int c)
{
    char	*p = (char *)NULL;

/* Ctrl-] as the char means get the next character */

    if ((c & 0x0ff) == (']' & 0x1f))
	c = EMACS_GetNonFunctionKey ();

    if (isalpha (c & 0x0ff))
	p = GEN_FindAliasMatch (c & 0x0ff);

    if (p != (char *)NULL)
	emacs_CurrentMacroString = p;

    else
	EMACS_Error (0);

    return EMACS_KEY_NORMAL;
}

/*
 * EMACS_FoldCase - convert word to UPPER/lower case.  This function is used
 * to implement M-u,M-l and M-c to upper case, lower case or Capitalize
 * words.
 */

static int F_LOCAL EMACS_FoldCase (int c)
{
    register char	*cp = emacs_CurrentPosition;

    if (cp == emacs_EndOfLine)
    {
	RingWarningBell ();
	return 0;
    }

    if (emacs_LastCommand != EMACS_SetArgValue)
	emacs_ArgumentCount = 1;

/* Remove pre-fix */

    c &= 0x0ff;

/* Process! */

    while (emacs_ArgumentCount--)
    {

/* First skip over any white-space */

	if (isupper (c))
	{
	    while ((cp != emacs_EndOfLine) && EMACS_IS_SPACE (*cp))
	      cp++;
	}

/*
 * Do the first char on its own since it may be a different action than for
 * the rest.
 */

	if (cp != emacs_EndOfLine)
	{
	    if (c == 'L')			/* M-l */
	    {
		if (isupper (*cp))
		    *cp = (char)tolower (*cp);
	    }

/* M-u or M-c */

	    else if (islower (*cp))
		*cp = (char)toupper (*cp);

	    cp++;
	}

/* If command was in lower case, only the current character */

	if (islower (c))
	    continue;

/* now for the rest of the word */

	while ((cp != emacs_EndOfLine) && !EMACS_IS_SPACE (*cp))
	{
	    if (c == 'U')			/* M-u */
	    {
		if (islower (*cp))
		    *cp = (char)toupper (*cp);
	    }

/* M-l or M-c */

	    else if (isupper (*cp))
		*cp = (char)tolower (*cp);

	    cp++;
	}
    }

    EMACS_GotoColumn (cp);
    return 0;
}

/*
 * Check argument count value
 */

static void F_LOCAL	EMACS_CheckArgCount (void)
{
    if ((emacs_LastCommand != EMACS_SetArgValue) ||
	(!emacs_ArgumentCount))
	emacs_ArgumentCount = 1;
}
#endif

/*
 * GENERAL APIs
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_VI) || defined (FLAGS_GMACS)

/*
 * Redraw the window
 */

static void F_LOCAL GEN_Redraw (int limit)
{
    int		i, j;
    char	*cp;

    AdjustOK = FALSE;

    if (limit == -1)
	GEN_PutACharacter (CHAR_NEW_LINE);

    else
	GEN_PutACharacter (CHAR_RETURN);

    FlushStreams ();

    if (emacs_StartVisible == ConsoleLineBuffer)
    {
	OutputUserPrompt (LastUserPrompt);
	CurrentScreenColumn = ReadCursorPosition () % MaximumColumns;
    }

    DisplayWidth = MaximumColumns - 2 - CurrentScreenColumn;
    LastVisibleCharValid = FALSE;

    cp = GEN_FindLastVisibleCharacter ();

    GEN_AdjustOutputString (emacs_StartVisible);

    if ((emacs_StartVisible != ConsoleLineBuffer) ||
	(emacs_EndOfLine > emacs_LastVisibleCharacter))
	limit = MaximumColumns;

    if (limit >= 0)
    {
	if (emacs_EndOfLine > emacs_LastVisibleCharacter)
	    i = 0;			/* we fill the line */

	else
	    i = limit - (emacs_LastVisibleCharacter - emacs_StartVisible);

	for (j = 0; j < i && CurrentScreenColumn < (MaximumColumns - 2); j++)
	    GEN_PutACharacter (CHAR_SPACE);

	i = CHAR_SPACE;

/* more off screen ? */

	if (emacs_EndOfLine > emacs_LastVisibleCharacter)
	{
	    if (emacs_StartVisible > ConsoleLineBuffer)
		i = '*';

	    else
		i = '>';
	}

	else if (emacs_StartVisible > ConsoleLineBuffer)
	    i = '<';

	GEN_PutACharacter (i);
	j++;

	while (j--)
	    GEN_PutACharacter (CHAR_BACKSPACE);
    }

    for (cp = emacs_LastVisibleCharacter; cp > emacs_CurrentPosition; )
	GEN_BackspaceOver (*--cp);

    AdjustOK = TRUE;
}

/*
 * GEN_FindLastVisibleCharacter - last visible char.  This function returns
 * a pointer to that char in the edit buffer that will be the last displayed
 * on the screen.  The sequence:
 *
 *      for (cp = GEN_FindLastVisibleCharacter (); cp > emacs_CurrentPosition;
 *	     cp)
 *        GEN_BackspaceOver (*--cp);
 *
 * Will position the cursor correctly on the screen.
 *
 */

static char * F_LOCAL GEN_FindLastVisibleCharacter (void)
{
    register char	*rcp;
    register int	i = 0;

    if (!LastVisibleCharValid)
    {
	for (rcp = emacs_StartVisible;
	     (rcp < emacs_EndOfLine) && (i < DisplayWidth); rcp++)
	    i += GEN_GetCharacterSize (*rcp);

	emacs_LastVisibleCharacter = rcp;
    }

    LastVisibleCharValid = TRUE;
    return (emacs_LastVisibleCharacter);
}

/*
 * Output character string
 */

static void F_LOCAL GEN_AdjustOutputString (char *str)
{
    int		adj = AdjustDone;

    GEN_FindLastVisibleCharacter ();

    while (*str && (str < emacs_LastVisibleCharacter) && (adj == AdjustDone))
	GEN_OutputCharacterWithControl (*(str++));
}

/*
 * Output character, accounting for control chars
 */

static void F_LOCAL GEN_OutputCharacterWithControl (int c)
{
#ifdef EMACS_TABS
    if (c == CHAR_TAB)
	GEN_PutAString ("    ");

    else
#endif

    if ((c < CHAR_SPACE) || (c == 0x7F))
    {
	GEN_PutACharacter (CHAR_XOR);
	c += '@';
    }

    GEN_PutACharacter (c);
}

/*
 * Backspace over a character
 */

static void F_LOCAL GEN_BackspaceOver (int c)
{
    int		 i = GEN_GetCharacterSize (c);

    while (i--)
	GEN_PutACharacter (CHAR_BACKSPACE);
}

/*
 * Get number of position on screen a character takes up
 */

static int F_LOCAL GEN_GetCharacterSize (int c)
{
#ifdef EMACS_TABS
    if (c == CHAR_TAB)
	return 4;	/* Kludge, tabs are always four spaces. */
#endif

    return ((c < CHAR_SPACE) || (c == 0x7F)) ? 2 : 1;
}

/*
 * Output a string
 */

static void F_LOCAL GEN_PutAString (char *s)
{
    register int	adj = AdjustDone;

    while (*s && (adj == AdjustDone))
	GEN_PutACharacter (*(s++));
}

/*
 * Output a character
 */

static void F_LOCAL GEN_PutACharacter (int c)
{
    if ((c == CHAR_RETURN) || (c == CHAR_NEW_LINE))
	CurrentScreenColumn = 0;

    if (CurrentScreenColumn < MaximumColumns)
    {
	fputchar (c);

	switch (c)
	{
	    case CHAR_RETURN:
	    case CHAR_NEW_LINE:
		break;

	    case CHAR_BACKSPACE:
		CurrentScreenColumn--;
		break;

	    default:
		CurrentScreenColumn++;
		break;
	}
    }

    if (AdjustOK &&
	((CurrentScreenColumn < 0) ||
	 (CurrentScreenColumn >= (MaximumColumns - 2))))
	GEN_AdjustRedraw ();
}

/*
 * GEN_AdjustRedraw - redraw the line adjusting starting point etc.
 *
 * This function is called when we have exceeded the bounds of the edit
 * window.  It increments AdjustDone so that functions like EMACS_InsertString
 * and EMACS_DeleteString know that we have been called and can skip the
 * GEN_BackspaceOver () stuff which has already been done by GEN_Redraw.
 */

static void F_LOCAL GEN_AdjustRedraw (void)
{
    AdjustDone++;		/* flag the fact that we were called. */

/* we had a problem if the prompt length > MaximumColumns / 2 */

    if ((emacs_StartVisible = emacs_CurrentPosition - (DisplayWidth / 2))
			    < ConsoleLineBuffer)
	emacs_StartVisible = ConsoleLineBuffer;

    LastVisibleCharValid = FALSE;
    GEN_Redraw (MaximumColumns);
    FlushStreams ();
}

/*
 * General Alias Search function
 */

static char * F_LOCAL GEN_FindAliasMatch (int c)
{
    char	RAlias[3];
    AliasList	*p;

    RAlias[0] = '_';
    RAlias[1] = (char)c;
    RAlias[2] = 0;

    if (((p = LookUpAlias (RAlias, FALSE)) == (AliasList *)NULL) ||
	(p->value == null))
	return (char *)NULL;

/* Alias Found */

    return p->value;
}
#endif
