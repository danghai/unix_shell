/*
 * MS-DOS SHELL - 'word' Interpretator
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Charles Forsyth
 *
 * This code is based on (in part) the shell program written by Charles
 * Forsyth and the subsequence modifications made by Simon J. Gerraty (for
 * his Public Domain Korn Shell) and is subject to the following copyright
 * restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh4.c,v 2.13 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh4.c,v $
 *	Revision 2.13  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.12  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.11  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>			/* String library functions     */
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include "sh.h"

/*
 * string expansion
 *
 * first pass: quoting, IFS separation, ${} and $() substitution.
 * second pass: filename expansion (*?[]~).
 */

/*
 * expansion generator state
 */

typedef struct Expand {
    /* int  type; */			/* see ExpandAWord()			*/
    char	*str;			/* string			*/
    union {
	char	**strv;			/* string[]			*/
	FILE	*file;			/* file				*/
    }		u;			/* source			*/

    bool	split;			/* split "$@"			*/
} Expand;

#define	XBASE	0		/* scanning original			*/
#define	XSUB	1		/* expanding ${} string			*/
#define	XARGSEP	2		/* ifs0 between "$@"			*/
#define	XARG	3		/* expanding $*, $@			*/
#define	XCOM	4		/* expanding $()			*/

/*
 * Quote processing
 */

#define QUOTE_NONE	0	/* None					*/
#define QUOTE_INSIDE	1	/* Inside quotes			*/
#define QUOTE_TEMP	2	/* Mark a temporary quote		*/

/*
 * for nested substitution: ${var:=$var2}
 */

typedef struct SubType {
    short	type;		/* [=+-?%#] action after expanded word	*/
    short	base;		/* begin position of expanded word	*/
    char	*name;		/* name for ${var=word}			*/
    int		index;		/* index ${var[index]=word}		*/
} SubType;

static void F_LOCAL	ExpandAWord (char *, Word_B **, int);
static int F_LOCAL	VariableSubstitute (Expand *, char *, int, int);
static int F_LOCAL	CommandSubstitute (Expand *, char *);
static char * F_LOCAL	TrimSubstitute (SubType *, char *);
static int F_LOCAL	MathsSubstitute (Expand *, char *);
static void F_LOCAL	ExpandGlobCharacters (char *, Word_B **);
static void F_LOCAL	GlobAWord (char *, char *, char *, Word_B **, bool);
static char * F_LOCAL	RemoveMagicMarkers (unsigned char *);
static char * F_LOCAL	TildeSubstitution (unsigned char *);
static unsigned char * F_LOCAL	CheckForMultipleDrives (unsigned char *);
static bool F_LOCAL	ProcessCommandTree (C_Op *, int);
static char * F_LOCAL	WordScan (char *, int);
static int F_LOCAL	HandleArrayValue (char *, char **, int);
static void F_LOCAL	CheckForUnset (char *, int);
static void F_LOCAL	AlternationExpand (char *, Word_B **, int);
static int F_LOCAL	AlternationScan (char **, char **, char, int);
static void		BuildVariableEntryList (const void *, VISIT, int);
static int F_LOCAL 	GetNumberofFloppyDrives (void);

static char		*PNullNSet = "%s: parameter null or not set";
static char		*GVAV_Name;		/* Name for building a	*/
						/* list of Variable	*/
						/* Array values		*/
static Word_B		*GVAV_WordList;		/* Word block for list	*/


/*
 * compile and expand word
 */

char	*substitute (char *cp, int ExpandMode)
{
    struct source	*sold = source;
    char		*res;

    source = pushs (SWSTR);
    source->str = (char *) cp;

    if (ScanNextToken (ONEWORD) != PARSE_WORD)
	PrintErrorMessage ("eval: substitute error");

    res = ExpandAString (yylval.cp, ExpandMode);
    source = sold;
    return res;
}

/*
 * expand arg-list
 */

char	**ExpandWordList (char **ap, int ExpandMode, ExeMode *PMode)
{
    Word_B	*w = (Word_B *)NULL;
    bool	FoundProgram = FALSE;
    int		i;
    int		InitialCount;
    char	*CurrentAp;

    if ((ap == NOWORDS) || (*ap == NOWORD))
	return ap;

/* Expand the arguments */

    while (*ap != NOWORD)
    {
	InitialCount = WordBlockSize (w);
	ExpandAWord (CurrentAp = *(ap++), &w, ExpandMode);

/*
 * Get the program mode for expansion of words and globs.  Update the
 * current mode to reflect it.
 */

	if ((!FoundProgram) && (PMode != (ExeMode *)NULL) && WordBlockSize (w))
	{
	    CheckProgramMode (w->w_words[0], PMode);

	    if (PMode->Flags & EP_NOEXPAND)
		ExpandMode &= ~EXPAND_GLOBBING;

	    if (PMode->Flags & EP_NOWORDS)
		ExpandMode &= ~EXPAND_SPLITIFS;

	    if (PMode->Flags & EP_CONVERT)
		ExpandMode |= EXPAND_CONVERT;
	}

/* Convert UNIX directories to DOS and - to / ?, except for first
 * argument
 */

	if (ExpandMode & EXPAND_CONVERT)
	{
	    for (i = max (InitialCount, 1); i < WordBlockSize (w); i++)
	    {
		PATH_TO_DOS (w->w_words[i]);

/* Convert - to /, if the string is not quoted */

		if ((*(w->w_words[i]) == CHAR_SWITCH) &&
		    ((*CurrentAp != WORD_OQUOTE) ||
		     (*CurrentAp != WORD_QTCHAR)))
		    *(w->w_words[i]) = '/';
	    }
	}
    }

/* Return the word list */

    return GetWordList (AddWordToBlock (NOWORD, w));
}

/*
 * expand string
 */

char	*ExpandAString (char *cp, int ExpandMode)
{
    Word_B		*w = (Word_B *)NULL;

    ExpandAWord (cp, &w, ExpandMode);

    return (WordBlockSize (w) == 0) ? null : w->w_words[0];
}

/*
 * expand string - return only one component
 * used from iosetup to expand redirection files
 */

char	*ExpandOneStringFirstComponent (char *cp, int ExpandMode)
{
    Word_B		*w = (Word_B *)NULL;

    ExpandAWord (cp, &w, ExpandMode);

    switch (WordBlockSize (w))
    {
	case 0:
	    return null;

	case 1:
	    return w->w_words[0];
    }

    return ExpandAString (cp, ExpandMode & ~EXPAND_GLOBBING);
}

/*
 * Expand a word or two!
 */

static void F_LOCAL ExpandAWord (char   *OriginalWord,	/* input word	*/
				 Word_B **WordList,  /* output word list*/
				 int    ExpandMode)  /* Expand Flags	*/
{
    int			c;
    int			type = XBASE;	/* expansion type		*/
    int			QuoteStatus = 0;	/* quoted		*/
    int			quotestack[11];	/* Keep this bigger than the	*/
					/* subtype stack		*/
    int			*qst = quotestack + 11;
    XString		ds;		/* Expandable destination string*/
    unsigned char	*dp;		/* Pointer into Destination S.	*/
    char		*sp;		/* source			*/
    int			fdo;		/* second pass flags; have word */
    int			word;
    int			combase;
    int			ArrayIndex;
    Expand		x;		/* expansion variables */
    SubType		subtype [10];	/* substitution type stack */
    SubType		*st = subtype + 10;
    int			newlines;	/* For trailing newlines in COMSUB */
    int			trimming = 0;	/* flag if expanding ${var#pat}	*/
					/* or ${var%pat}		*/
    char		ifs0 = *GetVariableAsString (IFS, FALSE);

    if (OriginalWord == NULL)
	PrintErrorMessage ("eval: expanding a NULL");

    if (FL_TEST (FLAG_DISABLE_GLOB))
	ExpandMode &= (~EXPAND_GLOBBING);

/*
 * Look for '{' in the input word
 */

    if ((ShellGlobalFlags & FLAGS_ALTERNATION) &&
	(!(ExpandMode & EXPAND_NOALTS)) &&
	(ExpandMode & EXPAND_GLOBBING) &&
	((sp = strchr (OriginalWord, CHAR_OPEN_BRACES)) != (char *)NULL) &&
	(sp[-1] == WORD_CHAR) &&
	(!(sp[1] == WORD_CHAR && sp[2] == CHAR_CLOSE_BRACES)))
    {
	AlternationExpand (OriginalWord, WordList, ExpandMode);
	return;
    }

    ExpandMode &= ~EXPAND_NOALTS;

/* Initialise */

    dp = (unsigned char *)XCreate (&ds, 128);	/* destination string	 */
    type = XBASE;
    sp = OriginalWord;
    fdo = 0;
    word = !(ExpandMode & EXPAND_SPLITIFS);

/* The Main loop!! */

    while (1)
    {
	XCheck (&ds, (unsigned char **)(&dp));

	switch (type)
	{
	    case XBASE:			/* original prefixed string	*/

		switch ((c = *(sp++)))
		{
		    case WORD_EOS:		/* End - Hurray		*/
			c = 0;
			break;

		    case WORD_CHAR:		/* Normal char		*/
			c = *(sp++);
			break;

		    case WORD_QCHAR:		/* Escaped char		*/
		    case WORD_QTCHAR:
			QuoteStatus |= QUOTE_TEMP;/* temporary quote	*/
			c = *(sp++);
			break;

		    case WORD_OQUOTE:		/* Start quoted		*/
		    case WORD_ODQUOTE:
			word = 1;
			QuoteStatus = QUOTE_INSIDE;
			continue;

		    case WORD_CQUOTE:		/* End quoted		*/
		    case WORD_CDQUOTE:
			QuoteStatus = QUOTE_NONE;
			continue;

		    case WORD_COMSUB:		/* $(....)		*/
			type = CommandSubstitute (&x, sp);
			sp = strchr (sp, 0) + 1;
			combase = XCurrentOffset (ds, dp);
			newlines = 0;
			continue;

		    case WORD_OMATHS:		/* $((....))		*/
			type = MathsSubstitute (&x, sp);
			sp = strchr (sp, 0) + 1;
			continue;

		    case WORD_OSUBST:	/* ${var{:}[=+-?]word}		*/
			OriginalWord = sp; 		/* variable	*/
			sp = strchr (sp, 0) + 1;	/* skip variable */

/* Check for Array Variable */

			ArrayIndex = 0;
			if (*sp == WORD_OARRAY)
			    ArrayIndex = HandleArrayValue (OriginalWord, &sp,
							   ExpandMode);

			c = (*sp == WORD_CSUBST) ? 0 : *(sp++);

/* Check for match option */

			if (((c & 0x7f) == CHAR_MATCH_START) ||
			    ((c & 0x7f) == CHAR_MATCH_END))
			{
			    CheckForUnset (OriginalWord, ArrayIndex);
			    trimming++;
			    type = XBASE;
			    *--qst = QuoteStatus;
			    QuoteStatus = QUOTE_NONE;
			}

			else
			    type = VariableSubstitute (&x, OriginalWord,
						       c, ArrayIndex);

/* expand? */

			if (type == XBASE)
			{
			    if (st == subtype)
				ShellErrorMessage ("ridiculous ${} nesting");

			    --st;
			    st->type = c;
			    st->base = XCurrentOffset (ds, dp);
			    st->name = OriginalWord;
			    st->index = ArrayIndex;
			}

			else
			    sp = WordScan (sp, WORD_CSUBST); /* skip word */

			continue;

		    case WORD_CSUBST: /* only get here if expanding word */
			*dp = 0;

			if (ExpandMode & EXPAND_GLOBBING)
			    ExpandMode &= (~EXPAND_PATTERN);

/*
 *  Check that full functionality is here!
 */
			switch (st->type & 0x7f)
			{
			    case CHAR_MATCH_START:
			    case CHAR_MATCH_END:
				*dp = 0;
				dp = XResetOffset (ds, st->base);

				QuoteStatus = *(qst++);
				x.str = TrimSubstitute (st, (char *)dp);
				type = XSUB;
				trimming--;
				continue;

			    case CHAR_ASSIGN:
				SetVariableArrayFromString
					(st->name, st->index,
					 (char *)XResetOffset (ds, st->base));
				break;

			    case '?':
				if (dp == XResetOffset (ds, st->base))
				    ShellErrorMessage (PNullNSet, OriginalWord);

				else
				    ShellErrorMessage ("%s",
						       XResetOffset (ds,
								     st->base));
			}

			st++;
			type = XBASE;
			continue;
		}

		break;

	    case XSUB:
		if ((c = *(x.str++)) == 0)
		{
		    type = XBASE;
		    continue;
		}

		break;

	    case XARGSEP:
		type = XARG;
		QuoteStatus = QUOTE_INSIDE;

	    case XARG:
		if ((c = *(x.str++)) == 0)
		{
		    if ((x.str = *(x.u.strv++)) == NULL)
		    {
			type = XBASE;
			continue;
		    }

		    else if (QuoteStatus && x.split)
		    {
			type = XARGSEP;		/* terminate word for "$@" */
			QuoteStatus = QUOTE_NONE;
		    }

		    c = ifs0;
		}

		break;

	    case XCOM:
		if (newlines)			/* Spit out saved nl's	*/
		{
		    c = CHAR_NEW_LINE;
		    --newlines;
		}

		else
		{
		    while ((c = getc (x.u.file)) == CHAR_NEW_LINE)
			newlines++;		/* Save newlines	*/

		    if (newlines && (c != EOF))
		    {
			ungetc (c, x.u.file);
			c = CHAR_NEW_LINE;
			--newlines;
		    }
		}

		if (c == EOF)
		{
		    OriginalWord = (char *)XResetOffset (ds, combase);
		    newlines = 0;
		    S_fclose (x.u.file, TRUE);
		    type = XBASE;
		    continue;
		}

		break;
	}

/* check for end of word or IFS separation */

	if ((c == 0) || (!QuoteStatus && (ExpandMode & EXPAND_SPLITIFS) &&
			 IS_IFS (c)))
	{
	    if (word)
	    {
		*(dp++) = 0;
		OriginalWord = XClose (&ds, dp);

		if (fdo & EXPAND_TILDE)
		    OriginalWord =
			TildeSubstitution ((unsigned char *)OriginalWord);

		if (fdo & EXPAND_GLOBBING)
		    ExpandGlobCharacters (OriginalWord, WordList);

		else
		    *WordList = AddWordToBlock (OriginalWord, *WordList);

/* Re-set */
		fdo = 0;
		word = 0;

		if (c != 0)
		    dp = (unsigned char *)XCreate (&ds, 128);
	    }

	    if (c == 0)
		return;
	}

/*
 * Mark any special second pass chars
 */
	else
	{
	    if (!QuoteStatus)
	    {
		switch (c)
		{
		    case CHAR_MATCH_ALL:
		    case CHAR_MATCH_ANY:
		    case CHAR_OPEN_BRACKETS:
			if ((ExpandMode & (EXPAND_PATTERN | EXPAND_GLOBBING)) ||
			    trimming)
			{
			    fdo |= (ExpandMode & EXPAND_GLOBBING);
			    *dp++ = CHAR_MAGIC;
			}

			break;

/*
 * Check for [^...
 */

		    case CHAR_NOT:
			if (((ExpandMode & (EXPAND_PATTERN | EXPAND_GLOBBING))
			    || trimming) &&
			    ((dp[-1] == CHAR_OPEN_BRACKETS) &&
			     (dp[-2] == CHAR_MAGIC)))
			    *dp++ = CHAR_MAGIC;
			break;

		    case CHAR_TILDE:
			if (((ExpandMode & EXPAND_TILDE) &&
			     (dp == XStart (ds))) ||
			    (!(ExpandMode & EXPAND_SPLITIFS) &&
			     (dp[-1] == '=' || dp[-1] == ':')))
			{
			    fdo |= EXPAND_TILDE;
			    *dp++ = CHAR_MAGIC;
			}

			break;
		}
	    }

	    else
		QuoteStatus &= ~QUOTE_TEMP;	/* undo temporary	*/

	    word = 1;
	    *dp++ = (char)c;		/* save output char	*/
	}
    }
}

/*
 * Prepare to generate the string returned by ${} substitution.
 */

static int F_LOCAL VariableSubstitute (Expand	*xp,
				       char	*name,
				       int	stype,
				       int	Index)
{
    int		c;
    int		type;

/* Handle ${#*|@}
 *        ${#name[*]}
 *        ${#name[value]}
 *
 * String length or argc
 */

    if ((*name == '#') && ((c = name[1]) != 0))
    {
	if ((c == '*') || (c == '@'))
	    c = ParameterCount - 1;

	else if (Index < 0)
	    c = CountVariableArraySize (name + 1);

	else
	    c = strlen (GetVariableArrayAsString (name + 1, Index, FALSE));

	xp->str = StringCopy (IntegerToString (c));
	return XSUB;
    }

    c = *name;

/* Handle ${*|@}
 *        ${*|@[*|@]}
 *
 * Use Parameter list
 */

    if (c == '*' || c == '@')
    {
	if (ParameterCount == 0)
	{
	    xp->str = null;
	    type = XSUB;
	}

	else
	{
	    xp->u.strv = ParameterArray + 1 + ((Index >= 0) ? Index : 0);
	    xp->str = *(xp->u.strv++);
	    xp->split = C2bool (c == '@');		/* $@ */
	    type = XARG;
	}
    }

/* ${name[*|@]} */

    else if (Index < 0)
    {

/* Build list of values */

	if (isdigit (*name))
	{
	    for (c = 0; isdigit (*name) && (c < 1000); name++)
		c = c * 10 + *name - '0';

	    xp->u.strv = (c <= ParameterCount) ? ParameterArray + c
					       : NOWORDS;
	}

	else
	{
	    GVAV_Name = name;
	    GVAV_WordList = (Word_B *)NULL;
	    twalk (VariableTree, BuildVariableEntryList);
	    xp->u.strv = WordBlockSize (GVAV_WordList)
				? GetWordList (AddWordToBlock (NOWORD,
							       GVAV_WordList))
				: NOWORDS;
	}

/* Set up list.  Check to see if there any any entries */

	if (xp->u.strv == NOWORDS)
	{
	    xp->str = null;
	    type = XSUB;
	}

	else
	{
	    xp->str = *(xp->u.strv++);
	    xp->split = C2bool (Index == -2); 		/* ${name[@]} */
	    type = XARG;
	}
    }

/* ${name[num]} */

    else
    {
	xp->str = GetVariableArrayAsString (name, Index, TRUE);
	type = XSUB;
    }

    c = stype & 0x7F;

/* test the compiler's code generator */

    if ((c == CHAR_MATCH_END) || (c == CHAR_MATCH_START) ||
	(((stype & CHAR_MAGIC) ? (*xp->str == 0)
			       : (xp->str == null))
		? (c == '=') || (c == '-') || (c == '?')
		: (c == '+')))
	type = XBASE;	/* expand word instead of variable value */

/* Check for unset value */

    if ((type != XBASE) && FL_TEST (FLAG_UNSET_ERROR) &&
    	(xp->str == null) && (c != '+'))
	ShellErrorMessage ("unset variable %s", name);

    return type;
}

/*
 * Run the command in $(...) and read its output.
 */

static int F_LOCAL CommandSubstitute (Expand *xp, char *cp)
{
    Source	*s;
    C_Op	*t;
    FILE	*fi;
    jmp_buf	ReturnPoint;
    int		localpipe;

    if ((localpipe = OpenAPipe ()) < 0)
	return XBASE;

/* Create a new environment */

    CreateNewEnvironment ();
    MemoryAreaLevel++;

    if (SetErrorPoint (ReturnPoint))
    {
        QuitCurrentEnvironment ();
        ReleaseMemoryArea (MemoryAreaLevel--);	/* free old space */
	ClearExtendedLineFile ();
	S_close (localpipe, TRUE);
	return XBASE;
    }

/* Create line buffer */

    e.line = GetAllocatedSpace (LINE_MAX);

/* Parse the command */

    s = pushs (SSTRING);
    s->str = cp;

/* Check for $(<file) */

    if (((t = BuildParseTree (s)) != (C_Op *)NULL) &&
	(t->type == TCOM)     && (*t->args == NOWORD) &&
	(*t->vars == NOWORD)  && (t->ioact != (IO_Actions **)NULL))
    {
	IO_Actions	*io = *t->ioact;
	char		*name;

/* We don't need the pipe - so get rid of it */

	S_close (localpipe, TRUE);

/* OK - terminate the created environment */

	QuitCurrentEnvironment ();

	if ((io->io_flag & IOTYPE) != IOREAD)
	    ShellErrorMessage ("funny $() command");

	if ((localpipe = S_open (FALSE, name = ExpandAString (io->io_name,
							      EXPAND_TILDE),
				 O_RMASK)) < 0)
	    ShellErrorMessage ("cannot open %s", name);
    }

/* Execute the command */

    else
    {
	if (!ProcessCommandTree (t, localpipe))
	    longjmp (ReturnPoint, 1);

	QuitCurrentEnvironment ();
    }

/* Open the IO Stream */

    if ((fi = ReOpenFile (ReMapIOHandler (localpipe),
    			  sOpenReadMode)) == (FILE *)NULL)
	ShellErrorMessage ("cannot open $() input");

/*
 * Free old memory area
 */

    ReleaseMemoryArea (MemoryAreaLevel--);
    xp->u.file = fi;
    return XCOM;
}

/*
 * perform #pattern and %pattern substitution in ${}
 */

static char * F_LOCAL TrimSubstitute (SubType *st, char *pat)
{
    int			mode = GM_SHORTEST;
    char		*pos;
    char		*tsp;
    char		*str = GetVariableArrayAsString (st->name, st->index,
							 TRUE);

/*
 * Switch on the match type
 */

    switch (st->type & 0xff)
    {
	case CHAR_MATCH_START | CHAR_MAGIC:/* longest match at begin	*/
	    mode = GM_LONGEST;

	case CHAR_MATCH_START:		/* shortest at begin		*/
	    if (GeneralPatternMatch (str, (unsigned char *)pat, FALSE, &pos, mode))
		return pos;

	    break;

	case CHAR_MATCH_END | CHAR_MAGIC:/* longest match at end	*/
	    mode = GM_LONGEST;

	case CHAR_MATCH_END:		/* shortest match at end	*/
	    if (SuffixPatternMatch (str, pat, &pos, mode))
	    {
		tsp = StringCopy (str);
		tsp[pos - str] = 0;
		return tsp;
	    }

	    break;

    }

    return str;		/* no match, return string */
}

/*
 * glob
 * Name derived from V6's /etc/glob, the program that expanded filenames.
 */

static void F_LOCAL ExpandGlobCharacters (char *Pattern, Word_B **WordList)
{
    char		path [FFNAME_MAX];
    int			oldsize = WordBlockSize (*WordList);
    int			newsize;

#if (OS_TYPE != OS_UNIX)
    char		*NewPattern;		/* Search file name	*/
    int			CurrentDrive;		/* Current drive	*/
    int			MaxDrives;		/* Max drive		*/
    int			SelectedDrive;		/* Selected drive	*/
    int			y_drive;		/* Dummies		*/
    unsigned char	*DriveCharacter;	/* Multi-drive flag	*/
    char		SDriveString[2];
    char		*EndPattern;

/* Search all drives ? */

    if ((DriveCharacter = CheckForMultipleDrives (Pattern))
    			!= (unsigned char *)NULL)
    {
	CurrentDrive = GetCurrentDrive ();
	MaxDrives = SetCurrentDrive (CurrentDrive);
	SDriveString[1] = 0;
	EndPattern = WordScan (Pattern, 0);
	NewPattern = GetAllocatedSpace ((EndPattern - Pattern) + 1);

/* Scan the available drives */

	for (SelectedDrive = 1; SelectedDrive <= MaxDrives; ++SelectedDrive)
	{
	    if (SetCurrentDrive (SelectedDrive) != -1)
	    {
		y_drive = GetCurrentDrive ();
		SetCurrentDrive (CurrentDrive);
	    }

	    else
	        y_drive = -1;

/* Check to see if the second diskette drive is really there */

	    if ((GetNumberofFloppyDrives () < 2) && (SelectedDrive == 2))
		continue;

/* If the drive exists and is in our list - process it */

	    *DriveCharacter = 0;
	    *SDriveString = GetDriveLetter (SelectedDrive);
	    strlwr (Pattern);

	    if ((y_drive == SelectedDrive) &&
		GeneralPatternMatch (SDriveString, Pattern, TRUE, (char **)NULL,
				     GM_ALL))
	    {
		*DriveCharacter = CHAR_DRIVE;
		*NewPattern = *SDriveString;
		memcpy (NewPattern + 1, DriveCharacter,
			((unsigned char *)EndPattern - DriveCharacter) + 1);

		GlobAWord (path, path, NewPattern, WordList, FALSE);
	    }

	    *DriveCharacter = CHAR_DRIVE;
	}

	ReleaseMemoryCell (NewPattern);
    }

/*
 * No special processing for drives
 */

    else
	GlobAWord (path, path, Pattern, WordList, FALSE);
#else

/* UNIX has not drives. Goodie! */

    GlobAWord (path, path, Pattern, WordList, FALSE);
#endif

/*
 * Sort or something
 */

    if ((newsize = WordBlockSize (*WordList)) == oldsize)
	*WordList = AddWordToBlock (RemoveMagicMarkers ((unsigned char *)Pattern),
				    *WordList);

    else
	qsort (&(*WordList)->w_words[oldsize], (size_t)(newsize - oldsize),
	       sizeof (char *), SortCompare);
}

/*
 * Recursive bit
 */

static void F_LOCAL GlobAWord (char   *ds,	/* dest path		*/
			       char   *dp,	/* dest end		*/
      			       char   *sp,	/* source path		*/
			       Word_B **WordList,/* output list		*/
			       bool   check)	/* check dest existence */
{
    char		*EndFileName;		/* next source component */
    char		EndChar;
    char		*CFileName;
    char		*tdp;
    DIR			*dirp;
    struct dirent	*d;
    bool		IgnoreCase = TRUE;

/* End of source path ? */

    if (sp == (char *)NULL)
    {
	if (check && (!S_access (ds, F_OK)))
	    return;

	*WordList = AddWordToBlock (StringCopy (ds), *WordList);
	return;
    }

    if ((dp > ds) && (!IsDriveCharacter (*(dp - 1))))
	*dp++ = CHAR_UNIX_DIRECTORY;

    while (IsPathCharacter (*sp))
	*(dp++) = *(sp++);

/* Find end of current file name */

    if ((EndFileName = FindPathCharacter (sp)) != (char *)NULL)
    {
	*(EndFileName++) = 0;
	EndChar = CHAR_UNIX_DIRECTORY;
    }

#if (OS_TYPE != OS_UNIX)
    if ((tdp = strchr (sp, CHAR_DRIVE)) != (char *)NULL)
    {
	if (EndFileName != (char *)NULL)
	    *(--EndFileName) = CHAR_UNIX_DIRECTORY;

	EndFileName = tdp;
	*(EndFileName++) = 0;
	EndChar = CHAR_DRIVE;
    }
#endif

    *dp = 0;

 /* contains no pattern? */

    if (strchr (sp, CHAR_MAGIC) == NULL)
    {
	tdp = dp;
	CFileName = sp;

	while ((*(tdp++) = *(CFileName++)) != 0)
	    continue;

	if (IsDriveCharacter (EndChar))
	{
	    *(tdp - 1) = CHAR_DRIVE;
	    *tdp = 0;
	}

	else
	    --tdp;

	GlobAWord (ds, tdp, EndFileName, WordList, check);
    }

    else
    {

/* Check for drive letter and append a . to get the current directory */

	if ((strlen (ds) == 2) && IsDriveCharacter (*(ds + 1)))
	{
	    *(ds + 2) = CHAR_PERIOD;
	    *(ds + 3) = 0;
	}

/* Scan the directory */

	if ((dirp = opendir ((*ds == 0) ? CurrentDirLiteral
					: ds)) != (DIR *)NULL)
	{
	    if ((IsHPFSFileSystem ((*ds == 0) ? CurrentDirLiteral : ds)) &&
		(!(ShellGlobalFlags & FLAGS_NOCASE)))
		IgnoreCase = FALSE;

	    while ((d = readdir (dirp)) != (struct dirent *)NULL)
	    {
		CFileName = d->d_name;

/*
 * Ignore . * ..
 */

		if ((*CFileName == CHAR_PERIOD) &&
		    ((*(CFileName + 1) == 0) ||
		     ((*(CFileName + 1) == CHAR_PERIOD) &&
		      (*(CFileName + 2) == 0))))
		    continue;

/*
 * Ignore . files unless match starts with a dot.
 */

		if ((*CFileName == CHAR_PERIOD && *sp != CHAR_PERIOD) ||
		    !GeneralPatternMatch (CFileName, (unsigned char *)sp,
					  IgnoreCase, (char **)NULL, GM_ALL))
		    continue;

		tdp = dp;
		while ((*tdp++ = *CFileName++) != 0)
		    continue;

		--tdp;

		GlobAWord (ds, tdp, EndFileName, WordList,
			   C2bool (EndFileName != NULL));
	    }

	    closedir (dirp);
	}
    }

    if (EndFileName != NULL)
	*(--EndFileName) = EndChar;
}

/*
 * remove MAGIC from string
 */

static char * F_LOCAL RemoveMagicMarkers (unsigned char *Word)
{
    unsigned char	*dp, *sp;

    for (dp = sp = Word; *sp != 0; sp++)
    {
	if (*sp != CHAR_MAGIC)
	    *dp++ = *sp;
    }

    *dp = 0;
    return (char *)Word;
}

/*
 * tilde expansion
 *
 * based on a version by Arnold Robbins
 *
 * Think this needs Expanable strings!!
 */

static char * F_LOCAL TildeSubstitution (unsigned char *acp)
{
    unsigned		c;
    unsigned char	path[FFNAME_MAX];
    unsigned char	*cp = acp;
    unsigned char	*wp = path;
    unsigned char	*dp;

    while (TRUE)
    {
	while (TRUE)
	{
	    if ((c = *cp++) == 0)
	    {
		*wp = 0;
		ReleaseMemoryCell ((void *)acp);
		return StringCopy ((char *)path);
	    }

	    else if ((c == CHAR_MAGIC) && (*cp == CHAR_TILDE))
		break;

	    else
		*wp++ = (char)c;
	}

	dp = NULL;	/* no output substitution */

 /*
  * ~ or ~/
  */

	if ((cp[1] == 0) || IsPathCharacter (cp[1]) || (IsDriveCharacter (cp[1])))
	{
	    dp = (unsigned char *)GetVariableAsString (HomeVariableName, FALSE);
	    cp += 1;
	}

	else if ((cp[1] == '+') && (IsPathCharacter (cp[2]) ||
				    IsDriveCharacter (cp[2]) || (cp[2] == 0)))
	{
	    dp = (unsigned char *)GetVariableAsString (PWDVariable, FALSE);
	    cp += 2;
	}

	else if ((cp[1] == '-') && (IsPathCharacter (cp[2]) ||
				    IsDriveCharacter (cp[2]) || (cp[2] == 0)))
	{
	    dp = (unsigned char *)GetVariableAsString (OldPWDVariable, FALSE);
	    cp += 2;
	}

/* substitute */

	if (dp != NULL)
	{
	    while (*dp != 0)
		*wp++ = *dp++;

/* Remove double //'s on directories */

	    if (IsPathCharacter (*(wp - 1)) && IsPathCharacter (*cp))
		cp++;
	}
    }
}

/*
 * Sort Compare
 */

int	SortCompare (const void *a1, const void *a2)
{
    return strcmp (*((char **)a1), *((char **)a2));
}

/*
 * Return the position of Prefix StopWord in the quoted string
 */

static char * F_LOCAL WordScan (char *QString, int StopWord)
{
    int		VarSubNest = 0;

    while (TRUE)
    {
	switch (*(QString++))
	{
	    case WORD_EOS:
		return QString;

	    case WORD_CHAR:
	    case WORD_QCHAR:
	    case WORD_QTCHAR:
		QString++;
		break;

	    case WORD_OQUOTE:
	    case WORD_ODQUOTE:
	    case WORD_CQUOTE:
	    case WORD_CDQUOTE:
		break;

	    case WORD_OARRAY:
		VarSubNest++;
		break;

	    case WORD_CARRAY:
		if ((StopWord == WORD_CARRAY) && (VarSubNest == 0))
		    return QString;

		VarSubNest--;
		break;

	    case WORD_OSUBST:
		VarSubNest++;

		while (*(QString++) != 0)
		    continue;

		if (*QString != WORD_CSUBST)
		    QString++;

		break;

	    case WORD_CSUBST:
		if ((StopWord == WORD_CSUBST) && (VarSubNest == 0))
		    return QString;

		VarSubNest--;
		break;

	    case WORD_COMSUB:
	    case WORD_OMATHS:
		while (*(QString++) != 0)
		    continue;

		break;
	}
    }
}

/*
 * Maths substitute - convert $((....)) to a number.
 */

static int F_LOCAL MathsSubstitute (Expand *xp, char *sp)
{
    char	DecimalString[12];
    char	*esp;

    esp = substitute (sp, 0);
    sprintf (DecimalString, "%lu", EvaluateMathsExpression (esp));
    xp->str = StringCopy (DecimalString);
    return XSUB;
}


/*
 * Check for multi_drive prefix
 */

#if (OS_TYPE != OS_UNIX)
static unsigned char * F_LOCAL CheckForMultipleDrives (unsigned char *prefix)
{
    if ((*(prefix++) != CHAR_MAGIC) || (!IS_WildCard (*prefix)))
	return (unsigned char *)NULL;

    if (*prefix != CHAR_OPEN_BRACKETS)
	return *(prefix + 1) == CHAR_DRIVE ? prefix + 1 : (unsigned char *)NULL;

    while (*prefix && (*prefix != CHAR_CLOSE_BRACKETS))
    {
	if ((*prefix == CHAR_MATCH_RANGE) && (*(prefix + 1)))
	    ++prefix;

	++prefix;
    }

    return (*prefix && (*(prefix + 1) == CHAR_DRIVE))
		? prefix + 1 : (unsigned char *)NULL;
}
#endif

/*
 * A command tree is to be expanded for stdin
 */

static bool F_LOCAL ProcessCommandTree (C_Op *outtree, int localpipe)
{
    long		s_flags = flags;
    Break_C		*S_RList = Return_List;	/* Save loval links	*/
    Break_C		*S_BList = Break_List;
    Break_C		*S_SList = SShell_List;
    bool		s_ProcessingEXECCommand = ProcessingEXECCommand;
    int			Local_depth = Execute_stack_depth++;
    jmp_buf		ReturnPoint;
    int			ReturnValue;
    FunctionList	*s_CurrentFunction = CurrentFunction;
    Break_C		bc;

/* Create the pipe to read the output from the command string */

    S_dup2 (localpipe, 1);

    FL_CLEAR (FLAG_EXIT_ON_ERROR);
    FL_CLEAR (FLAG_ECHO_INPUT);
    FL_CLEAR (FLAG_NO_EXECUTE);

/* Set up new environment */

    ReturnValue = CreateGlobalVariableList (FLAGS_NONE);

    if ((ReturnValue != -1) && (!SetErrorPoint (ReturnPoint)))
    {
	Return_List = (Break_C *)NULL;
	Break_List  = (Break_C *)NULL;

/* Clear execute flags.  */

	ProcessingEXECCommand = TRUE;
	CurrentFunction = (FunctionList *)NULL;

/* Parse the line and execute it */

	if (setjmp (bc.CurrentReturnPoint) == 0)
	{
	    bc.NextExitLevel = SShell_List;
	    SShell_List = &bc;
	    ReturnValue = ExecuteParseTree (outtree, NOPIPE, NOPIPE, 0);
	}

/* Parse error */

	else
	    ReturnValue = -1;

/* Clean up any files around we nolonger need */

	ClearExtendedLineFile ();
    }

    else
	ReturnValue = -1;

/* Restore environment */

    RestoreEnvironment (ReturnValue, Local_depth);

/* Free old space */

    FreeAllHereDocuments (MemoryAreaLevel);

/* Ok - completed processing - restore environment and read the pipe */

    ProcessingEXECCommand = s_ProcessingEXECCommand;
    flags = s_flags;
    Return_List = S_RList;
    Break_List	= S_BList;
    SShell_List = S_SList;
    CurrentFunction = s_CurrentFunction;

/* Move pipe to start so we can read it */

    lseek (localpipe, 0L, SEEK_SET);
    return C2bool (ReturnValue != -1);
}

/*	(pc@hillside.co.uk)
 * I have decided to `fudge' alternations by picking up the compiled command
 * tree and working with it recursively to generate the set of arguments.
 * This has the advantage of making a single discrete change to the code
 *
 * This routine calls itself recursively
 *
 *	a) Scan forward looking for { building the output string if none found
 *	   then call expand - and exit
 *	b) When { found, scan forward finding the end }
 *	c) Add first alternate to output string
 *	d) Scan for the end of the string copying into output
 *	e) Call routine with new string
 *
 * Major complication is quoting
 */

static void F_LOCAL AlternationExpand (char   *cp,	/* input word	*/
				       Word_B **WordList,/* output words	*/
				       int    ExpandMode)/* DO* flags	*/
{
    char	*srcp = cp;
    char	*left;		/* destination string of left hand side	*/
    char	*leftend;	/* end of left hand side		*/
    char	*alt;		/* start of alterate section		*/
    char	*altend;	/* end of alternate section		*/
    char	*ap;		/* working pointer			*/
    char	*right;		/* right hand side			*/
    char	*rp;		/* used to copy right-hand side		*/
    size_t	maxlen;		/* max string length			*/

    maxlen  = WordScan (cp, 0) - cp;
    left    = GetAllocatedSpace (maxlen);
    leftend = left;

    if (AlternationScan (&srcp, &leftend, CHAR_OPEN_BRACES, 0) == 0)
    {
	ExpandAWord (cp, WordList, ExpandMode & EXPAND_NOALTS);
	ReleaseMemoryCell (left);
	return;
    }

/* We have a alternation section */

    alt = GetAllocatedSpace (maxlen);
    altend = alt;
    srcp += 2;

    if (AlternationScan (&srcp, &altend, CHAR_CLOSE_BRACES, 1) == 0)
    {
	ReleaseMemoryCell (left);
	ReleaseMemoryCell (alt);
	PrintErrorMessage ("Mis-matched {}.");
    }

    *(altend++) = WORD_CHAR;
    *(altend++) = ',';
    *altend = WORD_EOS;

/* finally we may have a right-hand side */

    right = srcp + 2;

/* glue the bits together making a new string */

    for (srcp = alt; *srcp != WORD_EOS;)
    {
	ap = leftend;

	if (AlternationScan (&srcp, &ap, ',', -1) == 0)
	{
	    ReleaseMemoryCell (left);
	    ReleaseMemoryCell (alt);
	    PrintErrorMessage ("Missing comma.");
	}

	srcp += 2;
	rp = right;
	AlternationScan (&rp, &ap, WORD_EOS, 0);
	AlternationExpand (left, WordList, ExpandMode);
    }

    ReleaseMemoryCell (left);
    ReleaseMemoryCell (alt);
    return;
}

/*
 * Scan the tree
 */

static int F_LOCAL AlternationScan (char **cpp,/* source pointer      */
			            char **dpp,/* destination pointer */
			            char endc, /* last character look for  */
			            int  bal)
{
    char	*cp, *dp;
    bool	QuoteStatus = FALSE;
    int		balance = 0;
    bool	UseBalance = FALSE;
    int		VarSubNest = 0;

    if (bal)
    {
	UseBalance = TRUE;
	balance = (bal < 1) ? 0 : 1;
    }

    cp = *cpp;
    dp = *dpp;

    while (*cp != WORD_EOS)
    {
	switch (*cp)
	{
	    case WORD_CHAR:
		if (QuoteStatus)
		{
		    if (cp[1] == CHAR_CLOSE_BRACKETS)
			QuoteStatus = FALSE;
		}

		else if (!QuoteStatus)
		{
		    if (cp[1] == CHAR_OPEN_BRACKETS)
			QuoteStatus = TRUE;

		    else
		    {
			if (UseBalance)
			{
			    if (cp[1] == CHAR_OPEN_BRACES)
				balance++;

			    if (cp[1] == CHAR_CLOSE_BRACES)
				balance--;
			}

			if ((cp[1] == endc) && (balance == 0))
			{
			    *dp = WORD_EOS;
			    *dpp = dp;
			    *cpp = cp;
			    return 1;
			}
		    }
		}

	    case WORD_QCHAR:
	    case WORD_QTCHAR:
		*(dp++) = *(cp++);
		*(dp++) = *(cp++);
		break;

	    case WORD_OQUOTE:
	    case WORD_ODQUOTE:
		QuoteStatus = TRUE;
		*(dp++) = *(cp++);
		break;

	    case WORD_CQUOTE:
	    case WORD_CDQUOTE:
		QuoteStatus = FALSE;
		*(dp++) = *(cp++);
		break;

	    case WORD_OARRAY:
		VarSubNest++;
		*(dp++) = *(cp++);
		break;

	    case WORD_OSUBST:
		VarSubNest++;

		while ((*(dp++) = *(cp++)))
		    continue;

		if (*cp != WORD_CSUBST)
		    *(dp++) = *(cp++);

		break;

	    case WORD_CSUBST:
	    case WORD_CARRAY:
		*(dp++) = *(cp++);
		VarSubNest--;
		break;

	    case WORD_COMSUB:
	    case WORD_OMATHS:
		while ((*(dp++) = *(cp++)))
		    continue;

		break;
	}
    }

    *dp = WORD_EOS;
    *cpp = cp;
    *dpp = dp;

    return 0;
}

/*
 * Handle Array Value between WORD_OARRAY & WORD_CARRAY
 * Return the array index (-1 == [*]).
 *			  (-2 == [@]).
 */

static int F_LOCAL HandleArrayValue (char *name,
				     char **InputString,
				     int  ExpandMode)
{
				/* Start after the Open Array		*/
    char	*End = WordScan ((*InputString) + 1, WORD_CARRAY);
    size_t	Length = (End - *InputString) - 1;
    char	*ExpWord;
    Word_B	*WordList = (Word_B *)NULL;
    long	value;

/* Build a copy of the substring to expand */

    ExpWord = memcpy (GetAllocatedSpace (Length), *InputString + 1, Length);
    ExpWord[Length - 1] = WORD_EOS;

    ExpandMode &= ~(EXPAND_GLOBBING | EXPAND_CONVERT | EXPAND_NOALTS);
    ExpandAWord (ExpWord, &WordList, ExpandMode);

/* Check for valid value */

    if (WordBlockSize (WordList) != 1)
	ShellErrorMessage (LIT_BadArray, "too many words");

/*
 * There are a couple of special cases:
 *
 * ${#name[*]}
 * ${name[*]}
 * ${name[@]}
 */

    if (!strcmp (WordList->w_words[0], "*"))
	value = -1L;

    else if (!strcmp (WordList->w_words[0], "@") &&
	     ((*name != '#') || (*(name + 1) == 0)))
	value = -2L;

/*
 * Otherwise, get the array value
 */

    else
    {
	value = EvaluateMathsExpression (WordList->w_words[0]);

	if ((value < 0) || (value > INT_MAX))
	    ShellErrorMessage (LIT_ArrayRange, name);
    }

    *InputString = End;
    return (int)value;
}

/*
 * Check for Unset Variable
 */

static void F_LOCAL CheckForUnset (char *name, int Index)
{
    if ((FL_TEST (FLAG_UNSET_ERROR)) &&
	(GetVariableArrayAsString (name, Index, FALSE) == null))
	ShellErrorMessage ("%s: unset variable", name);
}


/*
 * TWALK - Build list of the values of an Environment Variable
 */

static void BuildVariableEntryList (const void *key, VISIT visit, int level)
{
    VariableList	*vp = (*(VariableList **)key);

    if (((visit == postorder) || (visit == leaf)) &&
       (strcmp (GVAV_Name, vp->name) == 0))
	GVAV_WordList = AddWordToBlock (GetVariableArrayAsString (vp->name,
								  vp->index,
								  TRUE),
					GVAV_WordList);
}

/*
 * Return the number of floppy disks
 */

#if (OS_TYPE == OS_OS2)
static int F_LOCAL GetNumberofFloppyDrives (void)
{
    BYTE	nflop = 1;

#  if (OS_SIZE == OS_16)
    DosDevConfig (&nflop, DEVINFO_FLOPPY, 0);
#  else
    DosDevConfig (&nflop, DEVINFO_FLOPPY);
#  endif

    return nflop;
}
#endif

/* DOS Version */

#if (OS_TYPE == OS_DOS)
static int F_LOCAL GetNumberofFloppyDrives (void)
{
#  if defined (__TURBOC__)

    return ((biosequip () & 0x00c0) >> 6) + 1;

#  elif defined (__EMX__)

    union REGS		r;

    SystemInterrupt (0x11, &r, &r);
    return ((r.x.REG_AX & 0x00c0) >> 6) + 1;

#  else

    return ((_bios_equiplist () & 0x00c0) >> 6) + 1;

#  endif
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
static int F_LOCAL GetNumberofFloppyDrives (void)
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
#endif
