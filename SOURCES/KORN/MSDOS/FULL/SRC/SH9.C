/*
 * MS-DOS SHELL - History Processing
 *
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
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh9.c,v 2.18 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh9.c,v $
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
 *	Revision 2.13  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.12  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.11  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.10  1993/06/16  13:50:00  istewart
 *	Fix defaults for Key initialisation
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <setjmp.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include "sh.h"

#if (OS_TYPE == OS_UNIX)
#  include <termios.h>
#endif

#if (OS_TYPE == OS_DOS)
#  ifndef _NKEYBRD_READ
#    define _NKEYBRD_READ		0x10	/* read next char from kbd */
#    define _NKEYBRD_READY		0x11	/* check for keystroke	*/
#    define _NKEYBRD_SHIFTSTATUS	0x12	/* get shift key status	*/
#  endif

#  ifndef _KEYBRD_READ		
#    define _KEYBRD_READ       		0x00	/* read next char from kbd */
#    define _KEYBRD_READY      		0x01	/* check for keystroke	*/
#    define _KEYBRD_SHIFTSTATUS		0x02	/* get shift key status	*/
#  endif

#  if defined (__TURBOC__)
#    define _bios_keybrd(a)		bioskey (a)
#  elif defined (__EMX__)
unsigned int	 			_bios_keybrd (unsigned int);
#  endif
#endif

/* Keyboard functions */

#define KF_LENGTH			ARRAY_SIZE (KF_List)

/*
 * Saved command line structure
 */

struct	cmd_history {
    int		number;
    char	*command;
};

/* Function Declarations */

static bool F_LOCAL	ProcessAlphaNumericKey (int);
static bool F_LOCAL	ProcessFunctionKey (int);
static bool F_LOCAL	Process_History (int);
static bool F_LOCAL	ScanHistory (void);
static void F_LOCAL	ReDisplayInputLine (void);
static void F_LOCAL	PageHistoryRecord (int);
static bool F_LOCAL	UpdateConsoleInputLine (char *);
static bool F_LOCAL	ReStartInput (char *);
static void F_LOCAL	GenerateNewCursorPosition (void);
static void F_LOCAL	EraseToEndOfLine (void);
static bool F_LOCAL	CompleteFileName (char *, bool);
static void F_LOCAL	InitialiseInput (bool);
static void F_LOCAL	PrintOutHistory (FILE *, bool, struct cmd_history *);
static void F_LOCAL	ReleaseCommandMemory (struct cmd_history *);
static void F_LOCAL	SaveCurrentHistory (void);
static void F_LOCAL	memrcpy (char *, char *, int);
static FILE * F_LOCAL	OpenHistoryFile (char *);
static void F_LOCAL	GetLineFromConsole (void);
#ifdef KEYDEBUG
static void		DisplayKeyCode (unsigned int, unsigned int);
#else
#  define DisplayKeyCode(a,b)
#endif


#if (OS_TYPE != OS_DOS) 
static void F_LOCAL	ChangeKeyboardMode (bool);
#else
#  define ChangeKeyboardMode(a)
#endif

#if (OS_TYPE == OS_NT)
static bool F_LOCAL	DoNTKeyMap (KEY_EVENT_RECORD *);
#endif

#if (OS_TYPE == OS_DOS) && (OS_SIZE == OS_16)
static bool		DESQviewActive = FALSE;/* Poll keyboard		*/
#endif

static bool F_LOCAL	InsertACharacter (int);
static int F_LOCAL	OutputACharacter (int);
static int F_LOCAL	GetLineFromDevice (void);

#if (OS_TYPE == OS_DOS)
static void F_LOCAL	CheckKeyboardPolling (void);
					/* Read keyboard parameter	*/
static unsigned int	ReadKeyboardOption = _KEYBRD_READ;
#else
#  define CheckKeyboardPolling()
#endif

static bool	InsertMode = FALSE;
static char	*c_buffer_pos;		/* Position in command line	*/
static char	*EndOfCurrentLine;	/* End of command line		*/
static int	m_line = 0;		/* Max write line number	*/
static int	c_history = -1;		/* Current entry		*/
static int	l_history = 0;		/* End of history array		*/
static int	M_length = -1;		/* Match length			*/
static int	MaxLength = 0;		/* Max line length		*/
static int	OldMaxLength = 0;	/* Previous Max line length	*/
static int	CurrentHistorySize = 0;	/* Current Length of History	*/
					/* Array			*/
static bool	AppendHistory = FALSE;	/* Append to history		*/
static bool	SaveHistory = FALSE;	/* Save history			*/
static char	*No_prehistory   = "No previous commands";
static char	*No_MatchHistory = "No history match found";
static char	*No_posthistory  = "No more commands";
static char	*History_2long   = "History line too long";

#if (OS_TYPE == OS_NT)
static HANDLE	NT_StdIn;		/* Standard Input handler	*/
static HANDLE	NT_StdOut;		/* Standard Ouput handler	*/

#define CTRL_PRESSED	((LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
#define ALT_PRESSED	((LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))

/* Scan codes to ignore on input */

static WORD	IgnoreScanCode[] = {
    0x1d,				/* Control key			*/
    0x38,				/* Alt key			*/
    0x2a,				/* Shift key			*/
    0x36,				/* Shift key			*/
    0x3a,				/* Caps key			*/
    0x45				/* Numlock key			*/
};

#define IGNORE_CNT	ARRAY_SIZE (IgnoreScanCode) 

/* Mapping table because we don't get converted scan codes */


struct NT_KFMap {
    unsigned char	Normal;		/* Normal scancode	*/
    unsigned char	Shift;		/* shift scancode	*/
    unsigned char	Control;	/* Control scancode	*/
    unsigned char	Alt;		/* Alt scancode		*/
} NT_KFMap [] =  {
    /* Key 		Normal  Shift-  Control-  Alt-	*/
    { /* F1 */		0x3b,   0x54,   0x5E,     0x68 },
    { /* F2 */		0x3c,   0x55,   0x5F,     0x69 },
    { /* F3 */		0x3d,   0x56,   0x60,     0x6A },
    { /* F4 */		0x3e,   0x57,   0x61,     0x6B },
    { /* F5 */		0x3f,   0x58,   0x62,     0x6C },
    { /* F6 */		0x40,   0x59,   0x63,     0x6D },
    { /* F7 */		0x41,   0x5A,   0x64,     0x6E },
    { /* F8 */		0x42,   0x5B,   0x65,     0x6F },
    { /* F9 */		0x43,   0x5C,   0x66,     0x70 },
    { /* F10 */		0x44,   0x5D,   0x67,     0x71 },
    { /* F11 */		0x85,   0x87,   0x89,     0x8b },
    { /* F12 */		0x86,   0x88,   0x8a,     0x8c },

    { /* INSERT */	0x52,   0x52,   0x00,     0x00 },
    { /* HOME */	0x47,   0x47,   0x77,     0x00 },
    { /* PAGE UP */	0x49,   0x49,   0x84,     0x00 },
    { /* DELETE */	0x53,   0x53,   0x00,     0x00 },
    { /* END */		0x4f,   0x4f,   0x75,     0x00 },
    { /* PAGE DOWN */	0x51,   0x51,   0x76,     0x00 },

    { /* UP ARROW */	0x48,   0x48,   0x00,     0x00 },
    { /* LEFT ARROW */	0x4b,   0x4b,   0x73,     0x00 },
    { /* DOWN ARROW */	0x50,   0x50,   0x00,     0x00 },
    { /* RIGHT ARROW */	0x4d,   0x4d,   0x74,     0x00 },
};

#define MAX_NT_FKMAP	ARRAY_SIZE (NT_KFMap)
#endif

/* Function Key table */

static struct Key_Fun_List {
    char		*kf_name;
    unsigned char	akey;
    unsigned char	fkey;
    unsigned char	fcode;
} KF_List[] = {
    { "ScanBackward",	0,	'I',	KF_SCANBACKWARD },
    { "ScanForeward",	0,	'Q',	KF_SCANFOREWARD },
    { "Previous",	0,	'H',	KF_PREVIOUS },
    { "Next",		0,	'P',	KF_NEXT },
    { "Left",		0,	'K',	KF_LEFT },
    { "Right",		0,	'M',	KF_RIGHT },
    { "WordRight",	0,	't',	KF_WORDRIGHT },
    { "WordLeft",	0,	's',	KF_WORDLEFT },
    { "Start",		0,	'G',	KF_START },
    { "Clear",		0x0ff,	'v',	KF_CLEAR },
    { "Flush",		0,	'u',	KF_FLUSH },
    { "End",		0,	'O',	KF_END },
    { "Insert",		0,	'R',	KF_INSERT },
    { "DeleteRight",	0,	'S',	KF_DELETERIGHT },
    { "DeleteLeft",	CHAR_BACKSPACE,	0,	KF_DELETELEFT },
    { "Complete",	0,	'w',	KF_COMPLETE },
    { "Directory",	0,	0x0f,	KF_DIRECTORY },
    { "ClearScreen",	0,	0x84,	KF_CLEARSCREEN },
    { "Jobs",		0,	0x68,	KF_JOBS },
    { "Transpose",	0x14,	0,	KF_TRANSPOSE },
    { "Quote",		0x11,	0,	KF_QUOTE },

/* End of function keys - flags */

    { "Bell",		1,	0,	KF_RINGBELL },
    { "HalfHeight",	0,	0,	KF_HALFHEIGTH },
    { "InsertMode",	0,	0,	KF_INSERTMODE },
    { "InsertCursor",	1,	0,	KF_INSERTCURSOR },
    { "RootDrive",	0,	0,	KF_ROOTDRIVE },
    { "EOFKey",		4,	0,	KF_EOFKEY },
};

/* Arrary of history Items */

static struct cmd_history	*cmd_history = (struct cmd_history *)NULL;

/*
 * Processing standard input.  Editting is only supported on OS/2 at the
 * moment.
 */

int GetConsoleInput (void)
{
    bool	console = C2bool (IS_Console (0));
    int		RetVal = 0;

/* Has dofc set the flag to say use the console buffer ? */

    if (UseConsoleBuffer)
    {
	UseConsoleBuffer = FALSE;
	SaveHistory = TRUE;
	SaveCurrentHistory ();
	SaveHistory = FALSE;
    }

/*
 * Read in a line from the console
 */

    else
    {

/* Set to last history item */

	if (FL_TEST (FLAG_INTERACTIVE) || (IS_TTY (0) && IS_TTY (1)))
	    SaveCurrentHistory ();

/* Zap the line and output the prompt.  Save history status info */

	memset (ConsoleLineBuffer, 0, LINE_MAX + 1);

	if (FL_TEST (FLAG_INTERACTIVE) || (IS_TTY (0) && IS_TTY (1)))
	{
	    bool	o_SaveHistory = SaveHistory;

	    OutputUserPrompt (LastUserPrompt);
	    SaveHistory = o_SaveHistory;
	}

/* Only if we are working on the console and not via a pipe or file do we
 * need to set up the console
 */

	if (console)
	{
	    CheckKeyboardPolling ();

	    if (ChangeInitLoad)
	    {
		ChangeInitLoad = FALSE;
		Configure_Keys ();
	    }

	    GetScreenParameters ();
	}

/*
 * Process the input.  If this is not the console, read from standard input
 */

	if (!console)
	    RetVal = GetLineFromDevice ();

	else 
	{
	    ChangeKeyboardMode (FALSE);

#if defined (FLAGS_VI) || defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
	    if (ShellGlobalFlags & FLAGS_EDITORS)
		RetVal = EditorInput ();

	    else
#endif
		GetLineFromConsole ();

	    ChangeKeyboardMode (TRUE);
	}

	if (LastUserPrompt == PS1)
	    LastUserPrompt = PS2;

/* Clean up */

	FlushStreams ();

	if (console)
	    SetCursorShape (FALSE);
    }

/* Return any errors */

    if (RetVal < 0)
        return 0;

/* If we are reading from the console, check for end of file.  From file or
 * pipe, we detect this differently
 */

    if (console)
    {
	if (*ConsoleLineBuffer == (char)KF_List[KF_EOFKEY].akey)
	    *ConsoleLineBuffer = 0;

	else
	    strcat (ConsoleLineBuffer, LIT_NewLine);
    }

    return strlen (ConsoleLineBuffer);
}

/*
 * Read a line from a file or pipe - well pipe really.
 */

static int F_LOCAL GetLineFromDevice (void)
{
    while (fgets (ConsoleLineBuffer, LINE_MAX, stdin) != (char *)NULL)
    {
	if (*ConsoleLineBuffer != CHAR_HISTORY)
	    return 0;
	
	else if (Process_History (0))
	{
	    puts (ConsoleLineBuffer);
	    return 0;
	}
    }

/* EOF detected - return that fact */

    return -1;
}

/*
 * Read line from Console
 */

static void F_LOCAL GetLineFromConsole (void)
{
    unsigned char	a_key, f_key;
    int			i;

/* Process the input */

    while (TRUE)
    {
	InitialiseInput ((bool)KF_List[KF_INSERTMODE].akey);

	while (((a_key = ReadKeyBoard (&f_key)) != KF_List[KF_EOFKEY].akey) &&
	       (a_key != CHAR_NEW_LINE) && (a_key != CHAR_RETURN))
	{

/* Handle Resize event */

	    if (a_key == KT_RESIZE)
	    {
		fputchar (CHAR_NEW_LINE);
		GetScreenParameters ();
		OutputUserPrompt (LastUserPrompt);
		ReDisplayInputLine ();
		continue;
	    }

/* Look up the keystroke to see if it is one of our functions */

	    if (!(i = LookUpKeyBoardFunction (a_key, f_key)))
		continue;

	    if (((i > 0) ? ProcessAlphaNumericKey (i)
	    		 : ProcessFunctionKey ((-i) - 1)))
		ReDisplayInputLine ();

/* Handle a special case on insert mode on line 25 */

	    if ((InsertMode) && (MaxLength > OldMaxLength) &&
		(((StartCursorPosition + MaxLength) / MaximumColumns)
			== MaximumLines) &&
		(((StartCursorPosition + MaxLength) % MaximumColumns) == 0))
		StartCursorPosition -= MaximumColumns;

/* Reposition the cursor */

	    GenerateNewCursorPosition ();

/* Save old line length */

	    OldMaxLength = MaxLength;
	}

/* Terminate the line */

	*(c_buffer_pos = EndOfCurrentLine) = 0;
	GenerateNewCursorPosition ();
	fputchar (CHAR_NEW_LINE);
	StartCursorPosition = -1;
	FlushStreams ();

/* Line input - check for history */

	if ((*ConsoleLineBuffer == CHAR_HISTORY) && Process_History (0))
	{
	    puts (ConsoleLineBuffer);
	    break;
	}

	else if (*ConsoleLineBuffer != CHAR_HISTORY)
	    break;
    }

    if (a_key == KF_List[KF_EOFKEY].akey)
	*EndOfCurrentLine = KF_List[KF_EOFKEY].akey;
}

/*
 * Handler Alpha_numeric characters
 */

static bool F_LOCAL ProcessAlphaNumericKey (int c)
{
    bool	redisplay = FALSE;

/* Normal character processing */

    if ((c_buffer_pos - ConsoleLineBuffer) == LINE_MAX)
	return RingWarningBell ();

    else if (!InsertMode)
    {
	if (c_buffer_pos == EndOfCurrentLine)
	    ++EndOfCurrentLine;

	else if (iscntrl (*c_buffer_pos) || iscntrl (c))
	    redisplay = TRUE;

	*(c_buffer_pos++) = (char)c;

	if (redisplay || (c == CHAR_TAB))
	    return TRUE;

/* Output the character */

	OutputACharacter (c);
	return FALSE;
    }

    else
	return InsertACharacter (c);
}

/* Process function keys */

static bool F_LOCAL ProcessFunctionKey (int fn)
{
    bool	fn_search = FALSE;
    char	tmp;

    switch (fn)
    {
	case KF_SCANBACKWARD:		/* Scan backwards in history	*/
	case KF_SCANFOREWARD:		/* Scan forewards in history	*/
	    *EndOfCurrentLine = 0;

	    if (M_length == -1)
		M_length = strlen (ConsoleLineBuffer);

	    PageHistoryRecord ((fn == KF_SCANBACKWARD) ? -1 : 1);
	    return TRUE;

	case KF_PREVIOUS:		/* Previous command		*/
	    *EndOfCurrentLine = 0;
	    Process_History (-1);
	    return TRUE;

	case KF_NEXT:			/* Next command line		*/
	    Process_History (1);
	    return TRUE;

	case KF_LEFT:			/* Cursor left			*/
	    if (c_buffer_pos != ConsoleLineBuffer)
		--c_buffer_pos;

	    else
		RingWarningBell ();

	    return FALSE;

	case KF_RIGHT:			/* Cursor right			*/
	    if (c_buffer_pos != EndOfCurrentLine)
		++c_buffer_pos;

	    else
		RingWarningBell ();

	    return FALSE;

	case KF_WORDLEFT:		/* Cursor left a word		*/
	    if (c_buffer_pos != ConsoleLineBuffer)
	    {
		--c_buffer_pos;		/* Reposition on previous char	*/

		while (isspace (*c_buffer_pos) &&
		       (c_buffer_pos != ConsoleLineBuffer))
		    --c_buffer_pos;

		while (!isspace (*c_buffer_pos) &&
		       (c_buffer_pos != ConsoleLineBuffer))
		    --c_buffer_pos;

		if (c_buffer_pos != ConsoleLineBuffer)
		    ++c_buffer_pos;
	    }

	    else
		RingWarningBell ();

	    return FALSE;

	case KF_WORDRIGHT:		/* Cursor right a word		*/
	    if (c_buffer_pos != EndOfCurrentLine)
	    {

/* Skip to the end of the current word */

		while (!isspace (*c_buffer_pos) &&
		       (c_buffer_pos != EndOfCurrentLine))
		    ++c_buffer_pos;

/* Skip over the white space */

		while (isspace (*c_buffer_pos) &&
		       (c_buffer_pos != EndOfCurrentLine))
		    ++c_buffer_pos;
	    }

	    else
		RingWarningBell ();

	    return FALSE;

	case KF_START:			/* Cursor home			*/
	    c_buffer_pos = ConsoleLineBuffer;
	    return FALSE;

	case KF_CLEAR:			/* Erase buffer			*/
	    c_buffer_pos = ConsoleLineBuffer;

	case KF_FLUSH:			/* Flush to end			*/
	    memset (c_buffer_pos, CHAR_SPACE, EndOfCurrentLine - c_buffer_pos);
	    EndOfCurrentLine = c_buffer_pos;
	    return TRUE;

	case KF_END:			/* Cursor end of command	*/
	    if (*ConsoleLineBuffer == CHAR_HISTORY)
	    {
		*EndOfCurrentLine = 0;
		Process_History (2);
		return TRUE;
	    }

	    c_buffer_pos = EndOfCurrentLine;
	    return FALSE;

	case KF_INSERT:			/* Switch insert mode		*/
	    InsertMode = C2bool (!InsertMode);
	    SetCursorShape (InsertMode);
	    return FALSE;

	case KF_CLEARSCREEN:		/* Clear the screen		*/
	    return ClearScreen ();

	case KF_TRANSPOSE:		/* Transpose characters		*/
	    if(c_buffer_pos == ConsoleLineBuffer)
		return RingWarningBell ();

	    if (c_buffer_pos == EndOfCurrentLine)
		--c_buffer_pos;

	    tmp = *(c_buffer_pos - 1);
	    *(c_buffer_pos - 1) = *c_buffer_pos;
	    *c_buffer_pos = tmp;

	    if (c_buffer_pos != EndOfCurrentLine)
		++c_buffer_pos;

	    return TRUE;

	case KF_QUOTE:			/* Quote characters		*/
	    return ProcessAlphaNumericKey ((int)ReadKeyBoard ((unsigned char *)&tmp));

#if (OS_TYPE != OS_DOS)
	case KF_JOBS:			/* Print Job Tree		*/
	    fputchar (CHAR_NEW_LINE);
#  if (OS_TYPE == OS_NT)
	    PrintJobs (TRUE);
#  else
	    PrintProcessTree (getpid ());
#  endif
	    OutputUserPrompt (LastUserPrompt);
	    StartCursorPosition = ReadCursorPosition ();
	    return TRUE;
#endif

	case KF_DELETERIGHT:		/* Delete right character	*/
	    if (c_buffer_pos == EndOfCurrentLine)
		return FALSE;

	    memcpy (c_buffer_pos, c_buffer_pos + 1,
		    EndOfCurrentLine - c_buffer_pos);

	    if (EndOfCurrentLine == ConsoleLineBuffer)
	    {
		RingWarningBell ();
		return TRUE;
	    }

	    if (--EndOfCurrentLine < c_buffer_pos)
		--c_buffer_pos;

   	    return TRUE;

	case KF_DIRECTORY:		/* File name directory		*/
	    fn_search = TRUE;

	case KF_COMPLETE:		/* File name completion		*/
	{
	    *EndOfCurrentLine = 0;
	    return CompleteFileName (c_buffer_pos, fn_search);
	}

	case KF_DELETELEFT:		/* Delete left character	*/
	    if (c_buffer_pos == ConsoleLineBuffer)
		return RingWarningBell ();

/* Decrement current position */

	    --c_buffer_pos;
	    memcpy (c_buffer_pos, c_buffer_pos + 1,
		    EndOfCurrentLine - c_buffer_pos);
	    --EndOfCurrentLine;
	    return TRUE;
    }

    return FALSE;
}

/* Set cursor shape */

#if (OS_TYPE == OS_OS2)
void	SetCursorShape (bool mode)
{
    VIOCURSORINFO	viociCursor;

    VioGetCurType (&viociCursor, 0);

    if (mode && KF_List[KF_INSERTCURSOR].akey)
	viociCursor.yStart = (USHORT)((KF_List[KF_HALFHEIGTH].akey
					    ? (viociCursor.cEnd / 2) + 1 : 1));

    else
	viociCursor.yStart = (USHORT)(viociCursor.cEnd - 1);

    VioSetCurType (&viociCursor, 0);
}
#endif

/* NT version */

#if (OS_TYPE == OS_NT)
void	SetCursorShape (bool mode)
{
    CONSOLE_CURSOR_INFO		ConsoleCursorInfo;

    GetConsoleCursorInfo (NT_StdOut, &ConsoleCursorInfo);

    if (mode && KF_List[KF_INSERTCURSOR].akey)
	ConsoleCursorInfo.dwSize = (KF_List[KF_HALFHEIGTH].akey ? 50 : 90);

    else
	ConsoleCursorInfo.dwSize = 10;

    ConsoleCursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo (NT_StdOut, &ConsoleCursorInfo);
}
#endif

/* DOS version */

#if (OS_TYPE == OS_DOS)
void	SetCursorShape (bool mode)
{
    union REGS		r;

/* Get the current cursor position to get the cursor lines */

    r.h.ah = 0x03;
    SystemInterrupt (0x10, &r, &r);

/* Reset the type */

    r.h.ah = 0x01;

    if (mode && KF_List[KF_INSERTCURSOR].akey)
	r.h.ch = (unsigned char)((KF_List[KF_HALFHEIGTH].akey
					? (r.h.cl / 2) + 1 : 1));

    else
	r.h.ch = (unsigned char)(r.h.cl - 1);

    SystemInterrupt (0x10, &r, &r);
}
#endif

/* UNIX version */

#if (OS_TYPE == OS_UNIX)
void	SetCursorShape (bool mode)
{
}
#endif

/*
 * Read Cursor position
 */

#if (OS_TYPE == OS_OS2)
int	ReadCursorPosition (void)
{
    USHORT	usRow;
    USHORT	usColumn;

    VioGetCurPos (&usRow, &usColumn, 0);
    return (MaximumColumns * usRow) + usColumn;
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
int	ReadCursorPosition (void)
{
    CONSOLE_SCREEN_BUFFER_INFO	CSI;

    GetConsoleScreenBufferInfo (NT_StdOut, &CSI);

    return (MaximumColumns * CSI.dwCursorPosition.Y) + CSI.dwCursorPosition.X;
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS) 
int	ReadCursorPosition (void)
{
    union REGS	r;

    FlushStreams ();

    r.h.ah = 0x03;				/* Read cursor position	*/
    r.h.bh = 0;					/* Page zero		*/
    SystemInterrupt (0x10, &r, &r);
    return (r.h.dh * MaximumColumns) + r.h.dl;
}
#endif

/*
 * Re-position the cursor
 */

#if (OS_TYPE == OS_OS2)
void	SetCursorPosition (int new)
{
    int		diff;
    USHORT	usRow;
    USHORT	usColumn;

    FlushStreams ();

    usRow = (USHORT)(new / MaximumColumns);
    usColumn = (USHORT)(new % MaximumColumns);

/* Are we at the bottom of the page? */

    if (usRow >= (unsigned char)MaximumLines)
    {
	diff = usRow + 1 - MaximumLines;
	usRow = (unsigned char)(MaximumLines - 1);
	StartCursorPosition -= MaximumColumns * diff;
    }

    VioSetCurPos (usRow, usColumn, 0);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
void	SetCursorPosition (int new)
{
    int		diff;
    COORD	Cp;

    FlushStreams ();

    Cp.Y = (USHORT)(new / MaximumColumns);
    Cp.X = (USHORT)(new % MaximumColumns);

/* Are we at the bottom of the page? */

    if (Cp.Y >= (unsigned char)MaximumLines)
    {
	diff = Cp.Y + 1 - MaximumLines;
	Cp.Y = (unsigned char)(MaximumLines - 1);
	StartCursorPosition -= MaximumColumns * diff;
    }

    SetConsoleCursorPosition (NT_StdOut, Cp);
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS) 
void	SetCursorPosition (int new)
{
    int		diff;
    union REGS	r;

    FlushStreams ();

    r.h.ah = 0x02;				/* Set new position	*/
    r.h.bh = 0;					/* Page zero		*/
    r.h.dh = (unsigned char)(new / MaximumColumns);
    r.h.dl = (unsigned char)(new % MaximumColumns);

/* Are we at the bottom of the page? */

    if (r.h.dh >= (unsigned char)MaximumLines)
    {
	diff = r.h.dh + 1 - MaximumLines;
	r.h.dh = (unsigned char)(MaximumLines - 1);
	StartCursorPosition -= MaximumColumns * diff;
    }

    SystemInterrupt (0x10, &r, &r);
}
#endif

/* Erase to end of line (avoid need for STUPID ansi.sys memory eater!) */

#if (OS_TYPE == OS_OS2) 
static void F_LOCAL EraseToEndOfLine (void)
{
    BYTE	abCell[2];
    USHORT	usRow;
    USHORT	usColumn;
    USHORT	cb = sizeof (abCell);

    FlushStreams ();

/* Read attribute under cursor */

    VioGetCurPos (&usRow, &usColumn, 0);
    VioReadCellStr (abCell, &cb , usRow, usColumn, 0);

    abCell[0] = CHAR_SPACE;

    if (m_line < (int)usRow)
	m_line = usRow;

    if ((cb = MaximumColumns - usColumn +
		((m_line - usRow) * MaximumColumns)) > 0)
	VioWrtNCell (abCell, cb, usRow, usColumn, 0);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static void F_LOCAL EraseToEndOfLine (void)
{
    WORD			Atts[2];
    CONSOLE_SCREEN_BUFFER_INFO	CSI;
    DWORD			ActionCount;
    DWORD			cb;
    WORD			*ostring;
    DWORD			i;

    FlushStreams ();

/* Read attribute under cursor */

    GetConsoleScreenBufferInfo (NT_StdOut, &CSI);

    ReadConsoleOutputAttribute (NT_StdOut, Atts, 1, CSI.dwCursorPosition,
    				&ActionCount);

/* How much to write ? */

    if (m_line < (int) CSI.dwCursorPosition.Y)
	m_line = CSI.dwCursorPosition.Y;

/* Write it.  What a pain!!! */

    if ((cb = MaximumColumns - CSI.dwCursorPosition.X +
		((m_line - CSI.dwCursorPosition.Y) * MaximumColumns)) > 0)
    {
	ostring = (WORD *)AllocateMemoryCell (cb * sizeof (WORD));
	memset (ostring, CHAR_SPACE, cb);

	WriteConsoleOutputCharacter (NT_StdOut, (char *)ostring, cb,
				     CSI.dwCursorPosition, &ActionCount);

/* Set up attributes */

    	for (i = 0; i < cb; i++)
	    ostring[i] = Atts[0];

	WriteConsoleOutputAttribute (NT_StdOut, ostring, cb,
				     CSI.dwCursorPosition, &ActionCount);
        ReleaseMemoryCell (ostring);
    }
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS) 
static void F_LOCAL EraseToEndOfLine (void)
{
    union REGS		r;
    unsigned char	backg;

    FlushStreams ();

/* Get the background attribute of the cursor */

    r.h.ah = 0x08;
    r.h.bh = 0;
    SystemInterrupt (0x10, &r, &r);
    backg = (unsigned char)(r.h.ah & 0x07);

    r.h.ah = 0x03;
    r.h.bh = 0;
    SystemInterrupt (0x10, &r, &r);

/* Check that we use the correct m_line */

    if (m_line < (int)r.h.dh)
	m_line = r.h.dh;

    if ((r.x.REG_CX = MaximumColumns - r.h.dl +
		      ((m_line - r.h.dh) * MaximumColumns)) > 0)
    {
	r.x.REG_AX = 0x0a20;
	r.x.REG_BX = backg;
	SystemInterrupt (0x10, &r, &r);
    }
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_UNIX) 
static void F_LOCAL EraseToEndOfLine (void)
{
    fputs ("UNIX: EraseToEndOfLine NI\n", stderr);
}
#endif

/*
 * Generate the new cursor position
 */

static void F_LOCAL GenerateNewCursorPosition (void)
{
    char	*cp = ConsoleLineBuffer - 1;
    int		off = StartCursorPosition;

/* Search to current position */

    while (++cp != c_buffer_pos)
    {
	if (*cp == CHAR_TAB)
	    while ((++off) % 8);

	else
	    off += (iscntrl (*cp)) ? 2 : 1;
    }

/* Position the cursor */

    SetCursorPosition (off);
}

/* Redisplay the current line */

static void F_LOCAL ReDisplayInputLine (void)
{
/* Reposition to start of line */

    SetCursorPosition (StartCursorPosition);

/* Output the line */

    *EndOfCurrentLine = 0;
    DisplayLineWithControl (ConsoleLineBuffer);

    if ((m_line = ((StartCursorPosition + MaxLength) / MaximumColumns) + 1) >=
	MaximumLines)
	m_line = MaximumLines - 1;

    EraseToEndOfLine ();		/* clear to end of line	*/
    MaxLength = EndOfCurrentLine - ConsoleLineBuffer;
}

/* Process history command
 *
 * -1: Previous command
 *  1: Next command
 *  0: Current command
 *  2: Current command with no options processing
 */

static bool F_LOCAL Process_History (int direction)
{
    char	*optionals = null;

    c_buffer_pos = ConsoleLineBuffer;
    EndOfCurrentLine = ConsoleLineBuffer;
    c_history += (direction == 2) ? 0 : direction;

    switch (direction)
    {
	case -1:			/* Move up one line		*/
	    if (c_history < 0)
	    {
		c_history = -1;
		return ReStartInput (No_prehistory);
	    }

	    break;

	case 1:				/* Move to next history line	*/
	    if (c_history >= l_history)
	    {
		c_history = l_history;
		return ReStartInput (No_posthistory);
	    }

	    break;

	case 0:				/* Check out ConsoleLineBuffer	*/
	    optionals = ConsoleLineBuffer;/* Are there any additions to	*/
					/* the history line		*/

/* Find the end of the first part */

	    while (!isspace (*optionals) && *optionals)
	    {
		if (*optionals == CHAR_HISTORY)
		{

/* Terminate at !! */

		    if (*(optionals + 1) == CHAR_HISTORY)
		    {
			optionals += 2;
			break;
		    }

/* Terminate at a numeric value */

		    else if (isdigit (*(optionals + 1)) ||
			     (*(optionals + 1) == '-'))
		    {
			optionals += 2;
			while (isdigit (*optionals))
			    ++optionals;

			break;
		    }
		}

		++optionals;
	    }

/* Copy selected item into line buffer */

	case 2:
	    M_length = (optionals == null) ? strlen (ConsoleLineBuffer) - 1
					   : optionals - ConsoleLineBuffer - 1;

	    if (!ScanHistory ())
		return FALSE;

	    break;
    }

    return UpdateConsoleInputLine (optionals);
}

/* Ok c_history points to the new line.  Move optionals after history
 * and the copy in history and add a space
 */

static bool F_LOCAL UpdateConsoleInputLine (char *optionals)
{
    int		opt_len;

    EndOfCurrentLine = &ConsoleLineBuffer[strlen (cmd_history[c_history].command)];

    if ((EndOfCurrentLine - ConsoleLineBuffer +
	 (opt_len = strlen (optionals)) + 1) >= LINE_MAX)
	return ReStartInput (History_2long);

    if (EndOfCurrentLine > optionals)
	memrcpy (EndOfCurrentLine + opt_len, optionals + opt_len, opt_len + 1);

    else
	strcpy (EndOfCurrentLine, optionals);

    strncpy (ConsoleLineBuffer, cmd_history[c_history].command,
	     (EndOfCurrentLine - ConsoleLineBuffer));
    EndOfCurrentLine = &ConsoleLineBuffer[strlen (ConsoleLineBuffer)];
    c_buffer_pos = EndOfCurrentLine;
    return TRUE;
}

/* Scan the line buffer for a history match */

static bool F_LOCAL ScanHistory (void)
{
    char	*cp = ConsoleLineBuffer + 1;
    char	*ep;
    int		i = (int)strtol (cp, &ep, 10);

/* Get the previous command ? (single ! or double !!) */

    if ((M_length == 0) || (*cp == CHAR_HISTORY))
    {
	if (c_history >= l_history)
	    c_history = l_history - 1;

	if (c_history < 0)
	    return ReStartInput (No_prehistory);

	return TRUE;
    }

/* Request for special history number item.  Check History file empty */

    if (l_history == 0)
	return ReStartInput (No_MatchHistory);

/* Check for number */

    if ((*ConsoleLineBuffer == CHAR_HISTORY) && (ep > cp) && M_length)
    {
	M_length = -1;

	for (c_history = l_history - 1;
	    (c_history >= 0) && (cmd_history[c_history].number != i);
	    --c_history)
	    continue;
    }

/* No - scan for a match */

    else
    {
	for (c_history = l_history - 1;
	     (c_history >= 0) &&
	     (strncmp (cp, cmd_history[c_history].command, M_length) != 0);
	     --c_history)
	    continue;
    }

/* Anything found ? */

    if (c_history == -1)
    {
	c_history = l_history - 1;
	return ReStartInput (No_MatchHistory);
    }

    return TRUE;
}

/*
 * Get record associated with event
 */

char	*GetHistoryRecord (int event)
{
    int		i = l_history - 1;

    while (i >= 0)
    {
        if (cmd_history[i].number == event)
	    return cmd_history[i].command;

	--i;
    }

    return (char *)NULL;
}

/*
 * Scan back or forward from current history
 */

static void F_LOCAL PageHistoryRecord (int direction)
{
    c_buffer_pos = ConsoleLineBuffer;
    EndOfCurrentLine = ConsoleLineBuffer;

    if (l_history == 0)
    {
	ReStartInput (No_MatchHistory);
	return;
    }

/* scan for a match */

    while (((c_history += direction) >= 0) && (c_history != l_history) &&
	   (strncmp (ConsoleLineBuffer, cmd_history[c_history].command,
		     M_length) != 0))
	continue;

/* Anything found ? */

    if ((c_history < 0) || (c_history >= l_history))
    {
	c_history = l_history - 1;
	ReStartInput (No_MatchHistory);
    }

    else
	UpdateConsoleInputLine (null);
}

/* Load history file */

void LoadHistory (void)
{
    FILE		*fp;
    char		*cp;
    int			i = 0;
    bool		Append = FALSE;

/* Initialise history array */

    c_history = -1;			/* Current entry		*/
    l_history = 0;			/* End of history array		*/

    if (GetVariableAsString (HistoryFileVariable, FALSE) == null)
    {
	SetVariableFromString (HistoryFileVariable,
			       (cp = BuildFileName ("history.sh")));
	ReleaseMemoryCell ((void *)cp);
    }

    if (GetVariableAsString (HistorySizeVariable, FALSE) == null)
	SetVariableFromNumeric (HistorySizeVariable, 100L);

    if ((fp = OpenHistoryFile (sOpenReadMode)) == (FILE *)NULL)
	return;

/* Read in file */

    cp = ConsoleLineBuffer;

    while ((i = fgetc (fp)) != EOF)
    {
	if (i == 0)
	{
	    *cp = 0;
	    cp = ConsoleLineBuffer;
	    AddHistory (Append);
	    Append = FALSE;
	}

	else if (i == CHAR_NEW_LINE)
	{
	    *cp = 0;
	    cp = ConsoleLineBuffer;
	    AddHistory (Append);
	    Append = TRUE;
	}

	else if ((cp - ConsoleLineBuffer) < LINE_MAX - 2)
	    *(cp++) = (char)i;
    }

    CloseFile (fp);
}

/* Open the history file */

static FILE * F_LOCAL OpenHistoryFile (char *mode)
{
    char	*name;

    if (!HistoryEnabled)
	return (FILE *)NULL;

    name = CheckDOSFileName (GetVariableAsString (HistoryFileVariable, FALSE));
    return FOpenFile (name, mode);
}

/* Add entry to history file */

void AddHistory (bool AppendToLast)
{
    char		*cp;
    struct cmd_history	*cmd;
    size_t		Len;
    int			HistorySize;

/* Clean up console buffer */

    CleanUpBuffer (strlen (ConsoleLineBuffer), ConsoleLineBuffer, GetEOFKey ());

/*
 * Ignore if no history or blank line
 */

    if ((!HistoryEnabled) || (strlen (ConsoleLineBuffer) == 0))
	return;

/* Has the size changed ? */

    HistorySize = (int)GetVariableAsNumeric (HistorySizeVariable);

    if (HistorySize != CurrentHistorySize)
    {

/* Zero - empty history */

	if (!HistorySize)
	    ClearHistory ();

/* Allocate a new buffer */

	else if ((cmd = (struct cmd_history *)GetAllocatedSpace
			    (sizeof (struct cmd_history) * HistorySize)) ==
		   (struct cmd_history *)NULL)
	    /* DO NOTHING IF NO MEMORY */;

/* If new buffer is bigger, copy old to new and release */

	else if ((HistorySize > CurrentHistorySize) ||
		 (l_history < HistorySize))
	{
	    if (cmd_history != (struct cmd_history *)NULL)
	    {
		int	Clen;

/* Calculate the length to copy */

		Clen = (HistorySize > CurrentHistorySize)
			? CurrentHistorySize : l_history;

		memcpy (cmd, cmd_history,
			sizeof (struct cmd_history) * Clen);

/* Set up new values */

		ReleaseMemoryCell (cmd_history);
	    }

	    CurrentHistorySize = HistorySize;
	    SetMemoryAreaNumber ((void *)(cmd_history = cmd), 0);
	}

/* Less space is available, copy reduced area and update entry numbers */

	else
	{
	    int		i = (CurrentHistorySize - HistorySize);

/* Free entries at bottom */

	    for (Len = 0; (Len < (size_t)i) && (Len < (size_t)l_history); Len++)
		ReleaseCommandMemory (&cmd_history[Len]);

/* Transfer entries at top */

	    memcpy (cmd, &cmd_history[i],
		    sizeof (struct cmd_history) * HistorySize);

/* Update things */

	    if ((c_history -= i) < -1)
		c_history = -1;

	    if ((l_history -= i) < 0)
		l_history = 0;

/* Set up new values */

	    ReleaseMemoryCell (cmd_history);
	    CurrentHistorySize = HistorySize;
	    SetMemoryAreaNumber ((void *)(cmd_history = cmd), 0);
	}
    }

/* If there is no history space - return */

    if (!CurrentHistorySize)
	return;

/* If the array is full, remove the last item */

    if (l_history == CurrentHistorySize)
    {
	ReleaseCommandMemory (&cmd_history[0]);

	--l_history;
	memcpy (&cmd_history[0], &cmd_history[1],
		sizeof (struct cmd_history) * (CurrentHistorySize - 1));
    }

/* Save the string.  Is this a PS2 prompt */

    if ((AppendToLast) && l_history)
    {
	cmd = &cmd_history[l_history - 1];

/* Check length */

	if ((Len = strlen (cmd->command) + strlen (ConsoleLineBuffer) + 2)
	    >= LINE_MAX)
	{
	    fprintf (stderr, BasicErrorMessage, LIT_history, History_2long);
	    return;
	}

/* Append to buffer, reallocate to new length */

	if ((cp = GetAllocatedSpace (Len)) == (char *)NULL)
	    return;

	sprintf (cp, "%s\n%s", cmd->command, ConsoleLineBuffer);

	ReleaseCommandMemory (cmd);
	cmd->command = cp;
	SetMemoryAreaNumber ((void *)cp, 0);
    }

/* Save the command line */

    else
    {
	Current_Event = GetLastHistoryEvent ();
	cmd_history[l_history].number = Current_Event;

	cmd_history[l_history].command = StringSave (ConsoleLineBuffer);
	c_history = ++l_history;
    }
}

/*
 * Dump history to file
 */

void DumpHistory (void)
{
    struct cmd_history	*cp = cmd_history;
    FILE		*fp;
    int			i;

/* If history is not enabled or we can't open the file, give up */

    if ((!HistoryEnabled) ||
	((fp = OpenHistoryFile (sOpenWriteMode)) == (FILE *)NULL))
	return;

/* Write history as a null terminated record */

    for (i = 0; i < l_history; ++cp, ++i)
    {
        fputs (cp->command, fp);
	fputc (0, fp);
    }

    CloseFile (fp);
}

/* Clear out history */

void ClearHistory (void)
{
    int			i;
    struct cmd_history	*cp = cmd_history;

/* Release the entries */

    for (i = 0; i < l_history; ++cp, ++i)
	ReleaseCommandMemory (cp);

    ReleaseMemoryCell (cmd_history);

/* Reset data information */

    c_history = -1;			/* Current entry		*/
    l_history = 0;			/* End of history array		*/
    Current_Event = 0;
    CurrentHistorySize = 0;
    cmd_history = (struct cmd_history *)NULL;
}

/* Output warning message and prompt */

static bool F_LOCAL ReStartInput (char *cp)
{
    if (cp != (char *)NULL)
    {
	if (!IS_Console (1) ||
	    (strlen (ConsoleLineBuffer) && (StartCursorPosition != -1)))
	    feputc (CHAR_NEW_LINE);

	PrintWarningMessage (BasicErrorMessage, LIT_history, cp);

	if (IS_Console (1))
	    EraseToEndOfLine ();

	fputchar (CHAR_NEW_LINE);
    }

    OutputUserPrompt (LastUserPrompt);

/* Re-initialise */

    InitialiseInput (InsertMode);
    return FALSE;
}

/* Copy backwards */

static void F_LOCAL memrcpy (char *sp1, char *sp, int cnt)
{
    while (cnt--)
	*(sp1--) = *(sp--);
}

/* Complete Filename functions */

static bool F_LOCAL CompleteFileName (char *InsertPosition, bool Searching)
{
    int			NumberOfMatches = 0;
    char		*SearchString = null;
    char		*NameStart = InsertPosition;
    char		*StartDirInCLB;
    size_t		MatchStringLen = 0;
    char		*cp;
    int			i;
    size_t		maxi;
    bool		InsertSpace = TRUE;
    char		**FileList;

/* Space or at start of line - use NULL file name */

    if ((NameStart != ConsoleLineBuffer) && isspace (*NameStart) &&
	!isspace (*(NameStart - 1)))
    {
	--NameStart;
	InsertSpace = FALSE;
    }

    if (!isspace (*NameStart))
    {
	while (!isspace (*NameStart) && (NameStart != ConsoleLineBuffer))
	    --NameStart;

	if (isspace (*NameStart))
	    ++NameStart;

	MatchStringLen = InsertPosition - NameStart;

	if ((SearchString = AllocateMemoryCell (MatchStringLen + 1)) ==
						(char *)NULL)
	    return RingWarningBell ();

	memcpy (SearchString, NameStart, MatchStringLen);
    }

/* Find the directory name */

    if ((cp = FindLastPathCharacter (SearchString)) != (char *)NULL)
	StartDirInCLB = NameStart + (int)(cp - SearchString + 1);

/* No directory flag - Drive specifier? */

    else if ((strlen (SearchString) > (size_t)1) &&
	     IsDriveCharacter (*(SearchString + 1)))
	StartDirInCLB = NameStart + 2;

/* No drive specifier */

    else
	StartDirInCLB = NameStart;

    FileList = BuildCompletionList (SearchString, strlen (SearchString),
				    &NumberOfMatches, FALSE);

    if (SearchString != null)
        ReleaseMemoryCell ((void *)SearchString);

/* If there are no matches, Just ring the bell */

    if (FileList == (char **)NULL)
	return RingWarningBell ();

/* At this point, we have some data.  If we are searching, sort the
 * filenames and display them.  Remember to release the memory allocated for
 * the word block and its entries.
 */

    if (Searching)
    {

/* Sort the file names and display them */

	qsort (&FileList[0], NumberOfMatches, sizeof (char *), SortCompare);

/* Display. */

	fputchar (CHAR_NEW_LINE);
	PrintAList (NumberOfMatches, FileList);

/* Release memory */

	ReleaseAList (FileList);

/* Redraw current line */

	OutputUserPrompt (LastUserPrompt);
	StartCursorPosition = ReadCursorPosition ();
	return TRUE;
    }

/* OK, we are completing. Get the common part of the filename list */

    maxi = GetCommonPartOfFileList (FileList);

/* If the file name is length matches the search length, there are no unique
 * parts to the filenames in the directory.  Just ring the bell and return.
 */

    if ((maxi == MatchStringLen) && (NumberOfMatches > 1))
    {
	ReleaseAList (FileList);
	return RingWarningBell ();
    }

/* So, at this point, we are completing and have something to append.  Check
 * that the line is not too long and if there is an end bit, we can save a
 * copy of it.
 */

/* Insert after spaces */

    if (InsertSpace && isspace (*InsertPosition) &&
	(InsertPosition != ConsoleLineBuffer))
    {
	++StartDirInCLB;
	++InsertPosition;
    }

    cp = null;

    if (((strlen (InsertPosition) + maxi +
         	(StartDirInCLB - ConsoleLineBuffer) + 3) >= LINE_MAX) ||
	(strlen (InsertPosition) &&
	 ((cp = StringCopy (InsertPosition)) == null)))
    {
	ReleaseAList (FileList);
	return RingWarningBell ();
    }

/* Append the new end of line bits */

    strcpy (StartDirInCLB, *FileList);
    strcpy (&StartDirInCLB[i = strlen (StartDirInCLB)], " ");

/* Save the current position */

    c_buffer_pos = &ConsoleLineBuffer[strlen (ConsoleLineBuffer)];

/* If we found only 1 and its a directory, append a d sep */

    if ((NumberOfMatches == 1) && IsDirectory (NameStart))
    {
	StartDirInCLB[i] = CHAR_UNIX_DIRECTORY;
	strcpy (c_buffer_pos, cp);
    }

/* If multiple matches, position at filename and not original position */

    else
    {
	if (isspace (*cp))
	    --c_buffer_pos;

	strcpy (c_buffer_pos, cp);

	if ((NumberOfMatches > 1) && !isspace (*cp))
	    --c_buffer_pos;
    }

/* Release the saved buffer and reset end of line pointer */

    if (cp != null)
	ReleaseMemoryCell ((void *)cp);

    ReleaseAList (FileList);
    EndOfCurrentLine = &ConsoleLineBuffer[strlen (ConsoleLineBuffer)];

/* Beep if more than one */

    if (NumberOfMatches > 1)
	RingWarningBell ();

    return TRUE;
}

/* Initialise input */

static void F_LOCAL InitialiseInput (bool im)
{
    c_buffer_pos = ConsoleLineBuffer;	/* Initialise			*/
    EndOfCurrentLine = ConsoleLineBuffer;
    SetCursorShape (InsertMode = im);
    M_length = -1;

/* Reset max line length */

    MaxLength = 0;
    OldMaxLength = 0;

/* Save the cursor position */

    if (IS_Console (1))
	StartCursorPosition = ReadCursorPosition ();
}

/* Configure Keyboard I/O */

void Configure_Keys (void)
{
    char		*cp;			/* Line pointers	*/
    int			i, fval, cval;
    int			nFields;
    LineFields		LF;
    long		value;

/* Get some memory for the input line and the file name */

    if ((LF.LineLength = strlen (cp = GetVariableAsString (ShellVariableName,
    						      FALSE)) + 5) < 200)
	LF.LineLength = 200;

    if ((LF.Line = AllocateMemoryCell (LF.LineLength)) == (char *)NULL)
	return;

    strcpy (LF.Line, cp);

/* Find the .exe in the name */

    if ((cp = FindLastPathCharacter (LF.Line)) != (char *)NULL)
	++cp;

    else
	cp = LF.Line;

    if ((cp = strrchr (cp, CHAR_PERIOD)) == (char *)NULL)
	cp = &LF.Line[strlen (LF.Line)];

    strcpy (cp, ".ini");

    if ((LF.FP = FOpenFile (CheckDOSFileName (LF.Line),
    			    sOpenReadMode)) == (FILE *)NULL)
    {
	ReleaseMemoryCell ((void *)LF.Line);
	return;
    }

/* Initialise the internal buffer */

    LF.Fields = (Word_B *)NULL;

/* Scan for the file */

    while ((nFields = ExtractFieldsFromLine (&LF)) != -1)
    {
        if (nFields < 2)
            continue;

/* Look up the keyword name */

	for (i = 0;
	     (i < KF_LENGTH) && (stricmp (LF.Fields->w_words[0],
				 KF_List[i].kf_name) != 0);
	     ++i)
	    continue;

/* Ignore no matches */

	if (i == KF_LENGTH)
	    continue;

/* Get the value of the second field - must be numeric */

	cval = 0;

	if (!ConvertNumericValue (LF.Fields->w_words[1], &value, 0))
	    continue;

	fval = (int)value;

/* Get the value of the third field, if it exists - must be numeric */

	if (nFields == 3)
	{
	    if (!ConvertNumericValue (LF.Fields->w_words[2], &value, 0))
		continue;

	    cval = (int)value;
	}

/* OK we have a valid value, save it */

	KF_List[i].akey = (char)fval;
	KF_List[i].fkey = (char)cval;
    }

    ReleaseMemoryCell ((void *)LF.Line);
}

/* Check cursor is in column zero */

#if (OS_TYPE == OS_OS2)
void PositionCursorInColumnZero (void)
{
    BYTE		abCell[2];
    USHORT		usRow;
    USHORT		usColumn;
    USHORT		cb = sizeof (abCell);

/* Get screen infor and cursor position */

    GetScreenParameters ();
    StartCursorPosition = ReadCursorPosition ();

    VioGetCurPos (&usRow, &usColumn, 0);
    VioReadCellStr (abCell, &cb , usRow, usColumn, 0);

    if ((StartCursorPosition % MaximumColumns) || (abCell[0] != CHAR_SPACE))
	fputchar (CHAR_NEW_LINE);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
void PositionCursorInColumnZero (void)
{
    char			abCell[2];
    CONSOLE_SCREEN_BUFFER_INFO	CSI;
    DWORD			ActionCount;

/* Get screen infor and cursor position */

    GetScreenParameters ();
    StartCursorPosition = ReadCursorPosition ();

    GetConsoleScreenBufferInfo (NT_StdOut, &CSI);
    ReadConsoleOutputCharacter (NT_StdOut, abCell, 1, CSI.dwCursorPosition,
    				&ActionCount);

    if ((StartCursorPosition % MaximumColumns) || (*abCell != CHAR_SPACE))
	fputchar (CHAR_NEW_LINE);
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS) 
void PositionCursorInColumnZero (void)
{
    union REGS		r;

/* Get screen infor and cursor position */

    GetScreenParameters ();
    StartCursorPosition = ReadCursorPosition ();

    r.h.ah = 0x08;
    r.h.bh = 0x00;
    SystemInterrupt (0x10, &r, &r);

    if ((StartCursorPosition % MaximumColumns) || (r.h.al != CHAR_SPACE))
	fputchar (CHAR_NEW_LINE);
}
#endif

/*
 * Get screen parameters
 */

#if (OS_TYPE == OS_OS2) 
void	GetScreenParameters (void)
{
    VIOMODEINFO		viomi;

    viomi.cb = sizeof(viomi);

    VioGetMode (&viomi, 0);
    MaximumColumns = viomi.col;
    MaximumLines = viomi.row;

/* Set up LINES and COLUMNS variables */

    SetVariableFromNumeric (LIT_LINES, (long)MaximumLines);
    SetVariableFromNumeric (LIT_COLUMNS, (long)MaximumColumns);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT) 
void	GetScreenParameters (void)
{
    CONSOLE_SCREEN_BUFFER_INFO	CSI;

    NT_StdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo (NT_StdOut, &CSI);

    MaximumColumns = CSI.dwMaximumWindowSize.X;
    MaximumLines = CSI.dwMaximumWindowSize.Y;

/* Set up LINES and COLUMNS variables */

    SetVariableFromNumeric (LIT_LINES, (long)MaximumLines);
    SetVariableFromNumeric (LIT_COLUMNS, (long)MaximumColumns);
}
#endif

/* DOS version */

#if (OS_TYPE == OS_DOS) 
void	GetScreenParameters (void)
{
    union REGS		r;

#  if (OS_SIZE == OS_16)
    MaximumColumns = *(int *)(0x0040004aL);
#  else
    MaximumColumns = *(short *)(0x044aL);
#  endif

    MaximumLines = 25;

/* Is this an EGA?  This test was found in NANSI.SYS */

    r.h.ah = 0x12;
    r.x.REG_BX = 0xff10;
    SystemInterrupt (0x10, &r, &r);

/* Else read the number of rows */

    if (!(r.x.REG_BX & 0xfefc))
    {
	r.x.REG_AX = 0x1130;
	r.h.bh = 0;
	SystemInterrupt (0x10, &r, &r);
	MaximumLines = r.h.dl + 1;
    }

/* Set up LINES and COLUMNS variables */

    SetVariableFromNumeric (LIT_LINES, (long)MaximumLines);
    SetVariableFromNumeric (LIT_COLUMNS, (long)MaximumColumns);
}
#endif

/* Ring Bell ? */

bool	RingWarningBell (void)
{
    if (KF_List[KF_RINGBELL].akey)
#if (OS_TYPE == OS_OS2)
	DosBeep (1380, 500);
#elif (OS_TYPE == OS_NT)
	Beep (1380, 500);
#else
	fputchar (0x07);
	fflush (stdout);
#endif

    return FALSE;
}

/*
 * Read keyboard function
 */

#if (OS_TYPE == OS_OS2)
unsigned char	ReadKeyBoard (unsigned char *f_key)
{
    KBDKEYINFO		kbci;

    *f_key = 0;

/* Wait for input */

    KbdCharIn (&kbci, IO_WAIT, 0);

    DisplayKeyCode (kbci.chChar, kbci.chScan);

    if (kbci.chChar == 0x03)
	raise (SIGINT);

/* Check for non-function character */

    if (kbci.chChar && (kbci.chChar != 0xe0))
	return (unsigned char)kbci.chChar;

/* Return scan code and type (normal or ALT'ed) */

    *f_key = (unsigned char)kbci.chScan;
    return (unsigned char)((kbci.fsState & (ALT | LEFTALT | RIGHTALT))
			   ? KT_ALTFUNCTION
			   : KT_FUNCTION);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
unsigned char	ReadKeyBoard (unsigned char *f_key)
{
    INPUT_RECORD	Buffer;
    DWORD		NumberOfEventsRead;
    int			i;

    *f_key = 0;

/* Wait for input */

    while (TRUE)
    {
	if (!ReadConsoleInput (NT_StdIn, &Buffer, 1, &NumberOfEventsRead))
	    continue;

	if (Buffer.EventType == WINDOW_BUFFER_SIZE_EVENT)
	    return KT_RESIZE;
	
/* Ignore other non-keyboard events */

	if (Buffer.EventType != KEY_EVENT)
	    continue;

/* Ignore key release events */

	if (!Buffer.Event.KeyEvent.bKeyDown)
	    continue;

/* See if it is an ignore key? */

	for (i = 0; i < IGNORE_CNT; i++)
	{
	    if (Buffer.Event.KeyEvent.wVirtualScanCode == IgnoreScanCode[i])
	        break;
	}

	if (i != IGNORE_CNT)
	    continue;

/* Keystroke mapping */

	if (DoNTKeyMap (&Buffer.Event.KeyEvent))
	    break;

/* ? wRepeatCount */

    }

    DisplayKeyCode (Buffer.Event.KeyEvent.uChar.AsciiChar,
		    Buffer.Event.KeyEvent.wVirtualScanCode);

    if (Buffer.Event.KeyEvent.uChar.AsciiChar == 0x03)
	raise (SIGINT);

/* Check for non-function character */

    if (Buffer.Event.KeyEvent.uChar.AsciiChar &&
	(Buffer.Event.KeyEvent.uChar.AsciiChar != 0xe0))
	return (unsigned char)Buffer.Event.KeyEvent.uChar.AsciiChar;

/* Return scan code and type (normal or ALT'ed) */

    *f_key = (unsigned char)Buffer.Event.KeyEvent.wVirtualScanCode;
    return (unsigned char)((Buffer.Event.KeyEvent.dwControlKeyState &
    			    (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
			   ? KT_ALTFUNCTION
			   : KT_FUNCTION);
}

/*
 * Handle keyboard mapping
 */

static bool F_LOCAL  DoNTKeyMap (KEY_EVENT_RECORD *KeyEvent)
{
    int		i;

    if (KeyEvent->wVirtualScanCode == 0x0f)	 /* TAB */
    {
	if (KeyEvent->dwControlKeyState & (CTRL_PRESSED | ALT_PRESSED))
	    return FALSE;
	
	else if (KeyEvent->dwControlKeyState & SHIFT_PRESSED)
	    KeyEvent->uChar.AsciiChar = 0;

	else
	    KeyEvent->uChar.AsciiChar = CHAR_TAB;
	    
	return TRUE;
    }

    else if ((KeyEvent->wVirtualScanCode == 0x0e) ||	/* BACKSPACE */
	     (KeyEvent->wVirtualScanCode == 0x1c)) 	/* RETURN */
	return !C2bool (KeyEvent->dwControlKeyState & ALT_PRESSED);

/* All other mapped characters are OK */

    else if (KeyEvent->uChar.AsciiChar)
        return TRUE;

/* Handle function keys */

    for (i = 0; i < MAX_NT_FKMAP; i++)
    {
        if (KeyEvent->wVirtualScanCode != NT_KFMap [i].Normal)
	    continue;

/* If Ctrl Alt pressed - ignore key */

	if ((KeyEvent->dwControlKeyState & (CTRL_PRESSED | ALT_PRESSED))
		== (CTRL_PRESSED | ALT_PRESSED))
	    return FALSE;

	else if (KeyEvent->dwControlKeyState & CTRL_PRESSED)
	    KeyEvent->wVirtualScanCode = NT_KFMap [i].Control;

	else if (KeyEvent->dwControlKeyState & ALT_PRESSED)
	    KeyEvent->wVirtualScanCode = NT_KFMap [i].Alt;

	else if (KeyEvent->dwControlKeyState & SHIFT_PRESSED)
	    KeyEvent->wVirtualScanCode = NT_KFMap [i].Shift;
	
	return TRUE;
    }

    return TRUE;
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS) 
unsigned char	ReadKeyBoard (unsigned char *f_key)
{
    unsigned char		a_key;
    union KeyCode {
	unsigned short		Key;
	struct Scan {
	    unsigned char	ascii, scan;
	}			Scan;
    }				KeyCode;

/* If DEQSview is active - we must poll the keyboard */

#  if (OS_SIZE == OS_16)
    if (DESQviewActive)
    {
	if (!((KeyCode.Scan.ascii = Poll_Keyboard ())) ||
	    (KeyCode.Scan.ascii == 0xe0))
	    KeyCode.Scan.scan = Poll_Keyboard ();
    }

    else
#  endif

/*
 * Read Keyboard via interrupt 10 function 1 or 10 depending on whether we
 * have an extended keyboard or not!.
 */

    {
	KeyCode.Key = _bios_keybrd (ReadKeyboardOption);

	if (KeyCode.Scan.ascii == 0xe0)
	    KeyCode.Scan.ascii = 0;
    }

    DisplayKeyCode (KeyCode.Scan.ascii, KeyCode.Scan.scan);

/* Check for interrupt */

    if (KeyCode.Scan.ascii == 3)
	raise (SIGINT);

/* Set up return values */

    if (KeyCode.Scan.ascii == 0)
    {
	a_key = KT_FUNCTION;
	*f_key = KeyCode.Scan.scan;

/* If ALT Key set, return 0xff instead of 0 for function key */

	if (_bios_keybrd (_KEYBRD_SHIFTSTATUS) & 0x08)
	    a_key = KT_ALTFUNCTION;
    }

/* ASCII keycode - return it. */

    else
    {
	a_key = KeyCode.Scan.ascii;
	*f_key = 0;
    }

    return a_key;
}
#endif

#ifdef KEYDEBUG
static void	DisplayKeyCode (unsigned int a, unsigned int b)
{
    int		cp = ReadCursorPosition ();

    SetCursorPosition (23*80 + 59);
    printf ("ASCII %.2x SCAN %.2x\n", a, b);
    SetCursorPosition (cp);
}
#endif

/*
 * Update Initialisation value
 */

bool	ChangeInitialisationValue (char *string, int newvalue)
{
    int		i;

    for (i = KF_END_FKEYS; i < KF_LENGTH; ++i)
    {
	if (stricmp (string, KF_List[i].kf_name) == 0)
	{
	    KF_List[i].akey = (char)newvalue;
	    return TRUE;
	}
    }

    return FALSE;
}

/*
 * Enable keyboard polling function - required if DESQview loaded
 */

#if (OS_TYPE == OS_DOS)
static void F_LOCAL CheckKeyboardPolling (void)
{
    static bool		Initialised = FALSE;
    union REGS		r;
    int			i;

    if (Initialised)
	return;
    
    Initialised = TRUE;

/* Check to see we are running under DESQview */

#  if (OS_SIZE == OS_16)
    r.x.REG_AX = 0x2b01;
    r.x.REG_CX = 0x4445;
    r.x.REG_DX = 0x5351;
    DosInterrupt (&r, &r);

    if (r.h.al != 0xff)
	DESQviewActive = TRUE;
#  endif

/* Otherwise check for extended keyboard.  Stick 0xffff in the keyboard
 * buffer and read it back.  If we find it - OK!
 */

    ReadKeyboardOption = _KEYBRD_READ;	/* Normal keyboard		*/

    r.h.ah = 0x05;
    r.x.REG_CX = 0xffff;
    SystemInterrupt (0x16, &r, &r);

    if (r.h.al)
	return;

/* 16 attempts to read extended keyboard interface */

    for (i = 0; i < 16; i++)
    {
	if ((unsigned)_bios_keybrd (_NKEYBRD_READ) == 0xffff)
	{
	    ReadKeyboardOption = _NKEYBRD_READ;	/* Extended keyboard	*/
	    return;
	}
    }
}
#endif

/*
 * Print a single history entry
 */

static void F_LOCAL PrintOutHistory (FILE		*fp,
				     bool		n_flag,
				     struct cmd_history *cmd)
{
    char	*cp = cmd->command;

    if (n_flag)
    {
	fprintf (fp, "%5d: ", cmd->number);

	while (*cp)
	{
	    putc (*cp, fp);

	    if (*(cp++) == CHAR_NEW_LINE)
		fputs ("       ", fp);
	}
    }

    else
	fputs (cp, fp);

    putc (CHAR_NEW_LINE, fp);
}

/*
 * Get the last history event number
 */

int GetLastHistoryEvent (void)
{
    return (l_history) ? cmd_history[l_history - 1].number + 1 : 1;
}

/*
 * Get the last history event number
 */

int GetFirstHistoryEvent (void)
{
    return (cmd_history[0].number) ? cmd_history[0].number : 1;
}

/*
 * Get the last history event
 */

#if (OS_TYPE != OS_DOS) 
char *GetLastHistoryString (void)
{
    return l_history ? cmd_history[l_history - 1].command : (char *)NULL;
}
#endif

/*
 * Dump history
 */

void PrintHistory (bool r_flag, bool n_flag, int First, int Last, FILE *fp)
{
    int		i;

    if (r_flag)
    {
	for (i = l_history - 1;
	     (i >= 0) && (cmd_history[i].number >= First); --i)
	{
	    if (cmd_history[i].number <= Last)
		PrintOutHistory (fp, n_flag, &cmd_history[i]);
	}
    }

    else
    {
	for (i = 0; (i < l_history) && (cmd_history[i].number <= Last); ++i)
	{
	    if (cmd_history[i].number >= First)
		PrintOutHistory (fp, n_flag, &cmd_history[i]);
	}
    }
}

/*
 * Search for an event
 */

int SearchHistory (char *buffer)
{
    int		Length = strlen (buffer);
    int		i;

    for (i = l_history - 1;
	 (i >= 0) && strncmp (buffer, cmd_history[i].command, Length); --i)
	continue;

    return (i == -1) ? -1 : cmd_history[i].number;
}

/*
 * Release Command
 */

static void F_LOCAL ReleaseCommandMemory (struct cmd_history *cp)
{
    if (cp->command != null)
	ReleaseMemoryCell ((void *)cp->command);
}

/*
 * Flush history buffer.  If save is set, the contents of the console
 * buffer will be saved.
 */

void	FlushHistoryBuffer (void)
{
    if (SaveHistory)
	AddHistory (AppendHistory);

    memset (ConsoleLineBuffer, 0, LINE_MAX + 1);

    AppendHistory = FALSE;
    SaveHistory = FALSE;
}

/*
 * Save the current console buffer
 */

static void F_LOCAL SaveCurrentHistory (void)
{
    c_history = l_history;

    if (SaveHistory)
	AddHistory (AppendHistory);

    AppendHistory = (bool)(LastUserPrompt == PS2);
    SaveHistory = (bool)((LastUserPrompt == PS1) || AppendHistory);
}

/*
 * Insert a Character into the buffer
 */

static bool F_LOCAL InsertACharacter (int NewCharacter)
{
    if ((EndOfCurrentLine - ConsoleLineBuffer) == LINE_MAX)
	return RingWarningBell ();	/* Ring bell - line full	*/

    if (c_buffer_pos != EndOfCurrentLine)
	memrcpy (EndOfCurrentLine + 1, EndOfCurrentLine,
		 EndOfCurrentLine - c_buffer_pos + 1);

    ++EndOfCurrentLine;

/* Fast end of line processing */

    if ((c_buffer_pos == EndOfCurrentLine - 1) && (NewCharacter != CHAR_TAB))
    {
	*(c_buffer_pos++) = (char)NewCharacter;
	OutputACharacter (NewCharacter);
	return FALSE;
    }

/* Not at end of line - redraw */

    *(c_buffer_pos++) = (char)NewCharacter;
    return TRUE;
}

/*
 * Delete Last History Item
 */

void DeleteLastHistory (void)
{
    if (l_history)
	ReleaseCommandMemory (&cmd_history[--l_history]);
}

/*
 * Clear the screen
 */

#if (OS_TYPE == OS_OS2)
bool	ClearScreen (void)
{
    BYTE	abCell[2];
    USHORT	usRow;
    USHORT	usColumn;
    USHORT	cb = sizeof (abCell);

    fputchar (CHAR_NEW_LINE);
    FlushStreams ();

/* Read attribute under cursor */

    VioGetCurPos (&usRow, &usColumn, 0);
    VioReadCellStr (abCell, &cb , usRow, usColumn, 0);

    abCell[0] = CHAR_SPACE;

    VioScrollUp (0, 0, 0xffff, 0xffff, 0xffff, abCell, 0);
    VioSetCurPos (0, 0, 0);

    OutputUserPrompt (LastUserPrompt);
    StartCursorPosition = ReadCursorPosition ();
    return TRUE;
}
#endif

/*
 * NT Version
 */

#if (OS_TYPE == OS_NT)
bool	ClearScreen (void)
{
    WORD			Atts[2];
    CONSOLE_SCREEN_BUFFER_INFO	CSI;
    DWORD			ActionCount;
    CHAR_INFO			*Buffer;
    COORD			dwBufferSize;
    COORD			dwBufferCoord;
    SMALL_RECT			WriteRegion;
    DWORD			Size = MaximumColumns * MaximumLines;

    FlushStreams ();

/* Read attribute under cursor */

    GetConsoleScreenBufferInfo (NT_StdOut, &CSI);
    ReadConsoleOutputAttribute (NT_StdOut, Atts, 1, CSI.dwCursorPosition,
    				&ActionCount);

/*
 * Write it.  What a pain!!!
 *
 * Get some memory
 */

    Buffer = (CHAR_INFO *)AllocateMemoryCell (Size * sizeof (CHAR_INFO));

/* Set up attributes */

    for (ActionCount = 0; ActionCount < Size; ActionCount++)
    {
	Buffer[ActionCount].Attributes = Atts[0];
	Buffer[ActionCount].Char.AsciiChar = CHAR_SPACE;
    }

/* Size */

    dwBufferSize.X = MaximumColumns;
    dwBufferSize.Y = MaximumLines;

/* Co-ordinated */

    dwBufferCoord.X = 0;
    dwBufferCoord.Y = 0;

/* Set up output data */

    WriteRegion.Left = 0;
    WriteRegion.Top = 0;
    WriteRegion.Right = MaximumColumns - 1;
    WriteRegion.Bottom = MaximumLines - 1;

    WriteConsoleOutput (NT_StdOut, Buffer, dwBufferSize, dwBufferCoord,
			&WriteRegion);

    ReleaseMemoryCell (Buffer);

    SetConsoleCursorPosition (NT_StdOut, dwBufferCoord);

    OutputUserPrompt (LastUserPrompt);
    StartCursorPosition = ReadCursorPosition ();
    return TRUE;
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS)
bool	ClearScreen (void)
{
    union REGS		r;
    unsigned char	backg;

    fputchar (CHAR_NEW_LINE);
    FlushStreams ();

/* Get the background attribute of the cursor */

    r.h.ah = 0x08;
    r.h.bh = 0;
    SystemInterrupt (0x10, &r, &r);
    backg = (unsigned char)(r.h.ah & 0x07);

/* Clear the screen */

    r.x.REG_AX = 0x0600;
    r.h.bh = backg;
    r.x.REG_CX = 0;
    r.h.dh = (unsigned char)MaximumLines;
    r.h.dl = (unsigned char)MaximumColumns;
    SystemInterrupt (0x10, &r, &r);

/* Position to top of page */

    r.h.ah = 0x02;				/* Set new position	*/
    r.h.bh = 0;					/* Page zero		*/
    r.x.REG_DX = 0x0000;
    SystemInterrupt (0x10, &r, &r);

    OutputUserPrompt (LastUserPrompt);
    StartCursorPosition = ReadCursorPosition ();
    return TRUE;
}
#endif

/*
 * Display a line, handling control characters
 */

void	DisplayLineWithControl (char *line)
{
    int		off = ReadCursorPosition ();

/* Print characters */

    while (*line)
    {

/* Process TABS */

	if (*line == CHAR_TAB)
	{
	    do
	    {
		fputchar (CHAR_SPACE);
	    } while ((++off) % 8);
	}

/* Process Control and printing characters */

	else
	    off += OutputACharacter (*line);

	++line;
    }
}

/*
 * Get the Root Disk Drive.  If not defined, set it to the current drive.
 */

#if (OS_TYPE != OS_UNIX)
int	GetRootDiskDrive (void)
{
    if (!KF_List[KF_ROOTDRIVE].akey)
	KF_List[KF_ROOTDRIVE].akey = (char)GetCurrentDrive ();

    return KF_List[KF_ROOTDRIVE].akey;
}
#endif

/*
 * Get the EOF Key
 */

int	GetEOFKey (void)
{
#if (OS_TYPE == OS_UNIX)
    struct termios	ts;

    return tcgetattr (1, &ts) ? 4 : ts.c_cc[VEOF];
#else
    if (!KF_List[KF_EOFKEY].akey)
	KF_List[KF_EOFKEY].akey = 0x1a;

    return KF_List[KF_EOFKEY].akey;
#endif
}

/*
 * Output a Character - excluding TABS.  Return # chars output.
 *
 * TABS are not checked for
 */

static int F_LOCAL OutputACharacter (int c)
{
    int		off = 1;

/* Check for control and process */

    if (iscntrl (c))
    {
	fputchar (CHAR_NOT);
	c += '@';
	off++;
    }

/* Output the character */

    fputchar (c);
    return off;
}

/*
 * Strip off trailing EOFs and EOLs from console buffer
 */

char	CleanUpBuffer (int length, char *buffer, int eofc)
{
    char	*cp = &buffer[length - 1];
    char	ret;

    while (length && ((*cp == (char)eofc) || (*cp == CHAR_NEW_LINE)))
    {
	length--;
	cp--;
    }

    ret = *(cp + 1);
    *(cp + 1) = 0;
    return ret;
}

/*
 * Look up the keystroke to see if it is one of our functions
 */

int	LookUpKeyBoardFunction (unsigned char a_key, unsigned char f_key)
{
    int		i;

    for (i = 0; (i < KF_END_FKEYS); ++i)
    {
	if (KF_List[i].akey != a_key)
	    continue;

/* Function or meta key? */

	if ((a_key != KT_FUNCTION) && (a_key != KT_ALTFUNCTION))
	    break;

	else if (KF_List[i].fkey == f_key)
	    break;
    }

/* If this is a function key and is not ours, ignore it */

    if ((i == KF_END_FKEYS) && ((a_key == KT_FUNCTION) ||
    				(a_key == KT_ALTFUNCTION)))
	return 0;

    return (i == KF_END_FKEYS) ? (int)a_key
    			       : -((int)(KF_List[i].fcode) + 1);
}

/*
 * Build a list of filenames for completion
 */

char 	**BuildCompletionList (char *String, size_t Length, int *Count,
			       bool Full)
{
    char	*MatchString = GetAllocatedSpace (Length + 2);
    char	**FileList;
    char	*vecp[2];
    char	*cp;
    char	**ap;
    size_t	DiscardLength;

/* Build the string to expand */

    *Count = 0;
    memmove (MatchString, String, Length);

    if (MatchString[Length - 1] != CHAR_MATCH_ALL)
    {
	MatchString[Length] = CHAR_MATCH_ALL;
	MatchString[Length + 1] = 0;
    }

    else
	MatchString[Length] = 0;

    vecp[0] = MatchString;
    vecp[1] = (char *)NULL;

/* Expand it */

    FileList = ExpandWordList (vecp, EXPAND_SPLITIFS | EXPAND_GLOBBING |
    				EXPAND_TILDE, (ExeMode *)NULL);

/* Check to see if it expanded */

    if ((strcmp (FileList[0], MatchString) == 0) &&
	(FileList[1] == (char *) NULL))
    {
	ReleaseMemoryCell (FileList[0]);
	ReleaseMemoryCell (FileList);
        FileList = (char **)NULL;
    }

    ReleaseMemoryCell (MatchString);

    if (FileList != (char **)NULL)
	*Count = CountNumberArguments (FileList);

/* Exit if expansion failed or we don't want directory stripping */

    if ((FileList == (char **)NULL) || Full)
	return FileList;

/* Strip off directory name ..../ or x: */

    if ((cp = FindLastPathCharacter (FileList[0])) != (char *)NULL)
        cp++;

    else if (IsDriveCharacter (FileList[0][1]))
        cp = &FileList[0][2];

    else
        return FileList;

/* Get the discard length and remove it from the strings */

    DiscardLength = cp - FileList[0];
    ap = FileList;

    while ((cp = *(ap++)) != (char *)NULL)
        strcpy (cp, cp + DiscardLength);

    return FileList;
}

/*
 * Get the Common part of the name from a list of files
 */

size_t	GetCommonPartOfFileList (char **FileList)
{
    char	**ap = FileList + 1;
    char	*BaseName = *FileList;
    size_t	maxi = strlen (*FileList);
    size_t	i;

/* Scan the list */

    while (*ap != (char *)NULL)
    {
	for (i = 0; (BaseName[i] == (*ap)[i]) && (i < maxi); i++)
	     continue;

	BaseName[maxi = i] = 0;
	ap++;
    }

    return maxi;
}

/*
 * Return the function key information associated with an internal key
 * function
 */

#if defined (FLAGS_EMACS) || defined (FLAGS_GMACS)
unsigned char	GetFunctionKeyMap (int key, unsigned char *f_key)
{
    *f_key = KF_List[key].fkey;
    return KF_List[key].akey;
}
#endif

/*
 * Change Keyboard to RAW/COOKED mode
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL	ChangeKeyboardMode (bool Cooked)
{
    KBDINFO		kbstInfo;

/* Get current mode */

    kbstInfo.cb = sizeof (kbstInfo);
    KbdGetStatus (&kbstInfo, 0);

/* Change it */

    kbstInfo.fsMask &= ~(KEYBOARD_ASCII_MODE | KEYBOARD_ECHO_ON |
		         KEYBOARD_ECHO_OFF   | KEYBOARD_BINARY_MODE);

    kbstInfo.fsMask |= (Cooked) ? (KEYBOARD_ASCII_MODE | KEYBOARD_ECHO_ON)
    				: (KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE);

    KbdSetStatus (&kbstInfo, 0);
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static void F_LOCAL	ChangeKeyboardMode (bool Cooked)
{
    DWORD		NewMode = ENABLE_WINDOW_INPUT;

/* Get handlers */

    NT_StdIn  = GetStdHandle (STD_INPUT_HANDLE);
    NT_StdOut = GetStdHandle (STD_OUTPUT_HANDLE);

/* Set up cooked mode if required */

    if (Cooked)
	NewMode = (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
		   ENABLE_ECHO_INPUT);

/* Change modes */

    SetConsoleMode (NT_StdIn, NewMode);
    SetConsoleMode (NT_StdOut, ENABLE_PROCESSED_OUTPUT |
			       ENABLE_WRAP_AT_EOL_OUTPUT);
}
#endif

/* UNIX Version */

#if (OS_TYPE == OS_UNIX)
static void F_LOCAL	ChangeKeyboardMode (bool Cooked)
{
    fputs ("UNIX: ChangeKeyboardMode NI\n", stderr);
}
#endif

/*
 * EMX needs a read keyboard function
 */

#if (OS_TYPE == OS_DOS) && defined (__EMX__)
unsigned int	 _bios_keybrd (unsigned int function)
{
    union REGS		r;

    r.h.ah = function;
    SystemInterrupt (0x16, &r, &r);
    return r.x.REG_AX;
}
#endif
