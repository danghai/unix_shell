/*
 * MS-DOS SHELL - Symantic Parser
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
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh2.c,v 2.12 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh2.c,v $
 *	Revision 2.12  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.11  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.10  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.9  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.8  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.7  1993/06/14  11:00:12  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.6  1993/06/02  09:52:35  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.5  1993/02/16  16:03:15  istewart
 *	Beta 2.22 Release
 *
 *	Revision 2.4  1993/01/26  18:35:09  istewart
 *	Release 2.2 beta 0
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
#include <stddef.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include "sh.h"

static C_Op * F_LOCAL	ScanPipeSyntax (int);
static C_Op * F_LOCAL	ScanAndOrSyntax (void);
static C_Op * F_LOCAL	CommandList (void);
static IO_Actions * F_LOCAL SynchroniseIOList (int);
static void F_LOCAL	CheckNextTokenIS (int, int);
static void F_LOCAL	SyntaxError (char *);
static C_Op * F_LOCAL	ScanNestedSyntax (int, int);
static C_Op * F_LOCAL	ScanSimpleCommand (int);
static C_Op * F_LOCAL	GetDoDoneCommandList (void);
static C_Op * F_LOCAL	ThenPartList (void);
static C_Op * F_LOCAL	ElsePartList (void);
static C_Op * F_LOCAL	CaseList (void);
static C_Op * F_LOCAL	CaseListEntries (void);
static char ** F_LOCAL	GetINWordList (void);
static C_Op * F_LOCAL	SetupTreeNode (int, C_Op *, C_Op *, char **);
static C_Op * F_LOCAL	CreateTreeNode (void);
static C_Op * F_LOCAL	yyparse (void);
static char * F_LOCAL	LookUpToken (int);
static int F_LOCAL	GetNextToken (int);
static int F_LOCAL	LookAtNextToken (int);
static char * F_LOCAL	MemoryDup (char *, size_t);

static char		*LIT_2ManyRedir = "Too many redirections";
static char		*LIT_Expecting = "%s - Expecting '%s', found '%s'";

/*
 * Special strings for (( and ))
 */

static char		LIT_ODP [] = {
    WORD_CHAR,
    CHAR_OPEN_PARATHENSIS,
    WORD_CHAR,
    CHAR_OPEN_PARATHENSIS,
    0
};

static char		LIT_CDP [] = {
    WORD_CHAR,
    CHAR_CLOSE_PARATHENSIS,
    WORD_CHAR,
    CHAR_CLOSE_PARATHENSIS,
    0
};

/* Special [[ */

static char		LIT_ODB [] = {
    WORD_CHAR,
    CHAR_OPEN_BRACKETS,
    WORD_CHAR,
    CHAR_OPEN_BRACKETS,
    0
};

/*
 * Other statics
 */

static bool		reject;		/* GetNextToken(cf) gets symbol	*/
					/* again			*/
static int		symbol;		/* yylex value			*/

#define NEWNODE		((C_Op *)CreateTreeNode ())
#define	REJECT		(reject = TRUE)
#define	ACCEPT		(reject = FALSE)

/*
 * Get the next token from input
 */

static int F_LOCAL	GetNextToken (int cf)
{
    if (reject)
	ACCEPT;
    
    else
    {
	symbol = ScanNextToken (cf);
	ACCEPT;
    }

    return symbol;
}

/*
 * Look at the next token from input
 */

static int F_LOCAL	LookAtNextToken (int cf)
{
    if (!reject)
    {
	symbol = ScanNextToken (cf);
	REJECT;
    }

    return symbol;
}

/*
 * Parse the current input stack
 */

static C_Op * F_LOCAL yyparse (void)
{
    C_Op	*t;		/* yyparse output */

    ACCEPT;
    yynerrs = 0;

/* Check for EOF */

    if ((LookAtNextToken (ALLOW_KEYWORD | ALLOW_ALIAS)) == 0)
    {
	(t = NEWNODE)->type = TEOF;
	DPRINT (1, ("yyparse: Create TEOF"));
    }

    else
    {
	t = CommandList ();
	CheckNextTokenIS (CHAR_NEW_LINE, 0);
    }

    return t;
}

/*
 * Check a pipeline
 */

static C_Op * F_LOCAL ScanPipeSyntax (int LexicalControlFlags)
{
    C_Op	*t, *p, *tl = NULL;

    if ((t = ScanSimpleCommand (LexicalControlFlags)) != (C_Op *)NULL)
    {
	while (GetNextToken (0) == CHAR_PIPE)
	{
	    if ((p = ScanSimpleCommand (ALLOW_CONTINUATION)) == NULL)
		SyntaxError ("no commands found following pipe");

	    if (tl == NULL)
	    {
		tl = SetupTreeNode (TPIPE, t, p, NOWORDS);
		t = tl;
	    }

	    else
	    {
		tl->right = SetupTreeNode (TPIPE, tl->right, p, NOWORDS);
		tl = tl->right;
	    }
	}

	REJECT;
    }

    return t;
}

static C_Op * F_LOCAL ScanAndOrSyntax (void)
{
    C_Op	*t, *p;
    int		c;

    t = ScanPipeSyntax (0);

    if (t != NULL)
    {
	while (((c = GetNextToken (0)) == PARSE_LOGICAL_AND) ||
	       (c == PARSE_LOGICAL_OR))
	{
	    if ((p = ScanPipeSyntax (ALLOW_CONTINUATION)) == NULL)
		SyntaxError ("no commands found following || or &&");

	    t = SetupTreeNode ((c == PARSE_LOGICAL_AND) ? TAND : TOR, t, p,
			       NOWORDS);
	}

	REJECT;
    }

    return t;
}

static C_Op *F_LOCAL CommandList (void)
{
    C_Op	*t, *p, *tl = NULL;
    int		c;

    if ((t = ScanAndOrSyntax ()) != NULL)
    {
	while (((c = GetNextToken (0)) == CHAR_SEPARATOR) ||
	       (c == CHAR_ASYNC) || (c == PARSE_COPROCESS) ||
	       ((AllowMultipleLines ||
	        (source->type == SSTRING) ||
		(source->type == SALIAS)) && (c == CHAR_NEW_LINE)))
	{
	    if ((c == CHAR_ASYNC) || (c == PARSE_COPROCESS))
	    {
		c = (c == CHAR_ASYNC) ? TASYNC : TCOPROCESS;

		if (tl)
		    tl->right = SetupTreeNode (c, tl->right, NOBLOCK, NOWORDS);

		else
		    t = SetupTreeNode (c, t, NOBLOCK, NOWORDS);
	    }

	    if ((p = ScanAndOrSyntax ()) == NULL)
		return t;

	    if (tl == NULL)
	    {
		tl = SetupTreeNode (TLIST, t, p, NOWORDS);
		t = tl;
	    }

	    else
	    {
		tl->right = SetupTreeNode (TLIST, tl->right, p, NOWORDS);
		tl = tl->right;
	    }
	}

	REJECT;
    }

    return t;
}

/*
 * Handle IO re-direction
 */

static IO_Actions * F_LOCAL SynchroniseIOList (int LexicalControlFlags)
{
    IO_Actions		*iop;

    if (LookAtNextToken (LexicalControlFlags) != PARSE_REDIR)
	return (IO_Actions *)NULL;

    ACCEPT;
    iop = yylval.iop;
    CheckNextTokenIS (PARSE_WORD, 0);
    iop->io_name = yylval.cp;

    if ((iop->io_flag & IOTYPE) == IOHERE)
    {
	if (*CurrentLexIdentifier != 0) /* unquoted */
	    iop->io_flag |= IOEVAL;

	SaveHereDocumentInfo (iop);
    }

    return iop;
}

static void F_LOCAL CheckNextTokenIS (int c, int LexicalControlFlags)
{
    int		got;

    if ((got = GetNextToken (LexicalControlFlags)) != c)
    {
	CompilingError ();
	ShellErrorMessage (LIT_Expecting, LIT_SyntaxError, LookUpToken (c),
			   LookUpToken (got));
    }
}

/*
 * Handle Nested thingys - ( and {
 */

static C_Op * F_LOCAL ScanNestedSyntax (int type, int mark)
{
    C_Op	*t;

    AllowMultipleLines++;
    t = CommandList ();
    CheckNextTokenIS (mark, ALLOW_KEYWORD);
    AllowMultipleLines--;
    return SetupTreeNode (type, t, NOBLOCK, NOWORDS);
}

/*
 * Handle a single command and its bits and pieces - IO redirection,
 * arguments and variable assignments
 */

static C_Op * F_LOCAL ScanSimpleCommand (int LexicalControlFlags)
{
    C_Op	*t;
    int			c;
    IO_Actions		*iop;
    Word_B		*Arguments = (Word_B *)NULL;
    Word_B		*Variables = (Word_B *)NULL;
    Word_B		*IOactions = (Word_B *)NULL;

/* Allocate space for IO actions structures */

    if (AllowMultipleLines)
	LexicalControlFlags = ALLOW_CONTINUATION;

    LexicalControlFlags |= ALLOW_KEYWORD | ALLOW_ALIAS;

    while ((iop = SynchroniseIOList (LexicalControlFlags)) != NULL)
    {
	if (WordBlockSize (IOactions) >= NUFILE)
	{
	    CompilingError ();
	    ShellErrorMessage (LIT_2ManyRedir);
	}

	IOactions = AddWordToBlock ((char *)iop, IOactions);
	LexicalControlFlags &=~ ALLOW_CONTINUATION;
    }

    switch (c = GetNextToken (LexicalControlFlags))
    {
	case 0:
	    CompilingError ();
	    ShellErrorMessage ("unexpected EOF");
	    return NULL;

	case CHAR_SEPARATOR:
	    REJECT;
	    (t = NEWNODE)->type = TCOM;
	    DPRINT (1, ("ScanSimpleCommand: Create TCOM"));
	    break;

	default:
	    REJECT;

	    if (WordBlockSize (IOactions) == 0)
		return (C_Op *)NULL;		/* empty line		*/

	    (t = NEWNODE)->type = TCOM;
	    DPRINT (1, ("ScanSimpleCommand: Create TCOM"));
	    break;

	case PARSE_WORD:
	case PARSE_MDPAREN:
	    REJECT;
	    (t = NEWNODE)->type = TCOM;
	    DPRINT (1, ("ScanSimpleCommand: Create TCOM"));

	    if (c == PARSE_MDPAREN)
	    {
		ACCEPT;
		Arguments = AddWordToBlock (MemoryDup (LIT_ODP, 5), Arguments);
		CheckNextTokenIS (PARSE_WORD, MATHS_EXPRESSION);
		Arguments = AddWordToBlock (yylval.cp, Arguments);
		Arguments = AddWordToBlock (MemoryDup (LIT_CDP, 5), Arguments);
	    }

	    while (1)
	    {
		switch (LookAtNextToken (0))
		{
		    case PARSE_REDIR:
			if (WordBlockSize (IOactions) >= NUFILE)
			{
			    CompilingError ();
			    ShellErrorMessage (LIT_2ManyRedir);
			}

			IOactions = AddWordToBlock (
					(char *)SynchroniseIOList (0),
					IOactions);
			break;

/*
 * Word - check to see if this should be an argument or a variable,
 * depending on what we've seen, the state of the k flag and an
 * assignment in the word
 */

		    case PARSE_WORD:
			ACCEPT;
			if (((WordBlockSize (Arguments) == 0) ||
			     FL_TEST (FLAG_ALL_KEYWORDS)) &&
			    (strchr (CurrentLexIdentifier + 1, CHAR_ASSIGN) !=
					(char *)NULL))
			    Variables = AddWordToBlock (yylval.cp, Variables);

			else
			    Arguments = AddWordToBlock (yylval.cp, Arguments);

			break;

		    case PARSE_MPAREN:
			ACCEPT;

			if (WordBlockSize (Arguments) != 1)
			    SyntaxError ("Too many function names");

			if (*CurrentLexIdentifier == 0)
			    SyntaxError ("Bad function name");

			(t = NEWNODE)->type = TFUNC;
			DPRINT (1, ("ScanSimpleCommand: Create TFUNC"));
			t->str = StringCopy (CurrentLexIdentifier);
			CheckNextTokenIS (CHAR_OPEN_BRACES,
					  ALLOW_CONTINUATION | ALLOW_KEYWORD);
			t->left = ScanNestedSyntax (TBRACE, CHAR_CLOSE_BRACES);
			return t;

		    default:
			goto Leave;
		}
	    }
Leave:
	    break;

	case CHAR_OPEN_PARATHENSIS:
	    t = ScanNestedSyntax (TPAREN, CHAR_CLOSE_PARATHENSIS);
	    break;

	case CHAR_OPEN_BRACES:
	    t = ScanNestedSyntax (TBRACE, CHAR_CLOSE_BRACES);
	    break;

/*
 * Format for:  [[ .....  ]]
 */

	case PARSE_TEST:
	    (t = NEWNODE)->type = TCOM;
	    DPRINT (1, ("ScanSimpleCommand: Create TCOM"));
	    Arguments = AddWordToBlock (MemoryDup (LIT_ODB, 5), Arguments);

	    while (GetNextToken (TEST_EXPRESSION) == PARSE_WORD)
	    {
		Arguments = AddWordToBlock (yylval.cp, Arguments);

		if (strcmp (CurrentLexIdentifier , "]]") == 0)
		    break;
	    }

	    break;

/*
 * Format for:	select word in list do .... done
 * 		select word do .... done
 *		for word in list do .... done
 * 		for word do .... done
 */

	case PARSE_FOR:
	case PARSE_SELECT:
	    (t = NEWNODE)->type = (c == PARSE_FOR) ? TFOR : TSELECT;
	    DPRINT (1, ("ScanSimpleCommand: Create TFOR/TSELECT"));
	    CheckNextTokenIS (PARSE_WORD, 0);
	    t->str = StringCopy (CurrentLexIdentifier);
	    AllowMultipleLines++;
	    t->vars = GetINWordList ();
	    t->left = GetDoDoneCommandList ();
	    AllowMultipleLines--;
	    break;


/*
 * Format for:	while command do ... done
 * 		until command do ... done
 */

	case PARSE_WHILE:
	case PARSE_UNTIL:
	    AllowMultipleLines++;
	    (t = NEWNODE)->type = (c == PARSE_WHILE) ? TWHILE : TUNTIL;
	    DPRINT (1, ("ScanSimpleCommand: Create TWHILE/TUNTIL"));
	    t->left = CommandList ();
	    t->right = GetDoDoneCommandList ();
	    AllowMultipleLines--;
	    break;

/*
 * Format for:	case name in .... esac
 */

	case PARSE_CASE:
	    (t = NEWNODE)->type = TCASE;
	    DPRINT (1, ("ScanSimpleCommand: Create TCASE"));
	    CheckNextTokenIS (PARSE_WORD, 0);
	    t->str = yylval.cp;
	    AllowMultipleLines++;
	    CheckNextTokenIS (PARSE_IN, ALLOW_KEYWORD | ALLOW_CONTINUATION);
	    t->left = CaseList ();
	    CheckNextTokenIS (PARSE_ESAC, ALLOW_KEYWORD);
	    AllowMultipleLines--;
	    break;

/*
 * Format for:	if command then command fi
 *		if command then command else command fi
 *		if command then command elif command then ... else ... fi
 */

	case PARSE_IF:
	    AllowMultipleLines++;
	    (t = NEWNODE)->type = TIF;
	    DPRINT (1, ("ScanSimpleCommand: Create TIF"));
	    t->left = CommandList ();
	    t->right = ThenPartList ();
	    CheckNextTokenIS (PARSE_FI, ALLOW_KEYWORD);
	    AllowMultipleLines--;
	    break;

/*
 * Format for: time command
 */

	case PARSE_TIME:
	    t = ScanPipeSyntax (ALLOW_CONTINUATION);
	    t = SetupTreeNode (TTIME, t, NOBLOCK, NOWORDS);
	    break;

/*
 * Format for:  function name { .... }
 */

	case PARSE_FUNCTION:
	    (t = NEWNODE)->type = TFUNC;
	    DPRINT (1, ("ScanSimpleCommand: Create TFUNC"));
	    CheckNextTokenIS (PARSE_WORD, 0);
	    t->str = StringCopy (CurrentLexIdentifier);
	    CheckNextTokenIS (CHAR_OPEN_BRACES,
			      (ALLOW_CONTINUATION | ALLOW_KEYWORD));
	    t->left = ScanNestedSyntax (TBRACE, CHAR_CLOSE_BRACES);
	    break;
    }

/* Get any remaining IOactions */

    while ((iop = SynchroniseIOList (ALLOW_KEYWORD)) != NULL)
    {
	if (WordBlockSize (IOactions) >= NUFILE)
	{
	    CompilingError ();
	    ShellErrorMessage (LIT_2ManyRedir);
	}

	IOactions = AddWordToBlock ((char *)iop, IOactions);
    }

/* Save the IOactions */

    if (WordBlockSize (IOactions) == 0)
	t->ioact = (IO_Actions **)NULL;

    else
	t->ioact = (IO_Actions **) GetWordList (AddWordToBlock (NOWORD,
								IOactions));

/* If TCOM, save the arguments and variable assignments */

    if (t->type == TCOM)
    {
	t->args = GetWordList (AddWordToBlock (NOWORD, Arguments));
	t->vars = GetWordList (AddWordToBlock (NOWORD, Variables));
    }

/* Handle re-direction on other pipelines */

    else if ((t->type != TPAREN) && (t->ioact != (IO_Actions **)NULL))
    {
	C_Op		*t1 = t;

	(t = NEWNODE)->type = TPAREN;
	DPRINT (1, ("ScanSimpleCommand: Create TPAREN"));
	t->left = t1;
	t->right = NOBLOCK;
	t->args = NOWORDS;
	t->vars = NOWORDS;
	t->ioact = t1->ioact;
	t1->ioact = (IO_Actions **)NULL;
    }

/*
 * We should probably release IOactions, Arguments and Variables if they
 * are not used.  However, I don't think its necessary.  The release memory
 * level should do it.
 */

    return t;
}


/*
 * Processing for the do grouping - do ... done
 */

static C_Op * F_LOCAL GetDoDoneCommandList (void)
{
    int		c;
    C_Op	*list;

    if ((c = GetNextToken (ALLOW_CONTINUATION | ALLOW_KEYWORD)) != PARSE_DO)
    {
	CompilingError ();
	ShellErrorMessage (LIT_Expecting, LIT_SyntaxError, "do",
			   LookUpToken (c));
    }

    list = CommandList ();
    CheckNextTokenIS (PARSE_DONE, ALLOW_KEYWORD);
    return list;
}


/*
 * Handle the then part of an if statement
 */

static C_Op * F_LOCAL ThenPartList (void)
{
    C_Op	*t;

    if (GetNextToken (0) != PARSE_THEN)
    {
	REJECT;
	return (C_Op *)NULL;
    }

    (t = NEWNODE)->type = 0;
    DPRINT (1, ("ThenPartList: Create dummy"));

    if ((t->left = CommandList ()) == (C_Op *)NULL)
	SyntaxError ("no command found after then");

    t->right = ElsePartList ();
    return t;
}


/*
 * Handle the else part of an if statement
 */

static C_Op * F_LOCAL ElsePartList (void)
{
    C_Op	*t;

    switch (GetNextToken (0))
    {
      case PARSE_ELSE:
	if ((t = CommandList ()) == (C_Op *)NULL)
	    SyntaxError ("no commands associated with else");

	return t;

      case PARSE_ELIF:
	(t = NEWNODE)->type = TELIF;
	DPRINT (1, ("ElsePartList: Create TELIF"));
	t->left = CommandList ();
	t->right = ThenPartList ();
	return t;

      default:
	REJECT;
	return (C_Op *)NULL;
    }
}


/*
 * Process the CASE statment
 */

static C_Op * F_LOCAL CaseList (void)
{
    C_Op	*t = (C_Op *)NULL;
    C_Op	*tl = (C_Op *)NULL;

    while ((LookAtNextToken (ALLOW_CONTINUATION | ALLOW_KEYWORD)) != PARSE_ESAC)
    {
	C_Op	*tc = CaseListEntries ();

	if (tl == (C_Op *)NULL)
	{
	    t = tc;
	    (tl = tc)->right = (C_Op *)NULL;
	}

	else
	{
	    tl->right = tc;
	    tl = tc;
	}
    }

    return t;
}


/*
 * Process an individual case entry: pattern) commands;;
 */

static C_Op * F_LOCAL CaseListEntries (void)
{
    C_Op	*t;
    int		LexicalControlFlags = ALLOW_CONTINUATION | ALLOW_KEYWORD;
    Word_B	*Patterns = (Word_B *)NULL;

    (t = NEWNODE)->type = TPAT;
    DPRINT (1, ("CaseListEntries: Create TPAT"));

    if (GetNextToken (LexicalControlFlags) != CHAR_OPEN_PARATHENSIS)
	REJECT;

    else
	LexicalControlFlags = 0;

    do
    {
	CheckNextTokenIS (PARSE_WORD, LexicalControlFlags);
	Patterns = AddWordToBlock (yylval.cp, Patterns);
	LexicalControlFlags = 0;
    } while (GetNextToken (0) == CHAR_PIPE);

    REJECT;

/*
 * Terminate the list of patterns
 */

    t->vars = GetWordList (AddWordToBlock (NOWORD, Patterns));

/*
 * Check for the terminating ), and get the command list
 */

    CheckNextTokenIS (CHAR_CLOSE_PARATHENSIS, 0);

    t->left = CommandList ();

    if ((LookAtNextToken (ALLOW_CONTINUATION | ALLOW_KEYWORD)) != PARSE_ESAC)
	CheckNextTokenIS (PARSE_BREAK, ALLOW_CONTINUATION | ALLOW_KEYWORD);

    return (t);
}


/*
 * Handle the in words.... part of a for or select statement.  Get the
 * words and build a list.
 */

static char ** F_LOCAL GetINWordList (void)
{
    int		c;
    Word_B	*Parameters = (Word_B *)NULL;

/*
 * Check to see if the next symbol is "in".  If not there are no words
 * following
 */

    if ((c = GetNextToken (ALLOW_CONTINUATION | ALLOW_KEYWORD)) != PARSE_IN)
    {
	REJECT;
	return NOWORDS;
    }

/* Get the list */

    while ((c = GetNextToken (0)) == PARSE_WORD)
	Parameters = AddWordToBlock (yylval.cp, Parameters);

    if ((c != CHAR_NEW_LINE) && (c != CHAR_SEPARATOR))
    {
	CompilingError ();
	ShellErrorMessage (LIT_Expecting, LIT_SyntaxError, "newline' or ';",
			   LookUpToken (c));
    }

/* Are there any words found? */

    if (Parameters == (Word_B *)NULL)
	return NOWORDS;

    return GetWordList (AddWordToBlock (NOWORD, Parameters));
}

/*
 * supporting functions
 */

static C_Op * F_LOCAL SetupTreeNode (int type, C_Op *t1, C_Op *t2, char **wp)
{
    C_Op	*t;

    (t = NEWNODE)->type = type;
    DPRINT (1, ("SetupTreeNode: Create %d", type));
    t->left = t1;
    t->right = t2;
    t->vars = wp;
    return t;
}

/*
 * Get and compile the next command from the user/file etc
 */

C_Op	*BuildParseTree (Source *s)
{
    C_Op	*t;		/* yyparse output */

    yynerrs = 0;
    AllowMultipleLines = 0;
    source = s;

    t = yyparse ();

    if (s->type == STTY || s->type == SFILE)
	s->str = null;			/* line is not preserved	*/

    return yynerrs ? (C_Op *)NULL : t;
}

/*
 * Get a new tree leaf structure
 */

static C_Op * F_LOCAL CreateTreeNode (void)
{
    C_Op	*t;

    if ((t = (C_Op *)AllocateMemoryCell (sizeof (C_Op))) == (C_Op *)NULL)
	ShellErrorMessage ("command line too complicated");

    return t;
}

/*
 * List of keywords
 */

static struct res {
    char	*r_name;
    int		r_val;
} restab[] = {
    { "for",	PARSE_FOR},		{ "case",	PARSE_CASE},
    { "esac",	PARSE_ESAC},		{ "while",	PARSE_WHILE},
    { "do",	PARSE_DO},		{ LIT_done,	PARSE_DONE},
    { "if",	PARSE_IF},		{ "in",		PARSE_IN},
    { "then",	PARSE_THEN},		{ "else",	PARSE_ELSE},
    { "elif",	PARSE_ELIF},		{ "until",	PARSE_UNTIL},
    { "fi",	PARSE_FI},		{ "select",	PARSE_SELECT},
    { "time",	PARSE_TIME},		{ "function",	PARSE_FUNCTION},
    { LIT_Test,	PARSE_TEST},
    { "{",	CHAR_OPEN_BRACES},	{ "}",		CHAR_CLOSE_BRACES},

    { (char *)NULL,	0},

/* Additional definitions */

    { "word",	PARSE_WORD},		{ "&&",		PARSE_LOGICAL_AND},
    { "||",	PARSE_LOGICAL_OR},	{ "redirection",PARSE_REDIR },
    { "(..)",	PARSE_MPAREN},		{ "((...))",	PARSE_MDPAREN},
    { "|&",	PARSE_COPROCESS},	{ "newline",'\n'},

    { (char *)NULL,	0}
};

int LookUpSymbol (char *n)
{
    struct res		*rp = restab;

    while ((rp->r_name != (char *)NULL) && strcmp (rp->r_name, n))
	rp++;

    return rp->r_val;
}

static char * F_LOCAL LookUpToken (int n)
{
    struct res		*rp = restab;
    int			first = TRUE;

    while (TRUE)
    {
        if ((rp->r_name == (char *)NULL) && !first)
	{
	    char 	*cp = GetAllocatedSpace (4);

	    if (cp == (char *)NULL)
		return (char *)NULL;

	    sprintf (cp, "%c", n);
	    return cp;
	}

	else if (rp->r_name == (char *)NULL)
	    first = FALSE;

	else if (rp->r_val == n)
	    return rp->r_name;

	rp++;
    }
}

/*
 * Syntax error
 */

static void F_LOCAL SyntaxError (char *emsg)
{
    CompilingError ();
    ShellErrorMessage ("%s - %s", LIT_SyntaxError, emsg);
}

/*
 * Duplicate a memory string
 */

static char * F_LOCAL	MemoryDup (char *string, size_t length)
{
    char	*t;

    if ((t = AllocateMemoryCell (length)) == (char *)NULL)
	ShellErrorMessage ("Out of memory");
    
    return memcpy (t, string, length);
}
