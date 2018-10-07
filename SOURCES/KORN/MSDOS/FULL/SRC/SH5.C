/*
 * MS-DOS SHELL - Lexical Scanner
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
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh5.c,v 2.13 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh5.c,v $
 *	Revision 2.13  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.12  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.11  1994/01/11  17:55:25  istewart
 *	Release 2.3 Beta 0 patches
 *
 *	Revision 2.10  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include "sh.h"

/*
 * lexical analysis and source input
 */

/* we set s->str to NULLSTR instead of "", so that ungetsc() works */

static	char	nullstr [] = {0, 0};
#define	NULLSTR	(nullstr + 1)

static bool		expanding_alias = FALSE;
static bool		AllowMultipleAliases = FALSE;

/*
 * Here document processing
 */

typedef struct here {
    struct here		*h_next;	/* Link to next			*/
    IO_Actions		*h_iop;		/* I/O options			*/
} Here_D;

/*
 * The full list contains all the documents created during a parse.  The
 * active list, contains the non-processed ones.
 */

					/* Full list header		*/
static Here_D		*HereListHead = (Here_D *)NULL;
					/* Active list Header		*/
static Here_D		*ActiveListHead = (Here_D *)NULL;


static void F_LOCAL	ReadHereDocument (IO_Actions *iop);
static void F_LOCAL	GetHereDocuments (void);

static char		*LIT_Unclosed = "here document `%s' unclosed";

/*
 * Get next character functions
 */

static int F_LOCAL	getsc_ (void);

/* optimized getsc_() */

#define	getsc()		((*source->str != 0) ? (int)*(source->str++) : getsc_())
#define	ungetsc()	(source->str--)

/*
 * states while lexing word
 */

#define	SBASE		0	/* outside any lexical constructs	*/
#define	SSQUOTE		1	/* inside ''				*/
#define	SDQUOTE		2	/* inside ""				*/
#define	SBRACE		3	/* inside ${}				*/
#define	SPAREN		4	/* inside $()				*/
#define	SBQUOTE		5	/* inside ``				*/
#define	SWORD		6	/* implicit quoting for substitute()	*/
#define	SDPAREN		7	/* inside (( )), implicit quoting	*/
#define	SDDPAREN	8	/* inside $(( )), implicit quoting	*/
#define	SVSUBST		9	/* inside ${ }, following the V name	*/
#define	SVARRAY		10	/* inside ${name [..].. } 		*/

/*
 * Lexical analyzer
 *
 * tokens are not regular expressions, they are LL(1).
 * for example, "${var:-${PWD}}", and "$(size $(whence ksh))".
 * hence the state stack.
 */

int	ScanNextToken (int LexicalControlFlags)
{
    int			c;
    char		states [64], *statep = states;
    XString		ws;		/* expandable output word	*/
    unsigned char	*wp;		/* output word pointer		*/
    char		*sp, *dp;
    char		istate;
    char		state;
    int			c2;
    static int		RecursiveAliasCount = 0;
    static AliasList	*RecursiveAlias[MAX_RECURSIVEALIASES];
    int			SDD_Count;	/* Parathensis counter		*/
    char		LexSeparator;

    if (expanding_alias)
    {
	expanding_alias = FALSE;

	while (RecursiveAliasCount-- > 0)
	    RecursiveAlias[RecursiveAliasCount]->AFlags &= ~ALIAS_EXPANDING;

	RecursiveAliasCount = 0;
    }

/*
 * Loop round again!
 */

Again:

    wp = (unsigned char *)XCreate (&ws, 64);

/*
 * Single word ?
 */

    if (LexicalControlFlags & ONEWORD)
	istate = SWORD;

/*
 * Maths expression?
 */

    else if (LexicalControlFlags & MATHS_EXPRESSION)
    {
	istate = SDPAREN;
	*(wp)++ = WORD_ODQUOTE;
    }

/*
 * Normal scanning
 */

    else
    {
	istate = SBASE;

/* Ignore white space */

	while ((c = getsc ()) == CHAR_SPACE || c == CHAR_TAB)
	    continue;

/* Comment? */

	if (c == CHAR_COMMENT)
	{
	    while (((c = getsc ()) != 0) && (c != CHAR_NEW_LINE))
		continue;
	}

	ungetsc ();
    }

/*
 * Which separator test to use?
 */

    LexSeparator = (char)((LexicalControlFlags & TEST_EXPRESSION)
			? (C_IFS | C_SEMICOLON)
			: C_LEX1);

/*
 * trailing ' ' in alias definition - Yes, allow more aliases
 */

    if (AllowMultipleAliases)
    {
	AllowMultipleAliases = FALSE;
	LexicalControlFlags |= ALLOW_ALIAS;
    }

/*
 * collect non-special or quoted characters to form word
 */

    for (*statep = state = istate;
	 !(((c = getsc ()) == 0) ||
	   ((state == SBASE) && (CharTypes[c] & LexSeparator))); )
    {
	XCheck (&ws, &wp);

	switch (state)
	{
	    case SBASE:
BasicCharacterProcessing:
		switch (c)
		{
		    case CHAR_META:
			c = getsc ();

			if (c != CHAR_NEW_LINE)
			{
			    *(wp)++ = WORD_QTCHAR;
			    *(wp)++ = (unsigned char)c;
			}

			else if (wp == XStart (ws))
			    goto Again;

			break;

		    case CHAR_SINGLE_QUOTE:
			*++statep = state = SSQUOTE;
			*(wp)++ = WORD_OQUOTE;
			break;

		    case CHAR_DOUBLE_QUOTE:
			*++statep = state = SDQUOTE;
			*(wp)++ = WORD_ODQUOTE;
			break;

		    default:
			goto Subst;
		}

		break;

/* Non-special character processing */

Subst:
		switch (c)
		{

/*
 * Escaped character
 */
		    case CHAR_META:
			switch (c = getsc ())
			{
			    case CHAR_NEW_LINE:
				break;

			    case CHAR_DOUBLE_QUOTE:
			    case CHAR_META:
			    case CHAR_VARIABLE:
			    case CHAR_BACKQUOTE:
				*(wp)++ = WORD_QTCHAR;
				*(wp)++ = (unsigned char)c;
				break;

			    default:
				XCheck (&ws, &wp);
				*(wp)++ = WORD_CHAR;
				*(wp)++ = CHAR_META;
				*(wp)++ = WORD_CHAR;
				*(wp)++ = (unsigned char)c;
				break;
			}

			break;

/*
 * Handler $(), $(()), ${}, $var, $num, $#, $*, $@
 */

		    case CHAR_VARIABLE:
			c = getsc ();

/* Check for $() & $(()) */

			if (c == CHAR_OPEN_PARATHENSIS)
			{
			    if (getsc () == CHAR_OPEN_PARATHENSIS)
			    {
				*++statep = state = SDDPAREN;
				*(wp)++ = WORD_OMATHS;
				SDD_Count = 0;
			    }

			    else
			    {
				ungetsc ();
				*++statep = state = SPAREN;
				*(wp)++ = WORD_COMSUB;
			    }
			}

/* Check for ${} */

			else if (c == CHAR_OPEN_BRACES)
			{
			    *++statep = state = SVSUBST;
			    *(wp)++ = WORD_OSUBST;
			    c = getsc ();

			    if (!IS_VariableFC (c) && !IS_VarNumeric (c))
				ShellErrorMessage ("invalid variable name");

			    do
			    {
				XCheck (&ws, &wp);
				*(wp)++ = (unsigned char)c;
				c = getsc ();
			    } while (IS_VariableSC (c));

			    *(wp)++ = 0;
			    ungetsc ();
			}

/* Check for $var */

			else if (IS_VariableFC (c))
			{
			    *(wp)++ = WORD_OSUBST;

			    do
			    {
				XCheck (&ws, &wp);
				*(wp)++ = (unsigned char)c;
				c = getsc ();
			    } while (IS_VariableSC(c));

			    *(wp)++ = 0;
			    *(wp)++ = WORD_CSUBST;
			    ungetsc ();

			}

/* Check for $number, $*, $@, $#, $! $$ $- $? */

			else if (IS_VarNumeric (c))
			{
			    XCheck (&ws, &wp);
			    *(wp)++ = WORD_OSUBST;
			    *(wp)++ = (unsigned char)c;
			    *(wp)++ = 0;
			    *(wp)++ = WORD_CSUBST;
			}

/* $??? - no mapping */

			else
			{
			    *(wp)++ = WORD_CHAR;
			    *(wp)++ = CHAR_VARIABLE;
			    ungetsc ();
			}

			break;

/*
 *  Handler ` ` (old style $())
 */
		    case CHAR_BACKQUOTE:
			*++statep = state = SBQUOTE;
			*(wp)++ = WORD_COMSUB;
			break;

/*
 * Normal character
 */

		    default:
			*(wp)++ = WORD_CHAR;
			*(wp)++ = (unsigned char)c;
		}

		break;

	    case SSQUOTE:			/* Inside '...'		*/
		if (c == CHAR_SINGLE_QUOTE)
		{
		    state = *--statep;
		    *(wp)++ = WORD_CQUOTE;
		}

/* Check for an escaped '.  None of the UNIX versions seem to do this, so
 * it may be wrong, so I've #if it out
 */

		else
		{
		    *(wp)++ = WORD_QCHAR;
		    *(wp)++ = (unsigned char)c;
		}

#if 0
		else if (c != CHAR_META)
		{
		    *(wp)++ = WORD_QCHAR;
		    *(wp)++ = (unsigned char)c;
		}

/* Yes - insert a quoted ' */
		else if ((c = getsc ()) == CHAR_SINGLE_QUOTE)
		{
		    *(wp)++ = WORD_QTCHAR;
		    *(wp)++ = CHAR_SINGLE_QUOTE;
		}

/* No - unget the character and insert the meta */

		else
		{
		    ungetsc ();
		    *(wp)++ = WORD_QCHAR;
		    *(wp)++ = CHAR_META;
		}
#endif

		break;

	    case SDQUOTE:			/* Inside "..."		*/
		if (c == CHAR_DOUBLE_QUOTE)
		{
		    state = *--statep;
		    *(wp)++ = WORD_CDQUOTE;
		}

		else
		    goto Subst;

		break;

	    case SPAREN:			/* Inside $(...)	*/
		if (c == CHAR_OPEN_PARATHENSIS)
		    *++statep = state;

		else if (c == CHAR_CLOSE_PARATHENSIS)
		    state = *--statep;

		if (state == SPAREN)
		    *(wp)++ = (unsigned char)c;

		else
		    *(wp)++ = 0; /* end of WORD_COMSUB */

		break;

	    case SBRACE:			/* Inside ${...}	*/
		if (c != CHAR_CLOSE_BRACES)
		    goto BasicCharacterProcessing;

		state = *--statep;
		*(wp)++ = WORD_CSUBST;
		break;

	    case SVARRAY:			/* Inside ${name [...].}*/
		if (c != CHAR_CLOSE_BRACKETS)
		    goto BasicCharacterProcessing;

		state = *--statep;
		*(wp)++ = WORD_CARRAY;
		break;


	    case SVSUBST:		/* Inside ${ }, following the 	*/						/* Variable name		*/

/* Set state to SBRACE to handle closing brace. */

		*statep = state = SBRACE;

/* Simple Name
 *
 * Possibilities: ${name}
 */

		if (c == CHAR_CLOSE_BRACES)
		    ungetsc ();

/*
 * Array Name.  Reset to this state for closing ] post processing
 *
 * Possibilities: ${name[index].....}
 */

		else if (c == CHAR_OPEN_BRACKETS)
		{
		    *statep = SVSUBST;
		    *++statep = state = SVARRAY;
		    *(wp)++ = WORD_OARRAY;
		}

/* Korn pattern trimming
 *
 * Possibilities: ${name#string}
 *		  ${name##string}
 *		  ${name%string}
 */

		else if ((c == CHAR_MATCH_START) || (c == CHAR_MATCH_END))
		{
		    if (getsc () == c)
			c |= CHAR_MAGIC;

		    else
			ungetsc ();

		    *(wp)++ = (unsigned char)c;
		}

/* Subsitution
 *
 * Possibilities: ${name:?value}
 * 		  ${name:-value}
 * 		  ${name:=value}
 * 		  ${name:+value}
 *		  ${name?value}
 * 		  ${name-value}
 * 		  ${name=value}
 * 		  ${name+value}
 */

		else
		{
		    c2 = (c == ':') ? CHAR_MAGIC : 0;

		    if (c == ':')
			c = getsc();

		    if (!IS_VarOp (c))
		    {
			CompilingError ();
			ShellErrorMessage ("Unknown substitution operator '%c'",
					   c);
		    }

		    *(wp)++ = (unsigned char)(c | c2);
		}

		break;

	    case SBQUOTE:			/* Inside `...`		*/
		if (c == CHAR_BACKQUOTE)
		{
		    *(wp)++ = 0;
		    state = *--statep;
		}

		else if (c == CHAR_META)
		{
		    switch (c = getsc())
		    {
			case CHAR_NEW_LINE:
			    break;

			case CHAR_META:
			case CHAR_VARIABLE:
			case CHAR_BACKQUOTE:
			    *(wp)++ = (unsigned char)c;
			    break;

			default:
			    *(wp)++ = CHAR_META;
			    *(wp)++ = (unsigned char)c;
			    break;
		    }
		}

		else
		    *(wp)++ = (unsigned char)c;

		break;

	    case SWORD:				/* ONEWORD		*/
		goto Subst;

	    case SDPAREN:			/* Include ((....))	*/
		if (c == CHAR_CLOSE_PARATHENSIS)
		{
		    if (getsc () == CHAR_CLOSE_PARATHENSIS)
		    {
			*(wp)++ = WORD_ODQUOTE;
			c = 0;
			goto Done;
		    }

		    else
			ungetsc ();
		}

		goto Subst;

	    case SDDPAREN:			/* Include $((....))	*/

/* Check for sub-expressions */

		if (c == CHAR_OPEN_PARATHENSIS)
		    SDD_Count++;

/* Check for end of sub-expression or $((..)) */

		else if (c == CHAR_CLOSE_PARATHENSIS)
		{
		    if (SDD_Count)
			SDD_Count--;

		    else if (getsc () == CHAR_CLOSE_PARATHENSIS)
		    {
			state = *--statep;
			*(wp)++ = 0;
			break;
		    }

		    else
		    {
			CompilingError ();
			ShellErrorMessage ("maths sub-expression not closed");
		    }
		}

/* Normal character? */

		*(wp)++ = (char)c;
		break;
	}
    }

/* Finally, the end! */

Done:
    XCheck (&ws, &wp);

    if (state != istate)
    {
	CompilingError ();
	ShellErrorMessage ("no closing quote");
    }

    if (!(LexicalControlFlags & TEST_EXPRESSION) &&
	 ((c == CHAR_INPUT) || (c == CHAR_OUTPUT)))
    {
	unsigned char *cp = XStart (ws);

/* Set c2 to the io unit value */

	if ((wp > cp) && (cp[0] == WORD_CHAR) && (IS_Numeric (cp[1])))
	{
	    wp = cp; /* throw away word */
	    c2 = cp[1] - '0';
	}

	else
	    c2 = (c == CHAR_OUTPUT) ? 1 : 0;	/* 0 for <, 1 for > */
    }

/* no word, process LEX1 character */

    if ((wp == XStart (ws)) && (state == SBASE))
    {
	XFree (ws);	/* free word */
	
	switch (c)
	{
	    default:
		return c;

    /* Check for double character thingys - ||, &&, ;; */

	    case CHAR_PIPE:
	    case CHAR_ASYNC:
	    case CHAR_SEPARATOR:
		if ((c2 = getsc ()) == c)
		{
		    switch (c)
		    {
			case CHAR_PIPE:
			    c = PARSE_LOGICAL_OR;
			    break;

			case CHAR_ASYNC:
			    c = PARSE_LOGICAL_AND;
			    break;

			case CHAR_SEPARATOR:
			    c = PARSE_BREAK;
			    break;

			default:
			    c = YYERRCODE;
			    break;
		    }
		}

/* Check for co-process |& */

		else if ((c == CHAR_PIPE) && (c2 == CHAR_ASYNC))
		    c = PARSE_COPROCESS;

		else
		    ungetsc ();

		return c;

    /* Re-direction */

	    case CHAR_INPUT:
	    case CHAR_OUTPUT:
	    {
		IO_Actions	*iop;

		iop = (IO_Actions *) GetAllocatedSpace (sizeof (IO_Actions));
		iop->io_unit = c2/*unit*/;

    /* Look at the next character */

		c2 = getsc ();

    /* Check for >>, << & <> */

		if ((c2 == CHAR_OUTPUT) || (c2 == CHAR_INPUT) )
		{
		    iop->io_flag = (c != c2) ? IORDWR
					     : (c == CHAR_OUTPUT) ? IOCAT
								  : IOHERE;
		    c2 = getsc ();
		}

		else
		    iop->io_flag = (c == CHAR_OUTPUT) ? IOWRITE : IOREAD;

    /* Check for <<- - strip tabs */

		if (iop->io_flag == IOHERE)
		{
		    if (c2 == '-')
			iop->io_flag |= IOSKIP;

		    else
			ungetsc ();
		}

    /* Check for <& or >& */

		else if (c2 == '&')
		    iop->io_flag = IODUP;

    /* Check for >! */

		else if ((c2 == CHAR_PIPE) && (iop->io_flag == IOWRITE))
		    iop->io_flag |= IOCLOBBER;

		else
		    ungetsc ();

		yylval.iop = iop;
		return PARSE_REDIR;
	    }

	    case CHAR_NEW_LINE:
		GetHereDocuments ();

		if (LexicalControlFlags & ALLOW_CONTINUATION)
		    goto Again;

		return c;

	    case CHAR_OPEN_PARATHENSIS:
		c2 = getsc();

		if (c2 == CHAR_CLOSE_PARATHENSIS)
		    c = PARSE_MPAREN;

		else if (c2 == CHAR_OPEN_PARATHENSIS)
		    c = PARSE_MDPAREN;

		else
		    ungetsc();

	    case CHAR_CLOSE_PARATHENSIS:
		return c;
	}
    }

/* Must be a word !!! */

    *(wp)++ = WORD_EOS;		/* terminate word */
    yylval.cp = XClose (&ws, wp);

    if (state == SWORD || state == SDPAREN)	/* ONEWORD? */
	return PARSE_WORD;

    ungetsc ();		/* unget terminator */

/* Make sure the CurrentLexIdentifier array stays '\0' padded */

    memset (CurrentLexIdentifier, 0, IDENT);

/* copy word to unprefixed string CurrentLexIdentifier */

    for (sp = yylval.cp, dp = CurrentLexIdentifier;
	 (dp < CurrentLexIdentifier + IDENT) && ((c = *sp++) == WORD_CHAR); )
	    *dp++ = *sp++;

    if (c != WORD_EOS)
	*CurrentLexIdentifier = 0;	/* word is not unquoted */

/* Are we allowing Keywords and Aliases ? */

    if (*CurrentLexIdentifier != 0 &&
	(LexicalControlFlags & (ALLOW_KEYWORD | ALLOW_ALIAS)))
    {
	AliasList	*p;
	int		yval;
	Source		*s;

/* Check keyword */

	if ((LexicalControlFlags & ALLOW_KEYWORD) &&
	    (yval = LookUpSymbol (CurrentLexIdentifier)))
	{
	    ReleaseMemoryCell (yylval.cp);
	    return yval;
	}

/* Check Alias */

	if ((LexicalControlFlags & ALLOW_ALIAS) &&
	    ((p = LookUpAlias (CurrentLexIdentifier, TRUE)) !=
		 (AliasList *)NULL) &&
	    (!(p->AFlags & ALIAS_EXPANDING)))
	{
	    if (RecursiveAliasCount == MAX_RECURSIVEALIASES)
	    {
		CompilingError ();
		ShellErrorMessage ("excessive recusrsive aliasing");
	    }

	    else
		RecursiveAlias[RecursiveAliasCount++] = p;

	    p->AFlags |= ALIAS_EXPANDING;

/* check for recursive aliasing */

	    for (s = source; s->type == SALIAS; s = s->next)
	    {
		if (s->u.Calias == p)
		    return PARSE_WORD;
	    }

	    ReleaseMemoryCell ((void *)yylval.cp);

/* push alias expansion */

	    s = pushs (SALIAS);
	    s->str = p->value;
	    s->u.Calias = p;
	    s->next = source;
	    source = s;
	    goto Again;
	}
    }

    return PARSE_WORD;
}

/*
 * read "<<word" text into temp file
 */

static void F_LOCAL ReadHereDocument (struct ioword *iop)
{
    FILE	*FP;
    int		tf;
    int		c;
    char	*EoFString;
    char	*cp;
    char	*Line;
    size_t	LineLen;

/* Expand the terminator */

    LineLen = strlen (EoFString = ExpandAString (iop->io_name, 0));

/* Save the tempoary file information */

    iop->io_name = StringCopy (GenerateTemporaryFileName ());

/* Create the tempoary file */

    if (((tf = S_open (FALSE, iop->io_name, O_CMASK | O_NOINHERIT)) < 0)
	|| ((FP = ReOpenFile (tf, sOpenWriteBinaryMode)) == (FILE *)NULL))
	ShellErrorMessage ("Cannot create temporary file");

/* Get some space */

    Line = GetAllocatedSpace (LineLen + 3);

/* Read the file */

    while (TRUE)
    {
	cp = Line;

/* Read the first n characters to check for terminator */

	while ((c = getsc ()) != CHAR_NEW_LINE)
	{
	    if (c == 0)
		ShellErrorMessage (LIT_Unclosed, EoFString);

	    if (cp > Line + LineLen)
		break;

/* Ignore leading tabs if appropriate */

	    if (!((iop->io_flag & IOSKIP) && (cp == Line) && (c == CHAR_TAB)))
		*(cp++) = (char)c;
	}

	*cp = 0;
	ungetsc ();

/* Check for end of string */

	if (strcmp (EoFString, Line) == 0 || c == 0)
	    break;

/* Dump the line */

	fputs (Line, FP);

	while ((c = getsc ()) != CHAR_NEW_LINE)
	{
	    if (c == 0)
		ShellErrorMessage (LIT_Unclosed, EoFString);

	    putc (c, FP);
	}

	putc (c, FP);
    }

    S_fclose (FP, TRUE);
}

/*
 * Handle scanning error
 */

void	CompilingError (void)
{
    yynerrs++;

    while (source->type == SALIAS) /* pop aliases */
	source = source->next;

    source->str = NULLSTR;	/* zap pending input */
}

/*
 * input for ScanNextToken with alias expansion
 */

Source *pushs (int type)
{
    Source *s = (Source *) GetAllocatedSpace (sizeof (Source));

    s->type = type;
    s->str = NULLSTR;
    return s;
}


/*
 * Get the next string from input source
 */

static int F_LOCAL getsc_ (void)
{
    Source	*s = source;
    int		c;

    while ((c = *s->str++) == 0)
    {
	s->str = NULL;		/* return 0 for EOF by default */

	switch (s->type)
	{
	    case SEOF:
		s->str = NULLSTR;
		return 0;

	    case STTY:
		s->line++;
		s->str = ConsoleLineBuffer;
		s->str[c = GetConsoleInput ()] = 0;

		FlushStreams ();

/* EOF? */
		if (c == 0)
		{
		    s->str = NULL;
		    s->line--;
		}

/* Ignore pre-white space */

		else
		{
		    c = 0;

		    while (s->str[c] && IS_IFS ((int)s->str[c]))
			c++;

/* Blank line?? */

		    if (!s->str[c])
		    {
			s->str = &s->str[c - 1];
			s->line--;
		    }

		    else
			s->str = &s->str[c];
		}

		break;

	    case SFILE:
		s->line++;
		s->str = fgets (e.line, LINE_MAX, s->u.file);

		DPRINT (1, ("getsc_: File line = <%s>", s->str));

		if (s->str == NULL)
		{
		    if (s->u.file != stdin)
			S_fclose (s->u.file, TRUE);
		}

		break;

	    case SWSTR:
		break;

	    case SSTRING:
		s->str = LIT_NewLine;
		s->type = SEOF;
		break;

	    case SWORDS:
		s->str = *s->u.strv++;
		s->type = SWORDSEP;
		break;

	    case SWORDSEP:
		if (*s->u.strv == NULL)
		{
		    s->str = LIT_NewLine;
		    s->type = SEOF;
		}

		else
		{
		    s->str = " ";
		    s->type = SWORDS;
		}

		break;

	    case SALIAS:
		s->str = s->u.Calias->value;

/*
 * If there is a trailing ' ', allow multiple aliases
 */

		if ((*(s->str) != 0) &&
		    (((c = s->str[strlen (s->str) - 1]) == CHAR_SPACE) ||
		     (c == CHAR_TAB)))
		    AllowMultipleAliases = TRUE;

		source = s = s->next;
		expanding_alias = TRUE;
		continue;
	}

	if (s->str == (char *)NULL)
	{
	    s->type = SEOF;
	    s->str = NULLSTR;
	    return 0;
	}

	if (s->echo)
	    foputs (s->str);
    }

    return c;
}

/*
 * Close all file handlers
 */

void CloseAllHandlers (void)
{
    int		u;

/*
 * Close any stream IO blocks
 */

    for (u = 0; u < MaxNumberofFDs; u++)
    {
#if  defined (__TURBOC__)
	if ((_streams[u].flags & _F_RDWR) && (_streams[u].fd >= NUFILE))
	    fclose (&_streams[u]);
#elif  defined (__WATCOMC__)
	if ((u < _NFILES) && (__iob[u]._flag & (_READ | _WRITE)) &&
	    (__iob[u]._handle >= NUFILE))
	    fclose (&__iob[u]);
#elif (OS_TYPE == OS_UNIX)
#  ifdef __STDC__
	if ((__iob[u]._flag & (_IOREAD | _IORW | _IOWRT)) &&
	    (__iob[u]._file >= NUFILE))
	    fclose (&__iob[u]);
#  else
	if ((_iob[u]._flag & (_IOREAD | _IORW | _IOWRT)) &&
	    (_iob[u]._file >= NUFILE))
	    fclose (&_iob[u]);
#  endif
#elif defined (__EMX__)
	if ((_streamv[u].flags & (_IOREAD | _IORW | _IOWRT)) &&
	    (_streamv[u].handle >= NUFILE))
	    fclose (&_streamv[u]);
#elif !defined (__OS2__)
	if ((_iob[u]._flag & (_IOREAD | _IORW | _IOWRT)) &&
	    (_iob[u]._file >= NUFILE))
	    fclose (&_iob[u]);
#endif
    }

/* Close any open files */

    for (u = NUFILE; u < MaxNumberofFDs;)
	S_close (u++, TRUE);
}


/*
 * remap fd into Shell's fd space
 */

int ReMapIOHandler (int fd)
{
    int		i;
    int		n_io = 0;
    int		map [NUFILE];
    int		o_fd = fd;

    if (fd < FDBASE)
    {
	do
	{
	    map[n_io++] = fd;
	    fd = dup (fd);

	} while ((fd >= 0) && (fd < FDBASE));

	for (i = 0; i < n_io; i++)
	    close (map[i]);

/* Check we can map it */

	if (fd >= (32 + FDBASE))
	{
	    close (fd);
	    fd = -1;
	}
	
	else
	{
	    S_Remap (o_fd, fd);
	    S_close (o_fd, TRUE);
	}

	if (fd < 0)
	{
#ifdef DEBUG
	    struct stat		st;

	    fprintf (stderr, "\nOld Fd=%d, New=%d, Map=0x%.8lx\n", o_fd, fd,
		     e.IOMap);

	    feputs ("Ch. Device Inode  Mode  Links RDev   Size\n");

	    for (i = 0; i < MaxNumberofFDs; i++)
	    {
		if (fstat (i, &st) < 0)
		    fprintf (stderr, "%2d: cannot get status (%s)\n", i,
			     strerror (errno));

		else
		    fprintf (stderr, "%2d: 0x%.4x %5u %6o %5d 0x%.4x %ld\n", i,
			     st.st_dev, st.st_ino, st.st_mode, st.st_nlink,
			     st.st_rdev, st.st_size);
	    }
#endif
	    ShellErrorMessage ("too many files open");
	}
    }

    return fd;
}


/*
 * Generate a temporary filename
 */

char *GenerateTemporaryFileName (void)
{
    static char	tmpfile[FFNAME_MAX];
    char	*tmpdir;	/* Points to directory prefix of pipe	*/
    static int	temp_count = 0;
    char	*sep = DirectorySeparator;

/* Find out where we should put temporary files */

    if (((tmpdir = GetVariableAsString ("TMP", FALSE)) == null) &&
	((tmpdir = GetVariableAsString (HomeVariableName, FALSE)) == null) &&
	((tmpdir = GetVariableAsString ("TMPDIR", FALSE)) == null))
	tmpdir = CurrentDirLiteral;

    if (strchr ("/\\", tmpdir[strlen (tmpdir) - 1]) != (char *)NULL)
	sep = null;

/* Get a unique temporary file name */

    while (1)
    {
	sprintf (tmpfile, "%s%ssht%.5u.tmp", tmpdir, sep, temp_count++);

	if (!S_access (tmpfile, F_OK))
	    break;
    }

    return tmpfile;
}

/*
 * XString functions
 */

/*
 * Check an expandable string for overflow
 */

void	XCheck (XString *xs, unsigned char **xp)
{
    if (*xp >= xs->SEnd)
    {
	int	OldOffset = *xp - xs->SStart;

	xs->SStart = ReAllocateSpace (xs->SStart, (xs->SLength *= 2) + 8);
	xs->SEnd = xs->SStart + xs->SLength;
	*xp = xs->SStart + OldOffset;
    }
}

/*
 * Close an Expanded String
 */

char	*XClose (XString *xs, unsigned char *End)
{
    size_t	len = End - xs->SStart;
    char	*s = memcpy (GetAllocatedSpace (len), xs->SStart, len);

    ReleaseMemoryCell (xs->SStart);
    return s;
}

/*
 * Create an Expanded String
 */

char	*XCreate (XString *xs, size_t len)
{
    xs->SLength = len;
    xs->SStart = GetAllocatedSpace (len + 8);
    xs->SEnd = xs->SStart + len;
    return (char *)xs->SStart;
}

/*
 * here documents
 *
 * Save a here file's IOP for later processing (ie delete of the temp file
 * name)
 */

void SaveHereDocumentInfo (IO_Actions *iop)
{
    Here_D	*h, *lh;

    if ((h = (Here_D *) GetAllocatedSpace (sizeof(Here_D))) == (Here_D *)NULL)
	return;

    h->h_iop     = iop;
    h->h_next    = (Here_D *)NULL;

    if (HereListHead == (Here_D *)NULL)
	HereListHead = h;

    else
    {
	for (lh = HereListHead; lh != (Here_D *)NULL; lh = lh->h_next)
	{
	    if (lh->h_next == (Here_D *)NULL)
	    {
		lh->h_next = h;
		break;
	    }
	}
    }
}

/*
 * Read all the active here documents
 */

static void F_LOCAL GetHereDocuments (void)
{
    Here_D	*h, *hp;

/* Scan here files first leaving HereListHead list in place */

    for (hp = h = HereListHead; h != (Here_D *)NULL; hp = h, h = h->h_next)
	ReadHereDocument (h->h_iop);

/* Make HereListHead list active - keep list intact for scraphere */

    if (hp != (Here_D *)NULL)
    {
	hp->h_next	 = ActiveListHead;
	ActiveListHead	 = HereListHead;
	HereListHead     = (Here_D *)NULL;
    }
}

/*
 * Zap all the here documents, unless they are currently in use by a
 * function.
 */

void ScrapHereList (void)
{
    Here_D	*h;

    for (h = HereListHead; h != (Here_D *)NULL; h = h->h_next)
    {
	if ((h->h_iop != (IO_Actions *)NULL) &&
	    (h->h_iop->io_name != (char *)NULL) &&
	    (!(h->h_iop->io_flag & IOFUNCTION)))
	    unlink (h->h_iop->io_name);
    }

    HereListHead = (Here_D *)NULL;
}

/*
 * unlink here temp files before a ReleaseMemoryArea (area)
 */

void FreeAllHereDocuments (int area)
{
    Here_D	*current = ActiveListHead;
    Here_D	*previous = (Here_D *)NULL;

    while (current != (Here_D *)NULL)
    {
	if (GetMemoryAreaNumber ((void *)current) >= area)
	{
	    if ((current->h_iop->io_name != (char *)NULL) &&
		(!(current->h_iop->io_flag & IOFUNCTION)))
		unlink (current->h_iop->io_name);

	    if (ActiveListHead == current)
		ActiveListHead = current->h_next;

	    else
		previous->h_next = current->h_next;
	}

	previous = current;
	current = current->h_next;
    }
}

/*
 * Open here document temp file.
 * If unquoted here, expand here temp file into second temp file.
 */

int	OpenHereFile (char *hname, bool sub)
{
    FILE	*FP;
    char	*cp;
    Source	*s;
    char	*tname = (char *)NULL;
    jmp_buf	ReturnPoint;

/* Check input file */

    if (hname == (char *)NULL)
	return -1;

/*
 * If processing for $, ` and ' is required, do it
 */

    if (sub)
    {
	CreateNewEnvironment ();

	if (SetErrorPoint (ReturnPoint) == 0)
	{
	    if ((FP = FOpenFile (hname, sOpenReadMode)) == (FILE *)NULL)
	    {
		PrintErrorMessage ("Here Document (%s) lost", hname);
		return -1;
	    }

/* set up ScanNextToken input from here file */

	    s = pushs (SFILE);
	    s->u.file = FP;
	    source = s;

	    if (ScanNextToken (ONEWORD) != PARSE_WORD)
		PrintErrorMessage ("Here Document error");

	    cp = ExpandAString (yylval.cp, 0);

    /* write expanded input to another temp file */

	    tname = StringCopy (GenerateTemporaryFileName ());

	    if ((FP = FOpenFile (tname, sOpenAppendMode)) == (FILE *)NULL)
	    {
		ShellErrorMessage (Outofmemory1);
		TerminateCurrentEnvironment (TERMINATE_COMMAND);
	    }

	    fputs (cp, FP);
	    CloseFile (FP);

	    QuitCurrentEnvironment ();

	    return S_open (TRUE, tname, O_RDONLY);
	}

/* Error - terminate */

	else
	{
	    QuitCurrentEnvironment ();
	    CloseFile (FP);

	    if (tname != (char *)NULL)
		unlink (tname);

	    return -1;
	}
    }

/* Otherwise, just open the document */

    return S_open (FALSE, hname, O_RDONLY);
}
