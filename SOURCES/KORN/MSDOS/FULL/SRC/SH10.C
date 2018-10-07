/*
 * MS-DOS SHELL - Function Processing
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
 * Note:  1.  The PrintProcessTree code is based on code written by Kai Uwe
 *	      Rommel
 *
 *	  2.  When parts of the original 2.1 shell were replaced by the Lexical
 *	      Analsyer written by Simon J. Gerraty (for his Public Domain Korn
 *	      Shell, which is also based on Charles Forsyth original idea), a
 *	      number of changes were made to reflect the changes Simon made to
 *	      the Parse output tree.  Some parts of this code in this module
 *	      are based on the algorithms/ideas that he incorporated into his
 *	      shell, in particular the Function Processing functions.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh10.c,v 2.15 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh10.c,v $
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
 *	Revision 2.11  1993/12/01  11:58:34  istewart
 *	Release 226 beta
 *
 *	Revision 2.10  1993/08/25  16:03:57  istewart
 *	Beta 225 - see Notes file
 *
 *	Revision 2.9  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.8  1993/06/14  11:01:44  istewart
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
 *	Revision 2.2  1992/07/16  14:33:34  istewart
 *	Beta 212 Baseline
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
#include "sh.h"

#if (OS_TYPE == OS_UNIX)
#  include <sys/times.h>
#endif 

/* Function declarations */

static void F_LOCAL	PrintCommand (C_Op *, int);
static void F_LOCAL	PrintIOInformation (IO_Actions *);
static void F_LOCAL	PrintCaseCommand (C_Op *);
static void F_LOCAL	PrintIndentedString (char *, int, int);
static void F_LOCAL	PrintVarArg (unsigned char *);
static void F_LOCAL	fputMagicChar (unsigned int);
static void F_LOCAL	PrintMode (int);

static void F_LOCAL	SaveReleaseExecuteTree (C_Op *, void (*)(void *));
static void F_LOCAL	SaveReleaseWordList (char **, void (*)(void *));
static void F_LOCAL	SaveReleaseIOActions (IO_Actions **, void (*)(void *));
static void		SaveTreeEntry (void *);

static C_Op *	F_LOCAL	DuplicateFunctionTree (C_Op *);
static char **  F_LOCAL	DuplicateWordList (char **list);
static IO_Actions ** F_LOCAL DuplicateIOActions (IO_Actions **);

static int		FindFunction (const void *, const void *);
static int		SearchFunction (const void *, const void *);
static void		DisplayFunction (const void *, VISIT, int);
static void		DeleteAFunction (const void *, VISIT, int);

static int		FindAlias (const void *, const void *);
static int		SearchAlias (const void *, const void *);
static void		UntrackAlias (const void *, VISIT, int);
static void		DisplayAlias (const void *, VISIT, int);

#if (OS_TYPE != OS_DOS)
static void		DisplayJob (const void *, VISIT, int);
static void		CountJob (const void *, VISIT, int);
static int		SearchJob (const void *, const void *);
static int		FindJob (const void *, const void *);
static void		FindJobByString (const void *, VISIT, int);
static int		FindJobByPID (const void *, const void *);
static int		FindJobBySession (const void *, const void *);
#endif

#if (OS_TYPE == OS_OS2)
#  if (OS_SIZE == OS_32)
#    if !defined (__EMX__)
#      define Dos32GetPrty	DosGetPrty
#      define Dos32QProcStatus	DosQProcStatus
#      pragma linkage (DosQProcStatus, far16 pascal)
#      pragma linkage (DosGetPrty, far16 pascal)
#    else
USHORT _THUNK_FUNCTION (Dos16GetPrty) ();
USHORT _THUNK_FUNCTION (Dos16QProcStatus) ();
#    endif
extern USHORT			Dos32QProcStatus (PVOID, USHORT);
extern USHORT			Dos32GetPrty (USHORT, PUSHORT, USHORT);
#  else
#    define Dos32QProcStatus	DosQProcStatus
#    define Dos32GetPrty	DosGetPrty
extern USHORT APIENTRY		Dos32QProcStatus (PVOID, USHORT);
#  endif
#endif

/*
 * OS/2 Process Information structures
 *
 *
 * Declare OS2 1.x 16-bit version structures
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_16)
typedef struct process
{
    USHORT	pid;
    USHORT	ppid;
    USHORT	threads;
    USHORT	children;
    USHORT	modhandle;
    USHORT	module;
} V1Process_t;

typedef struct module
{
    USHORT	modhandle;
    USHORT	max_dependents;
    USHORT	*dependents;
    UCHAR	*modname;
} V1Module_t;

typedef struct ProcessInfo
{
    V1Process_t		**V1Processes;
    V1Module_t		**V1Modules;
    USHORT		M_Processes;
    USHORT		N_Processes;
    USHORT		M_Modules;
    USHORT		N_Modules;
} V1ProcessStatus_t;
#endif

/*
 * OS2 2.0 - 32 Bit version structures
 */

#if (OS_TYPE == OS_OS2)
#  if (OS_SIZE == OS_32)
#    define PTR(ptr)	(ptr)
#  else
#    define PTR(ptr)	((void *)((((ULONG)ps)     & 0xFFFF0000L) |	\
				  (((ULONG)(ptr))  & 0x0000FFFFL) ))
#  endif

#  define PROCESS_END_INDICATOR	3

/* Process Status structures */

#  if (OS_SIZE == OS_32)
#    pragma pack(1)
#  endif

/*
 * Thread Info
 */

typedef struct thread2
{
    ULONG	ulRecType;		/* Record type (thread = 100)	*/
    USHORT	tidWithinProcess;	/* TID within process (TID is	*/
    					/* 4 bytes!!)			*/
    USHORT	usSlot;			/* Unique thread slot number	*/
    ULONG	ulBlockId;		/* Sleep id thread is sleeping on*/
    ULONG	ulPriority;		/* Priority			*/
    ULONG	ulSysTime;		/* Thread System Time		*/
    ULONG	ulUserTime;		/* Thread User Time		*/
    UCHAR	uchState;		/* 1=ready,2=blocked,5=running	*/
    UCHAR	uchPad;			/* Filler			*/
    USHORT	usPad;			/* Filler			*/
} V2Thread_t;

/*
 * Process Information
 */

typedef struct process2
{
    ULONG	ulEndIndicator;		/* 1 means not end, 3 means	*/
    					/* last entry			*/
    V2Thread_t	*ptiFirst;		/* Address of the 1st Thread	*/
    					/* Control Blk			*/
    USHORT	pid;			/* Process ID (2 bytes - PID	*/
    					/* is 4 bytes)			*/
    USHORT	pidParent;		/* Parent's process ID		*/
    ULONG	ulType;			/* Process Type			*/
    ULONG	ulStatus;		/* Process Status		*/
    ULONG	idSession;		/* Session ID			*/
    USHORT	hModRef;		/* Module handle of EXE		*/
    USHORT	usThreadCount;		/* Number of threads in this	*/
    					/* process			*/
    ULONG	ulSessionType;		/* Session Type			*/
    PVOID	pvReserved;		/* Unknown			*/
    USHORT	usSem16Count;		/* Number of 16-bit system	*/
    					/* semaphores			*/
    USHORT	usDllCount;		/* Number of Dlls used by	*/
    					/* process			*/
    USHORT	usShrMemHandles;	/* Number of shared memory	*/
    					/* handles			*/
    USHORT	usReserved;		/* Unknown			*/
    PUSHORT	pusSem16TableAddr;	/* Address of a 16-bit semaphore*/
    					/* table			*/
    PUSHORT	pusDllTableAddr;	/* Address of a Dll table	*/
    PUSHORT	pusShrMemTableAddr;	/* Address of a shared memory	*/
    					/* table			*/
} V2Process_t;

/*
 * Process Status header
 */

typedef struct processstatus2
{
    PVOID	psumm;		/* SUMMARY section ptr			*/
    V2Process_t	*ppi;		/* PROCESS section ptr			*/
    PVOID	psi;		/* SEM section ptr (add 16 to offset)	*/
    PVOID	pDontKnow1;	/*					*/
    PVOID	psmi;		/* SHARED MEMORY section ptr		*/
    PVOID	pmi;		/* MODULE section ptr			*/
    PVOID	pDontKnow2;	/*					*/
    PVOID	pDontKnow3;	/*					*/
} V2ProcessStatus_t;

#  if (OS_SIZE == OS_32)
#    pragma pack()
#  endif
#endif

/*
 * Associated functions
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_16)
static int			SortV1Processes (void *, void *);
static void F_LOCAL		PrintV1ProcessTree (pid_t, int,
						    V1ProcessStatus_t *);
static bool F_LOCAL		Parse_V1ProcessTable (UCHAR *,
						      V1ProcessStatus_t *);
#endif

#if (OS_TYPE == OS_OS2)
static void F_LOCAL		V2_DisplayProcessTree (USHORT, USHORT,
						       V2ProcessStatus_t *);
static V2ProcessStatus_t * F_LOCAL GetProcessStatus (char *);
#endif

#if (OS_TYPE != OS_DOS)
static char	*JobSearchKey;		/* Job search string		*/
static int	NumberOfJobs = 0;	/* Number of Jobs		*/
static JobList	**JobSearchEntry;	/* Matching entry		*/
#endif

static bool	DisplayListMode = FALSE;/* Mode for Display Job/Alias	*/
static int	Print_indent;		/* Current indent level		*/

					/* IO types			*/
static char	*IOTypes [] = { "<", ">", "<>", "<<", ">>", ">&" };

/*
 * Duplicate a memory area
 */

#define DuplicateMemoryArea(area, type)					\
		(type)((area == (void *)NULL)				\
		    ? (void *)NULL					\
		    : DuplicateMemoryCell (area))

/*
 * Print ALL functions
 */

int PrintAllFunctions (void)
{
    twalk (FunctionTree, DisplayFunction);
    return 0;
}

/*
 * TWALK function to display the JOB, FUNCTION and ALIAS trees
 */

#if (OS_TYPE != OS_DOS)
static void CountJob (const void *key, VISIT visit, int level)
{
    if ((visit == postorder) || (visit == leaf))
	NumberOfJobs++;
}

static void DisplayJob (const void *key, VISIT visit, int level)
{
    if ((visit == postorder) || (visit == leaf))
    {
	printf ("[%d] %c", (*(JobList **)key)->Number,
		((*(JobList **)key)->Number == CurrentJob)
			? CHAR_PLUS
			: (((*(JobList **)key)->Number == PreviousJob)
			    ? CHAR_HYPHEN : CHAR_SPACE));

	if (DisplayListMode)
	    printf (" %d", (*(JobList **)key)->pid);

	fputchar (CHAR_TAB);
	FlushStreams ();
	DisplayLineWithControl ((*(JobList **)key)->Command);
	fputchar (CHAR_NEW_LINE);
    }
}
#endif

static void DisplayFunction (const void *key, VISIT visit, int level)
{
    if ((visit == postorder) || (visit == leaf))
        PrintFunction ((*(FunctionList **)key)->tree, PF_MODE_NORMAL);

}

static void DisplayAlias (const void *key, VISIT visit, int level)
{
    if (((visit == postorder) || (visit == leaf)) &&
	(!DisplayListMode || (*(AliasList **)key)->AFlags & ALIAS_TRACKED))
	PrintAlias ((*(AliasList **)key)->name);
}


/*
 * DISPLAY A FUNCTION TREE
 */

/*
 * print the execute tree - used for displaying functions
 */

void PrintFunction (register C_Op *t, int mode)
{
    char		**wp;

    if (t == (C_Op *)NULL)
	return;

/* Check for start of print */

    if (t->type == TFUNC)
    {
	Print_indent = 0;
	printf (LIT_2Strings, t->str, "()");
	PrintFunction (t->left, PF_MODE_NORMAL);
	FlushStreams ();
	return;
    }

/* Otherwise, process the tree and print it */

    switch (t->type)
    {
	case TASYNC:			/* Asyn commands		*/
	    PrintFunction (t->left, PF_MODE_ASYNC);
	    return;

	case TCOPROCESS:		/* Co-process			*/
	    PrintFunction (t->left, PF_MODE_COPROC);
	    return;

	case TPAREN:			/* ()				*/
	case TCOM:			/* A command process		*/
	    PrintCommand (t, mode);
	    return;

	case TPIPE:			/* Pipe processing		*/
	    PrintFunction (t->left, PF_MODE_NORMAL);
	    PrintIndentedString ("|\n", 0, PF_MODE_NORMAL);
	    PrintFunction (t->right, mode);
	    return;

	case TLIST:			/* Entries in a for statement	*/
	    PrintFunction (t->left, PF_MODE_NORMAL);
	    PrintFunction (t->right, mode);
	    return;

	case TOR:			/* || and &&			*/
	case TAND:
	    PrintFunction (t->left, PF_MODE_NORMAL);

	    if (t->right != (C_Op *)NULL)
	    {
		PrintIndentedString ((t->type == TAND) ? "&&\n" : "||\n",
				     0, PF_MODE_NORMAL);
		PrintFunction (t->right, mode);
	    }

	    return;

	case TFOR:			/* First part of a for statement*/
	case TSELECT:
	    PrintIndentedString ((t->type == TFOR) ? "for " : "select ",
				 0, PF_MODE_NORMAL);
	    foputs (t->str);

	    if ((wp = t->vars) != NOWORDS)
	    {
		foputs (" in");

		while (*wp != NOWORD)
		{
		    fputchar (CHAR_SPACE);
		    PrintVarArg ((unsigned char *)*(wp++));
		}
	    }

	    fputchar (CHAR_NEW_LINE);
	    PrintIndentedString ("do\n", 1, PF_MODE_NORMAL);
	    PrintFunction (t->left, PF_MODE_NORMAL);
	    PrintIndentedString (LIT_done, -1, mode);
	    return;

	case TWHILE:			/* WHILE and UNTIL functions	*/
	case TUNTIL:
	    PrintIndentedString ((t->type == TWHILE) ? "while\n" : "until\n",
				 1, PF_MODE_NORMAL);
	    PrintFunction (t->left, PF_MODE_NORMAL);
	    Print_indent--;
	    PrintIndentedString ("do\n", 1, PF_MODE_NORMAL);
	    PrintFunction (t->right, PF_MODE_NORMAL);
	    PrintIndentedString (LIT_done, -1, mode);
	    return;

	case TIF:			/* IF and ELSE IF functions	*/
	case TELIF:
	    if (t->type == TIF)
		PrintIndentedString ("if\n", 1, PF_MODE_NORMAL);

	    else
		PrintIndentedString ("elif\n", 1, PF_MODE_NORMAL);

	    PrintFunction (t->left, PF_MODE_NORMAL);

	    if (t->right != (C_Op *)NULL)
	    {
		Print_indent -= 1;
		PrintIndentedString ("then\n", 1, PF_MODE_NORMAL);
		PrintFunction (t->right->left, PF_MODE_NORMAL);

		if (t->right->right != (C_Op *)NULL)
		{
		    Print_indent -= 1;

		    if (t->right->right->type != TELIF)
			PrintIndentedString ("else\n", 1, PF_MODE_NORMAL);

		    PrintFunction (t->right->right, PF_MODE_NORMAL);
		}
	    }

	    if (t->type == TIF)
		PrintIndentedString ("fi", -1, mode);

	    return;

	case TCASE:			/* CASE function		*/
	    PrintIndentedString ("case ", 1, PF_MODE_NORMAL);
	    PrintVarArg ((unsigned char *)t->str);
	    puts (" in");
	    PrintCaseCommand (t->left);
	    PrintIndentedString ("esac", -1, mode);
	    return;

	case TBRACE:			/* {} statement			*/
	    PrintIndentedString ("{\n", 1, PF_MODE_NORMAL);
	    if (t->left != (C_Op *)NULL)
		PrintFunction (t->left, mode);

	    PrintIndentedString ("}", -1, PF_MODE_NORMAL);
	    return;

	case TTIME:
	    PrintIndentedString ("time\n", 1, PF_MODE_NORMAL);
	    PrintFunction (t->left, mode);
	    Print_indent--;
	    return;
    }
}


/*
 * Print a command line
 */

static void F_LOCAL PrintCommand (register C_Op *t, int mode)
{
    IO_Actions	**iopp;
    char	**wp;
    int		i;

/* Parenthesis ? */

    if (t->type == TPAREN)
    {
	PrintIndentedString ("(\n", 1, PF_MODE_NORMAL);
	PrintFunction (t->left, PF_MODE_NORMAL);
	PrintIndentedString (")", -1, PF_MODE_NO);

    }

    else
    {
	PrintIndentedString (null, 0, PF_MODE_NORMAL);

 /* Process arguments and assigments */

	for (i = 0, wp = t->vars; i < 2; i++)
	{
	    if (wp != NOWORDS)
	    {
		while (*wp != NOWORD)
		{
		    PrintVarArg ((unsigned char *)*(wp++));

		    if (*wp != NOWORD)
			fputchar (CHAR_SPACE);
		}
	    }

	    wp = t->args;
	}
    }

/* Set up anyother IO required */

    if ((iopp = t->ioact) != (IO_Actions **)NULL)
    {
	while (*iopp != (IO_Actions *)NULL)
	    PrintIOInformation (*(iopp++));
    }

    PrintMode (mode);
}

/*
 * Print an argument or variable assigment string
 */

static void F_LOCAL PrintVarArg (unsigned char *string)
{
    register unsigned 	c;
    register bool	quoted = FALSE;

    while (1)
    {
	switch ((c = *(string++)))
	{
	    case WORD_EOS:	/* end of string			*/
		return;

	    case WORD_CHAR:	/* unquoted character			*/
		fputMagicChar (*(string++));
		break;

	    case WORD_QCHAR:	/* quoted character			*/
	    case WORD_QTCHAR:
		if (!quoted)
		    fputchar (CHAR_META);

		fputMagicChar (*(string++));
		break;

	    case WORD_OQUOTE:	/* opening " or '			*/
	    case WORD_ODQUOTE:
		quoted = TRUE;
		fputchar ((c == WORD_OQUOTE) ? CHAR_SINGLE_QUOTE
					     : CHAR_DOUBLE_QUOTE);
		break;

	    case WORD_CQUOTE:	/* closing " or '			*/
	    case WORD_CDQUOTE:
		quoted = FALSE;
		fputchar ((c == WORD_CQUOTE) ? CHAR_SINGLE_QUOTE
					     : CHAR_DOUBLE_QUOTE);
		break;

	    case WORD_OARRAY:	/* Opening ${...[...] ...		*/
		fputchar (CHAR_OPEN_BRACKETS);
		break;

	    case WORD_CARRAY:	/* Closing ${...[...] ...		*/
	    case WORD_OSUBST:	/* opening ${ substitution		*/
		if (c == WORD_CARRAY)
		    fputchar (CHAR_CLOSE_BRACKETS);

/* Start of variable - output the name */

		else
		{
		    fputchar (CHAR_VARIABLE);
		    fputchar (CHAR_OPEN_BRACES);

		    while ((c = *(string++)) != 0)
			fputchar (c);
		}

/* Check for some special characters */

		if ((*string != WORD_CSUBST) && (*string != WORD_OARRAY))
		{
		    if (((c = *(string++)) & CHAR_MAGIC) &&
			(IS_VarOp (c & 0x7f)))
		    {

/* Check for %% or ## */
			if (((c &= 0x7f) == CHAR_MATCH_START) ||
			    (c == CHAR_MATCH_END))
			    fputMagicChar (c);

/* :?string case */
			else
			    fputMagicChar (':');

		    }

		    fputMagicChar (c);
		}

		break;

	    case WORD_CSUBST:	/* closing } of above			*/
		fputchar (CHAR_CLOSE_BRACES);
		break;

	    case WORD_COMSUB:	/* $() substitution (0 terminated)	*/
	    case WORD_OMATHS:	/* opening $(()) substitution (0 term)	*/
		fputchar (CHAR_VARIABLE);
		fputchar (CHAR_OPEN_PARATHENSIS);

		if (c == WORD_OMATHS)
		    fputchar (CHAR_OPEN_PARATHENSIS);

		while (*string != 0)
		    fputMagicChar (*(string++));

		string++;		/* Skip over the terminator	*/
		fputchar (CHAR_CLOSE_PARATHENSIS);

		if (c == WORD_OMATHS)
		    fputchar (CHAR_CLOSE_PARATHENSIS);

		break;
	}
    }
}


/*
 * Output a potentially magic character
 */

static void F_LOCAL fputMagicChar (unsigned int c)
{
    if ((c & 0x60) == 0)
    {
	fputchar ((c & CHAR_MAGIC) ? CHAR_VARIABLE : '^');
	fputchar (((c & 0x7F) | 0x40));
    }

    else if ((c&0x7F) == 0x7F)
    {
	fputchar ((c & CHAR_MAGIC) ? CHAR_VARIABLE : '^');
	fputchar ('?');
    }

    else
	fputchar (c);
}


/*
 * Print the IO re-direction
 */

static void F_LOCAL PrintIOInformation (register IO_Actions *iop)
{
    int		unit = iop->io_unit;
    int		IOflag = iop->io_flag & IOTYPE;
    char	*type;
    bool	TermQuote = FALSE;

/* Set up the IO type string and display it */

    type = IOTypes [IOflag - 1];

    if ((IOflag == IODUP) && (!unit))
	type = "<&";

    printf (" %d%s", unit, type);

/* Print clobber override and skip tabs information on here documents */

    if (iop->io_flag & IOCLOBBER)
	fputchar (CHAR_PIPE);

    if (iop->io_flag & IOSKIP)
	fputchar (CHAR_HYPHEN);

/* Quoted here document ? */

    if ((TermQuote = C2bool ((IOflag == IOHERE) && (!(iop->io_flag & IOEVAL)))))
	fputchar (CHAR_SINGLE_QUOTE);

/* Here document names are in expanded format */

    (IOflag == IOHERE) ? (void)foputs (iop->io_name)
		       : (void)PrintVarArg ((unsigned char *)iop->io_name);

    if (TermQuote)
	fputchar (CHAR_SINGLE_QUOTE);
}

/*
 * Print out the contents of a case statement
 */

static void F_LOCAL PrintCaseCommand (C_Op *t)
{
    register char	**wp;

    while (t != (C_Op *)NULL)
    {

/* Print patterns (the conditions) */

	PrintIndentedString (null, 0, PF_MODE_NORMAL);

	for (wp = t->vars; *wp != NOWORD; )
	{
	    PrintVarArg ((unsigned char *)*(wp++));

	    if (*wp != NOWORD)
		foputs (" | ");
	}

	puts (" )");
	Print_indent += 1;

/* print the functions */

	PrintFunction (t->left, PF_MODE_NORMAL);
	PrintIndentedString (";;", -1, PF_MODE_NORMAL);
	t = t->right;
    }
}

/*
 * Print an indented string
 */

static void F_LOCAL PrintIndentedString (char *cp, int indent, int mode)
{
    int		i;

    if (indent < 0)
	Print_indent += indent;

    for (i = 0; i < (Print_indent / 2); i++)
	fputchar (CHAR_TAB);

    if (Print_indent % 2)
	foputs ("    ");

    foputs (cp);

/* Append the mode */

    if ((mode != PF_MODE_NORMAL) || (indent < 0))
	PrintMode (mode);

    if (indent > 0)
	Print_indent += indent;
}

/*
 * Output mode
 */

static void F_LOCAL PrintMode (int mode)
{
    if (mode == PF_MODE_ASYNC)
	foputs (" &\n");
    
    else if (mode == PF_MODE_COPROC)
	foputs (" |&\n");

    else if (mode == PF_MODE_NORMAL)
	fputchar (CHAR_NEW_LINE);
}
    
/*
 * TWALK and TDELETE compare functions for FUNCTION, ALIAS and JOB trees
 *
 * Note: We know about these two function, so we know the key is always
 *       the first parameter.  So we only pass the char * not the FunctionList
 *       pointer (in the case of JOB, its an int *)
 */

static int FindFunction (const void *key1, const void *key2)
{
    return strcmp (key1, ((FunctionList *)key2)->tree->str);
}

static int FindAlias (const void *key1, const void *key2)
{
    return strcmp (key1, ((AliasList *)key2)->name);
}

/* By string name */

#if (OS_TYPE != OS_DOS)
static void FindJobByString (const void *key, VISIT visit, int level)
{
    if (JobSearchEntry != (JobList **)NULL)
	return;

    if (((visit == postorder) || (visit == leaf)) &&
	(((*JobSearchKey == CHAR_MATCH_ANY) &&
	  (strstr (((JobList *)key)->Command, JobSearchKey) != (char *)NULL)) ||
	 ((*JobSearchKey != CHAR_MATCH_ANY) &&
	  (strncmp (JobSearchKey, ((JobList *)key)->Command,
		   strlen (JobSearchKey)) == 0))))
	JobSearchEntry = (JobList **)&key;
}

/* By process id */

static int FindJobByPID (const void *key1, const void *key2)
{
    return *(PID *)key1 - ((JobList *)key2)->pid;
}

/* By session id */

static int FindJobBySession (const void *key1, const void *key2)
{
    return *(unsigned short *)key1 - ((JobList *)key2)->SessionId;
}

/* By job number */

static int FindJob (const void *key1, const void *key2)
{
    return *(int *)key1 - ((JobList *)key2)->Number;
}
#endif

/*
 * Look up a function in the save tree
 */

FunctionList *LookUpFunction (char *name, bool AllowDot)
{
    FunctionList	**fp = (FunctionList **)NULL;
    
    if (AllowDot || (*name != '.'))
	fp = (FunctionList **)tfind (name, &FunctionTree, FindFunction);

    return fp != (FunctionList **)NULL ? *fp : (FunctionList *)NULL;
}


/*
 * TSEARCH compare functions for FUNCTION, ALIAS and JOB trees
 */

static int SearchFunction (const void *key1, const void *key2)
{
    return strcmp (((FunctionList *)key1)->tree->str,
		   ((FunctionList *)key2)->tree->str);
}

static int SearchAlias (const void *key1, const void *key2)
{
    return strcmp (((AliasList *)key1)->name, ((AliasList *)key2)->name);
}

#if (OS_TYPE != OS_DOS)
static int SearchJob (const void *key1, const void *key2)
{
    return ((JobList *)key1)->Number - ((JobList *)key2)->Number;
}
#endif

/*
 * SAVE/DELETE A FUNCTION
 */

/*
 * Save a function tree
 */

bool SaveFunction (C_Op *t)
{
    char			*name = t->str;
    register FunctionList	*fp;
    void			(*save_signal)(int);
    char			*sname = name;

/* Allow dot as the first character */

    if ((*name == '.') && (strlen (name) > (size_t)1))
	sname++;

    if (!IsValidAliasName (sname, FALSE))
    {
	PrintWarningMessage (BasicErrorMessage, name, LIT_Invalidfunction);
	return FALSE;
    }

/* Create new entry */

    if ((fp = (FunctionList *)GetAllocatedSpace (sizeof (FunctionList)))
	== (FunctionList *)NULL)
	return FALSE;

/* Delete the old function if it exists */

    DeleteFunction (t);

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);
    fp->tree = t;
    fp->Traced = FALSE;

/* Set up the tree */

    if (tsearch (fp, &FunctionTree, SearchFunction) != (void *)NULL)
    {
	SetMemoryAreaNumber ((void *)fp, 0);
	SaveReleaseExecuteTree (t, SaveTreeEntry);
    }

/* Restore signals */

    signal (SIGINT, save_signal);
    return TRUE;
}

/*
 * Clean up functions on exit.  We should just delete the files, but its
 * easier to delete the functions
 */

void DeleteAllFunctions (void)
{
    void			(*save_signal)(int);

    save_signal = signal (SIGINT, SIG_IGN);
    twalk (FunctionTree, DeleteAFunction);
    signal (SIGINT, save_signal);
}

/*
 * Associate TREE WALK function to delete all functions
 */

static void DeleteAFunction (const void *key, VISIT visit, int level)
{
    if ((visit == postorder) || (visit == leaf))
	SaveReleaseExecuteTree ((*(FunctionList **)key)->tree,
			        ReleaseMemoryCell);
}

/*
 * Delete a function tree
 */

void DeleteFunction (C_Op *t)
{
    char			*name = t->str;
    register FunctionList	*fp = LookUpFunction (name, TRUE);
    void			(*save_signal)(int);

    if (fp == (FunctionList *)NULL)
	return;

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Free the tree and delete the entry */

    tdelete (name, &FunctionTree, FindFunction);
    SaveReleaseExecuteTree (fp->tree, ReleaseMemoryCell);
    ReleaseMemoryCell ((void *)fp);

/* Restore signals */

    signal (SIGINT, save_signal);
}


/*
 * Set ExTree areas to zero function
 */

static void  SaveTreeEntry (void *s)
{
    SetMemoryAreaNumber (s, 0);
}

/*
 * Set/Free function tree area by recursively processing of tree
 */

static void F_LOCAL SaveReleaseExecuteTree (C_Op *t, void (* func)(void *))
{
    if (t == (C_Op *)NULL)
	return;

/*
 * Process the tree - saving or deleting
 */

    if (t->str != (char *)NULL)
	(*func)((void *)t->str);

    SaveReleaseWordList (t->vars, func);
    SaveReleaseWordList (t->args, func);
    SaveReleaseIOActions (t->ioact, func);
    SaveReleaseExecuteTree (t->left, func);
    SaveReleaseExecuteTree (t->right, func);

    (*func)((void *)t);
}


/*
 * Save/Release a Block of words
 */

static void F_LOCAL SaveReleaseWordList (char **list, void (* func)(void *))
{
    char	**Slist = list;

    if (list == NOWORDS)
        return;

/* Ok - save/release it */

    while (*list != NOWORD)
	(*func)((void *)*(list++));

/* Handle the block itself */

    (*func)((void *)Slist);
}


/*
 * Save/Release the IO Actions block
 */

static void F_LOCAL SaveReleaseIOActions (IO_Actions **list, void (* func)(void *))
{
    IO_Actions	**Slist = list;

    if (list == (IO_Actions **)NULL)
        return;

/* Ok - save/release it */

    while (*list != (IO_Actions *)NULL)
    {

/*
 * If this is a Here Document, we need some clever processing to stop it
 * beening deleted or to delete it when the function disappears
 */

	if (((*list)->io_flag & IOTYPE) == IOHERE)
	{

/*
 * Mark this is a function file name.  This should stop the here processing
 * from deleting it
 */

	    (*list)->io_flag |= IOFUNCTION;

/* If this is a delete - delete the file name */

	    if (func == ReleaseMemoryCell)
		unlink ((*list)->io_name);
	}

/* OK, normal processing now!! */

	(*func)((void *)(*list)->io_name);
	(*func)((void *)*(list++));
    }

/* Handle the block itself */

    (*func)((void *)Slist);
}


/*
 * FUNCTION TREE DUPLICATION
 */

/*
 * Copy function tree area by recursively processing of tree
 */

static C_Op * F_LOCAL DuplicateFunctionTree (C_Op *Old_t)
{
    C_Op	*New_t;

    if (Old_t == (C_Op *)NULL)
	return (C_Op *)NULL;

/*
 * This will copy function and for identifiers quite accidently
 */

    New_t        = (C_Op *)DuplicateMemoryCell (Old_t);
    New_t->str   = DuplicateMemoryArea (Old_t->str, char *);
    New_t->vars  = DuplicateWordList (Old_t->vars);
    New_t->args  = DuplicateWordList (Old_t->args);
    New_t->ioact = DuplicateIOActions (Old_t->ioact);
    New_t->left  = DuplicateFunctionTree (Old_t->left);
    New_t->right = DuplicateFunctionTree (Old_t->right);
    return New_t;
}


/*
 * Duplicate a Block of words
 */

static char ** F_LOCAL DuplicateWordList (char **list)
{
    char	**Nlist;
    char	**Np;

    if ((Nlist = DuplicateMemoryArea (list, char **)) == (char **)NULL)
        return NOWORDS;

/* Ok - dup it */

    for (Np = Nlist; *list != NOWORD; list++)
	*(Np++) = DuplicateMemoryArea (*list, char *);

/* Terminate it */

    *Np = NOWORD;

    return Nlist;
}


/*
 * Duplicate the IO Actions block
 */

static IO_Actions ** F_LOCAL DuplicateIOActions (IO_Actions **list)
{
    IO_Actions	**Nlist;
    IO_Actions	**Np;

    if ((Nlist = DuplicateMemoryArea (list,
				      IO_Actions **)) == (IO_Actions **)NULL)
        return (IO_Actions **)NULL;

/* Ok - dup it */

    for (Np = Nlist; *list != (IO_Actions *)NULL; list++, Np++)
    {
	*Np = (IO_Actions *)DuplicateMemoryCell (*list);
	(*Np)->io_name = DuplicateMemoryArea ((*list)->io_name, char *);
    }

/* Terminate it */

    *Np = (IO_Actions *)NULL;
    return Nlist;
}


/*
 * Duplicate the tree
 */

C_Op		*CopyFunction (C_Op *Old_t)
{
    ErrorPoint	save_ErrorReturnPoint;
    jmp_buf	new_ErrorReturnPoint;
    C_Op	*New_t = (C_Op *)NULL;

/* Set up for error handling - like out of space */

    save_ErrorReturnPoint = e.ErrorReturnPoint;

    if (SetErrorPoint (new_ErrorReturnPoint) == 0)
	New_t = DuplicateFunctionTree (Old_t);

    e.ErrorReturnPoint = save_ErrorReturnPoint;
    return New_t;
}

/*
 * Alias processing
 */

void PrintAlias (char *name)
{
    register AliasList	*al = LookUpAlias (name, FALSE);

    if ((al == (AliasList *)NULL) || (al->value == null))
	return;

    printf (ListVarFormat, name, al->value);
}

/*
 * Print All Aliases
 */

int  PrintAllAlias (bool tracked)
{
    DisplayListMode = tracked;		/* Set mode			*/

    twalk (AliasTree, DisplayAlias);
    return 0;
}

/*
 * Save an alias
 */

bool SaveAlias (char *name, char *arguments, bool tracked)
{
    register AliasList	*al;
    void		(*save_signal)(int);


/* Create new entry */

    if (((al = (AliasList *)GetAllocatedSpace (sizeof (AliasList)))
		== (AliasList *)NULL) ||
	((al->name = GetAllocatedSpace (strlen (name) + 1)) == (char *)NULL))
	return FALSE;

    if ((arguments != null) &&
	((al->value = GetAllocatedSpace (strlen (arguments) + 1))
		== (char *)NULL))
	return FALSE;

/* Delete old name */

    DeleteAlias (name);

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);
    strcpy (al->name, name);

/* Add it to the tree */

    if (tsearch (al, &AliasTree, SearchAlias) != (void *)NULL)
    {
	SetMemoryAreaNumber ((void *)al, 0);
	SetMemoryAreaNumber ((void *)al->name, 0);

	if (arguments != null)
	    SetMemoryAreaNumber ((void *)strcpy (al->value, arguments), 0);

	else
	    al->value = null;

        if (tracked)
	    al->AFlags = ALIAS_TRACKED;
    }


/* Restore signals */

    signal (SIGINT, save_signal);
    return TRUE;
}

/*
 * Delete an alias
 */

void DeleteAlias (char *name)
{
    register AliasList	**alp = (AliasList **)tfind (name, &AliasTree,
						     FindAlias);
    void		(*save_signal)(int);
    register AliasList	*al;

    if (alp == (AliasList **)NULL)
	return;

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

/* Delete the tree entry and release the memory */

    al = *alp;
    tdelete (name, &AliasTree, FindAlias);
    ReleaseMemoryCell ((void *)al->name);

    if (al->value != null)
	ReleaseMemoryCell ((void *)al->value);

    ReleaseMemoryCell ((void *)al);

/* Restore signals */

    signal (SIGINT, save_signal);
}

/*
 * Search for an Alias
 */

AliasList	*LookUpAlias (char *name, bool CreateTracked)
{
    AliasList	**alp = (AliasList **)tfind (name, &AliasTree, FindAlias);
    char	*path;

/* If we found a tracked alias, which has been untracked, re-track it if
 * necesary
 */

    if ((alp != (AliasList **)NULL) && ((*alp)->value == null))
    {
        if (CreateTracked &&
	    ((path = AllocateMemoryCell (FFNAME_MAX)) != (char *)NULL) &&
	    (FindLocationOfExecutable (path, name) != EXTENSION_NOT_FOUND))
        {
	    SetMemoryAreaNumber ((void *)path, 0);
	    (*alp)->value = PATH_TO_UNIX (path);
	    return *alp;
        }

        else
            return (AliasList *)NULL;
    }

    return (alp == (AliasList **)NULL) ? (AliasList *)NULL : *alp;
}

/*
 * Check for valid alias name
 */

bool IsValidAliasName (char *s, bool alias)
{
    if (!IS_VariableFC ((int)*s) || LookUpSymbol (s))
	return FALSE;

    while (IS_VariableSC ((int)*s))
        ++s;

    return C2bool (!*s);
}

/*
 * Untrack all Aliases
 */

void UnTrackAllAliases (void)
{
    twalk (AliasTree, UntrackAlias);
}

/*
 * The associate TWALK function
 */

static void  UntrackAlias (const void *key, VISIT visit, int level)
{
    AliasList	*al = *(AliasList **)key;

    if (((visit == postorder) || (visit == leaf)) &&
	(al->AFlags & ALIAS_TRACKED) && (al->value != null))
    {
	 ReleaseMemoryCell ((void *)al->value);
	 al->value = null;
    }
}

/*
 * Look up a job in the save tree
 */

#if (OS_TYPE != OS_DOS)
JobList *LookUpJob (int JobNumber)
{
    JobList	**jp = (JobList **)tfind (&JobNumber, &JobTree, FindJob);

    return jp != (JobList **)NULL ? *jp : (JobList *)NULL;
}

/*
 * Search for a job
 */

JobList	*SearchForJob (char *String)
{
    JobSearchKey = String;
    JobSearchEntry = (JobList **)NULL;

    if ((strcmp (String, "%") == 0) || (strcmp (String, "+") == 0))
	return LookUpJob (CurrentJob);

    else if (strcmp (String, "-") == 0)
	return LookUpJob (PreviousJob);

/* Search for it */

    twalk (JobTree, FindJobByString);
    return JobSearchEntry != (JobList **)NULL ? *JobSearchEntry
					      : (JobList *)NULL;
}

/*
 * Delete a job by Session ID
 */

void	DeleteJobBySession (unsigned short SessionId)
{
    register JobList	**jpp = (JobList **)tfind (&SessionId, &JobTree,
						   FindJobBySession);

    if (jpp != (JobList **)NULL)
	DeleteJob ((*jpp)->pid);
}

/*
 * Delete a job by Process ID
 */

void DeleteJob (PID pid)
{
    register JobList	**jpp = (JobList **)tfind (&pid, &JobTree,
						   FindJobByPID);
    void		(*save_signal)(int);
    JobList		*jp;

    if (jpp == (JobList **)NULL)
	return;

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);
    jp = *jpp;

/* Free the tree and delete the entry */

    tdelete (&pid, &JobTree, FindJobByPID);

    if (jp->Number == PreviousJob)
	PreviousJob = 0;

    if (jp->Number == CurrentJob)
	CurrentJob = PreviousJob;

    ReleaseMemoryCell ((void *)jp->Command);
    ReleaseMemoryCell ((void *)jp);

/* Restore signals */

    signal (SIGINT, save_signal);
}

/*
 * Save a job ID
 */

int	AddNewJob (PID pid, unsigned short SessionId, char *command)
{
    register JobList	*jp;
    static int		JobNumber = 1;
    void		(*save_signal)(int);
    char		*tmp;

/* We if we can get the full command */

   if ((tmp = GetLastHistoryString ()) != (char *)NULL)
       command = tmp;

/* Create new entry */

    if (((jp = (JobList *)GetAllocatedSpace (sizeof (JobList))) == (JobList *)NULL) ||
	((jp->Command = GetAllocatedSpace (strlen (command) + 1)) == (char *)NULL))
	return 0;

/* Get the next available job number */

    while (TRUE)
    {
	jp->pid = pid;
	jp->Number = JobNumber++;
	jp->SessionId = SessionId;

	if (JobNumber > 32000)
	    JobNumber = 1;

	if (tfind (jp, &JobTree, SearchJob) == (void *)NULL)
	    break;
    }

/* Disable signals */

    save_signal = signal (SIGINT, SIG_IGN);

    if (tsearch (jp, &JobTree, SearchJob) != (void *)NULL)
    {
	SetMemoryAreaNumber ((void *)jp, 0);
	SetMemoryAreaNumber ((void *)strcpy (jp->Command, command), 0);
	PATH_TO_UNIX (jp->Command);
    }

/* Restore signals */

    signal (SIGINT, save_signal);

    PreviousJob = CurrentJob;
    return CurrentJob = jp->Number;
}

/*
 * Display Jobs
 */

int PrintJobs (bool Mode)
{
    DisplayListMode = Mode;		/* Set mode			*/

    twalk (JobTree, DisplayJob);
    return 0;
}

/*
 * Count the number of active jobs
 */

int NumberOfActiveJobs (void)
{
    NumberOfJobs = 0;

    twalk (JobTree, CountJob);
    return NumberOfJobs;
}
#endif

/*
 * OS2 1.x - Parse the kernel process information
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_16)
static bool F_LOCAL Parse_V1ProcessTable (UCHAR * bBuf, V1ProcessStatus_t *pi)
{
    USHORT	sel, offs;
    USHORT	type, tpid;
    USHORT	count, kount;
    UCHAR	buffer[256];
    UCHAR	*cptr, *ptr;

    ptr = bBuf;
    sel = SELECTOROF (ptr);

    while ((type = *(USHORT *) ptr) != 0xFFFFU )
    {
	ptr += 2;
	offs = *(USHORT *) ptr;
	ptr += 2;

	switch ( type )
	{
	    case 0:					/* Process */
		if (pi->N_Processes >= pi->M_Processes)
		{
		    V1Process_t	**newp = (V1Process_t **)ReAllocateSpace (
		    				(void *)pi->V1Processes,
			       			(pi->M_Processes + 50) *
						    sizeof (V1Process_t *));

		    if (newp == (V1Process_t **)NULL)
			return TRUE;

		    pi->V1Processes = newp;
		    pi->M_Processes += 50;
		}

/* Create the process entry */

		pi->V1Processes[pi->N_Processes] =
		    (V1Process_t *) GetAllocatedSpace (sizeof (V1Process_t));

		if (pi->V1Processes[pi->N_Processes] == (V1Process_t *)NULL)
		    return TRUE;

		pi->V1Processes[pi->N_Processes]->pid = *(USHORT *)ptr;
		ptr += 2;
		pi->V1Processes[pi->N_Processes]->ppid = *(USHORT *) ptr;
		ptr += 2;
		ptr += 2;
		pi->V1Processes[pi->N_Processes]->modhandle = *(USHORT *) ptr;
		pi->V1Processes[pi->N_Processes++]->threads = 0;

		break;

	    case 1:				 /* Thread	 */
		ptr += 2;
		tpid = *(USHORT *) ptr;

/* Increment the thread count for the process */

		for (count = 0; count < pi->N_Processes; count++)
		{
		    if (pi->V1Processes[count]->pid == tpid)
		    {
			++pi->V1Processes[count]->threads;
			break;
		    }
		}

		break;

	    case 2:					/* module	*/
		if (pi->N_Modules >= pi->M_Modules)
		{
		    V1Module_t	**newm = (V1Module_t **)ReAllocateSpace (
		    				(void *)pi->V1Modules,
						(pi->M_Modules + 50) *
						    sizeof (V1Module_t *));

		    if (newm == (V1Module_t **)NULL)
			return TRUE;

		    pi->V1Modules = newm;
		    pi->M_Modules += 50;
		}

		pi->V1Modules[pi->N_Modules]
		    = (V1Module_t *)GetAllocatedSpace (sizeof(V1Module_t));

		if (pi->V1Modules[pi->N_Modules] == (V1Module_t *)NULL)
		    return TRUE;

		pi->V1Modules[pi->N_Modules]->modhandle = *(USHORT *) ptr;
		ptr += 2;
		pi->V1Modules[pi->N_Modules]->max_dependents = *(USHORT *) ptr;
		ptr += 2;
		ptr += 2;
		ptr += 2;

		if (pi->V1Modules[pi->N_Modules] -> max_dependents)
		    ptr += (pi->V1Modules[pi->N_Modules] -> max_dependents) * 2;

		for (cptr = buffer; *cptr++ = *ptr++;)
		    continue;

		if ((pi->V1Modules[pi->N_Modules]->modname =
			StringCopy (buffer)) == null)
		    return 1;

		++pi->N_Modules;

		break;

	    case 3:				/* system semaphore	*/
	    case 4:				/* shared memory	*/
		break;
	}

	ptr = MAKEP(sel, offs);
    }

/* Count modules */

    for (count = 0; count < pi->N_Processes; count++)
    {
	for (kount = 0; kount < pi->N_Modules; kount++)
	{
	    if (pi->V1Processes[count]->modhandle ==
		pi->V1Modules[kount]->modhandle)
	    {
		pi->V1Processes[count]->module = kount;
		break;
	    }
	}
    }

/* Count children */

    for (count = 0; count < pi->N_Processes; count++)
    {
	for (kount = 0; kount < pi->N_Processes; kount++)
	{
	    if (pi->V1Processes[count]->pid == pi->V1Processes[kount]->ppid)
		(pi->V1Processes[count]->children)++;
	}
    }

    return FALSE;
}

/*
 * Process the process information
 */

static void F_LOCAL PrintV1ProcessTree (pid_t pid, int indent,
					V1ProcessStatus_t *pi)
{
    USHORT	count;
    UCHAR	*mName, pName[256];

    for (count = 0; count < pi->N_Processes; count++)
    {
	if ((indent &&    (pi->V1Processes[count]->ppid == (USHORT)pid)) ||
	    ((!indent) && (pi->V1Processes[count]->pid == (USHORT)pid)))
	{
	    if (pi->V1Processes[count]->module)
	    {
		mName = pi->V1Modules[pi->V1Processes[count]->module]->modname;
		DosGetModName (pi->V1Processes[count]->modhandle,
			       sizeof(pName), pName);
	    }

	    else
	    {
		mName = "unknown";  /* Zombie process,	*/
		pName[0] = 0;
	    }

	    printf ("%5d %5d %3d %-8s %*s%s\n", pi->V1Processes[count]->pid,
		    pi->V1Processes[count]->ppid,
		    pi->V1Processes[count]->threads, mName, indent, "", pName);

	    PrintV1ProcessTree (pi->V1Processes[count]->pid, indent + 2, pi);
	}
    }
}


static int SortV1Processes (void *p1, void *p2)
{
    return (*(V1Process_t **)p1)->pid - (*(V1Process_t **)p2)->pid;
}
#endif

/*
 * OS/2 2.x 32-bit version - Display Process Tree
 */

#if (OS_TYPE == OS_OS2)
static void F_LOCAL V2_DisplayProcessTree (USHORT pid, USHORT indent,
					   V2ProcessStatus_t *ps)
{
    V2Process_t		*proc;
    UCHAR		name[FFNAME_MAX];
    USHORT		prty;

/* Not sure if there isn't another termination method	*/

    for (proc = PTR (ps->ppi); (proc->ulEndIndicator != PROCESS_END_INDICATOR);
         proc = (V2Process_t *) PTR (proc->ptiFirst + proc->usThreadCount))
    {
	if ((indent    && (proc->pidParent == pid)) ||
	    ((!indent) && (proc->pid == pid)))
	{
#  if (OS_SIZE == OS_32)
	    if (DosQueryModuleName (proc->hModRef, sizeof (name), name))
#  else
	    if (DosGetModName (proc->hModRef, sizeof (name), name))
#  endif
		strcpy(name, "<unknown>");

	    if (Dos32GetPrty (PRTYS_PROCESS, &prty, proc->pid))
		prty = 0;

	    printf ("%5d %5d %3d %04x %*s%s\n", proc->pid, proc->pidParent,
	    	    proc->usThreadCount, prty, indent, "", name);

	    V2_DisplayProcessTree (proc->pid, indent + 2, ps);
	}
    }
}

/*
 * Print the Process Tree
 */

int PrintProcessTree (pid_t pid)
{
#  if (OS_SIZE == OS_16)
    USHORT		rc;

/* Switch on release number */

    if (_osmajor < 20)
    {
	UCHAR			*pBuf = GetAllocatedSpace (0x2000);
	USHORT			count;
	V1ProcessStatus_t	pi;

	pi.M_Processes = pi.M_Modules = pi.N_Processes = pi.N_Modules = 0;
	pi.V1Processes = NULL;
	pi.V1Modules = NULL;

	if (pBuf == (UCHAR *)NULL)
	    return 1;

	if (rc = Dos32QProcStatus (pBuf, 0x2000))
	    return PrintWarningMessage ("jobs: DosQProcStatus failed\n%s",
	    				GetOSSystemErrorMessage (rc));

	if (Parse_V1ProcessTable (pBuf, &pi))
	   return 1;

	ReleaseMemoryCell ((void *)pBuf);

	qsort ((void *)pi.V1Processes, pi.N_Processes, sizeof (V1Process_t *),
		SortV1Processes);

        puts ("   PID  PPID TC Name     Program");
	PrintV1ProcessTree (pid, 0, &pi);

	for (count = 0; count < pi.N_Processes; count++)
	    ReleaseMemoryCell ((void *)pi.V1Processes[count]);

	for (count = 0; count < pi.N_Modules; count++)
	{
	    ReleaseMemoryCell ((void *)pi.V1Modules[count] -> modname);
	    ReleaseMemoryCell ((void *)pi.V1Modules[count]);
	}

	ReleaseMemoryCell ((void *)pi.V1Processes);
	ReleaseMemoryCell ((void *)pi.V1Modules);
    }

/*
 * OS2 2.0 - grap space, get the information and display it
 */

    else
#  endif
#  if defined (__WATCOMC__)
    {
	int	res;

	if ((res = spawnlp (P_WAIT, "ps.exe", "ps", IntegerToString (pid),
			    (char *)NULL)) < 0)
	    return PrintWarningMessage ("jobs: cannot find ps.exe\n");

	else if (res)
	    return 1;
    }
#  else
    {
	V2ProcessStatus_t	*ps;

	if ((ps = GetProcessStatus ("jobs")) == (V2ProcessStatus_t *)NULL)
	    return 1;

        puts ("   PID  PPID TC PRI  Program");
	V2_DisplayProcessTree (pid, 0, ps);
	ReleaseMemoryCell ((void *)ps);
    }
#  endif
    return 0;
}

/*
 * Get OS/2 2.x Process Structure
 */

static V2ProcessStatus_t * F_LOCAL GetProcessStatus (char *name)
{
    OSCALL_RET		rc;
    V2ProcessStatus_t	*ps;

    if ((ps = (V2ProcessStatus_t *)GetAllocatedSpace (0x8000))
	    == (V2ProcessStatus_t *)NULL)
	return (V2ProcessStatus_t *)NULL;

    if ((rc = Dos32QProcStatus (ps, 0x8000)))
    {
	PrintWarningMessage ("%s: DosQProcStatus failed\n%s", name,
			     GetOSSystemErrorMessage (rc));
	return (V2ProcessStatus_t *)NULL;
    }

    return ps;
}
#endif

/*
 * A unix version of print process tree
 */

#if (OS_TYPE == OS_UNIX)
int PrintProcessTree (pid_t pid)
{
    return  system ("ps") ? PrintWarningMessage ("jobs: cannot find ps\n")
			  : 0;
}
#endif

/*
 * A sort of times is available for OS/2. Not quite the complete UNIX
 * emulation, since it only reports current children.  However!
 * I think the times in the process structure are in 1/100th second.
 */

#if (OS_TYPE == OS_OS2) && (OS_SIZE == OS_32)
int PrintTimes (void)
{
    V2ProcessStatus_t	*ps;
    V2Process_t		*ppiLocal;
    V2Thread_t		*ptiCurrent;
    time_t		*Utime;
    time_t		*Stime;
    int			i;
    USHORT		pid = getpid ();
    time_t		tms_utime = 0;
    time_t		tms_stime = 0;
    time_t		tms_cutime = 0;
    time_t		tms_cstime = 0;

/* Initialise */

    if ((ps = GetProcessStatus ("times")) == (V2ProcessStatus_t *)NULL)
	return 1;

/* Scan the process tree */

    for (ppiLocal = ps->ppi;
	 (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR);
         ppiLocal = (V2Process_t *) (ppiLocal->ptiFirst +
	 			     ppiLocal->usThreadCount))
    {
	if (ppiLocal->pidParent == pid)
	{
	    Utime = &tms_cutime;
	    Stime = &tms_cstime;
	}

	else if (ppiLocal->pid == pid)
	{
	    Utime = &tms_utime;
	    Stime = &tms_stime;
	}

/* If neither - skip */

	else
	    continue;

/* Process threads */

	ptiCurrent = ppiLocal->ptiFirst;

	for (i = 0; i < ppiLocal->usThreadCount; i++, ptiCurrent++)
	{
	    *Utime += ptiCurrent->ulUserTime;
	    *Stime += ptiCurrent->ulSysTime;
	}
    }

/* Dump the info */

    printf ("%ldm%.2ld.%.2lds %ldm%ld.%.2lds\n",
	    tms_utime / 6000L,  (tms_utime % 6000L) / 100L,  tms_utime % 100L,
	    tms_stime / 6000L,  (tms_stime % 6000L) / 100L,  tms_stime % 100L);
    printf ("%ldm%.2ld.%.2lds %ldm%ld.%.2lds\n",
	    tms_cutime / 6000L, (tms_cutime % 6000L) / 100L, tms_cutime % 100L,
	    tms_cstime / 6000L, (tms_cstime % 6000L) / 100L, tms_cstime % 100L);

    return 0;
}
#endif

/* NT Version */

#if (OS_TYPE == OS_NT)
int PrintTimes (void)
{
    FILETIME	CreationTime;
    FILETIME	ExitTime;
    FILETIME	KernelTime;
    FILETIME	UserTime;

    if (!GetProcessTimes (0, &CreationTime, &ExitTime, &KernelTime, &UserTime))
	return PrintWarningMessage ("times: GetProcesTimes failed\n%s",
				    GetOSSystemErrorMessage (GetLastError ()));
    
    printf ("Kernel Time = %ld %ld\n", KernelTime.dwHighDateTime,
	    KernelTime.dwLowDateTime);
    printf ("User Time   = %ld %ld\n", UserTime.dwHighDateTime,
	    UserTime.dwLowDateTime);

    return 0;
}
#endif

/* UNIX Version */

#if (OS_TYPE == OS_UNIX)
int PrintTimes (void)
{
    struct tms		tms;

    times (&tms);

    printf ("%dm%.2d.%.2ds %dm%d.%.2ds\n%dm%.2d.%.2ds %dm%d.%.2ds\n",
	    tms.tms_utime / (60L * (long)CLK_TCK),
	      (tms.tms_utime % (60L * (long)CLK_TCK)) / (long)CLK_TCK,
	      tms.tms_utime % (long)CLK_TCK,
	    tms.tms_stime / (60L * (long)CLK_TCK),
	      (tms.tms_stime % (60L * (long)CLK_TCK)) / (long)CLK_TCK,
	      tms.tms_stime % (long)CLK_TCK,
	    tms.tms_cutime / (60L * (long)CLK_TCK),
	      (tms.tms_cutime % (60L * (long)CLK_TCK)) / (long)CLK_TCK,
	      tms.tms_cutime % (long)CLK_TCK,
	    tms.tms_cstime / (60L * (long)CLK_TCK),
	      (tms.tms_cstime % (60L * (long)CLK_TCK)) / (long)CLK_TCK,
	      tms.tms_cstime % (long)CLK_TCK);

    return 0;
}
#endif

/*
 * Specials for EMX
 */

#if defined (__EMX__) && (OS_TYPE == OS_OS2)
USHORT	Dos32GetPrty (USHORT usScope, PUSHORT pusPriority, USHORT pid)
{
    return ((USHORT)
	    (_THUNK_PROLOG (2 + 4 + 2);
	     _THUNK_SHORT (usScope);
	     _THUNK_FLAT (pusPriority);
	     _THUNK_SHORT (pid);
	     _THUNK_CALL (Dos16GetPrty)));
}

USHORT	Dos32QProcStatus (PVOID pvProcData, USHORT usSize)
{
    return ((USHORT)
	    (_THUNK_PROLOG (4 + 2);
	     _THUNK_FLAT (pvProcData);
	     _THUNK_SHORT (usSize);
	     _THUNK_CALL (Dos16QProcStatus)));
}
#endif
