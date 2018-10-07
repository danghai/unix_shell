/*
 * MS-DOS SHELL - Maths Evaluation Functions
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited and Paul Falstad
 *
 * This code is based on (in part) the shell program written by Paul Falstad
 * and is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 * This source is based on the math.c (mathematical expression evaluation) from
 * the Z shell.  Z Shell is free software, under the GNU license.  This
 * code is included in this program under the provisions of paragraph 8 of
 * GNU GENERAL PUBLIC LICENSE, Version 1, February 1989.  I contacted Paul
 * via E-Mail, and he is happy to allow the source to be included in this
 * program.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh11.c,v 2.10 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh11.c,v $
 *	Revision 2.10  1994/08/25  20:49:11  istewart
 *	MS Shell 2.3 Release
 *
 *	Revision 2.9  1994/02/01  10:25:20  istewart
 *	Release 2.3 Beta 2, including first NT port
 *
 *	Revision 2.8  1993/11/09  10:39:49  istewart
 *	Beta 226 checking
 *
 *	Revision 2.7  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.7  1993/07/02  10:21:35  istewart
 *	224 Beta fixes
 *
 *	Revision 2.6  1993/06/14  11:01:44  istewart
 *	More changes for 223 beta
 *
 *	Revision 2.5  1993/06/02  09:52:35  istewart
 *	Beta 223 Updates - see Notes file
 *
 *	Revision 2.4  1993/02/16  16:03:15  istewart
 *	Beta 2.22 Release
 *
 *	Revision 2.3  1993/01/26  18:35:09  istewart
 *	Release 2.2 beta 0
 *
 *	Revision 2.2  1992/11/06  10:03:44  istewart
 *	214 Beta test updates
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
#include <setjmp.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "sh.h"

#define STACK_SIZE	100		/* Stack size			*/
#define MAX_PRECEDENCE	15
#define MAX_VARIABLES	32		/* Number of user variables	*/

/*
 * Some macros
 */

#define POP_2_VALUES()		b = stack[StackPointer--].val;	\
			  	a = stack[StackPointer--].val;

#define PUSH_VALUE_ON_STACK(X)	PushOnToStack ((long)(X), -1);
#define SET_VALUE_ON_STACK(X)	PushOnToStack (SetVariableValue	\
						(lv, (long)(X)), lv);

/* the value stack */

static int	LastToken;		/* Last token			*/
static int	StackPointer = -1;	/* Stack pointer		*/

static struct MathsStack {
    int		lval;
    long	val;
} stack[STACK_SIZE];

static int	NumberOfVariables = 0;		/* Number of variables	*/
static char	*ListOfVariableNames[MAX_VARIABLES];
static char	*CStringp;			/* Current position	*/
static long	YYLongValue;
static int	YYIntValue;
static int	JustParsing = 0;		/* Nonzero means we are	*/
						/* not evaluating, just	*/
						/* parsing		*/
static bool	RecogniseUnaryOperator = TRUE;	/* TRUE means recognize	*/
						/* unary plus, minus,	*/
						/* etc.			*/

/*
 * LEFT_RIGHT_A = left-to-right associativity
 * RIGHT_LEFT_A = right-to-left associativity
 * BOOL_A = short-circuiting boolean
 */

#define LEFT_RIGHT_A			0
#define RIGHT_LEFT_A			1
#define BOOL_A				2

#define OP_OPEN_PAR			0
#define OP_CLOSE_PAR			1
#define OP_NOT				2
#define OP_COMPLEMENT			3
#define OP_POST_PLUS			4
#define OP_POST_MINUS			5
#define OP_UNARY_PLUS			6
#define OP_UNARY_MINUS			7
#define OP_BINARY_AND_EQUALS		8
#define OP_BINARY_XOR_EQUALS		9
#define OP_BINARY_OR_EQUALS		10
#define OP_MULTIPLY			11
#define OP_DIVIDE			12
#define OP_MODULUS			13
#define OP_PLUS				14
#define OP_MINUS			15
#define OP_SHIFT_LEFT			16
#define OP_SHIFT_RIGHT			17
#define OP_LESS				18
#define OP_LESS_EQUALS			19
#define OP_GREATER			20
#define OP_GREATER_EQUALS		21
#define OP_EQUALS			22
#define OP_NOT_EQUALS			23
#define OP_LOGICAL_AND_EQUALS		24
#define OP_LOGICAL_OR_EQUALS		25
#define OP_LOGICAL_XOR_EQUALS		26
#define OP_QUESTION_MARK		27
#define OP_COLON			28
#define OP_SET				29
#define OP_PLUS_EQUAL			30
#define OP_MINUS_EQUAL			31
#define OP_MULTIPLY_EQUAL		32
#define OP_DIVIDE_EQUAL			33
#define OP_MODULUS_EQUAL		34
#define OP_BINARY_AND_SET		35
#define OP_BINARY_XOR_SET		36
#define OP_BINARY_OR_SET		37
#define OP_SHIFT_LEFT_EQUAL		38
#define OP_SHIFT_RIGHT_EQUAL		39
#define OP_LOGICAL_AND_SET		40
#define OP_LOGICAL_OR_SET		41
#define OP_LOGICAL_XOR_SET		42
#define OP_COMMA			43
#define OP_END_OF_INPUT			44
#define OP_PRE_PLUS			45
#define OP_PRE_MINUS			46
#define OP_NUMERIC_VALUE		47
#define OP_VARIABLE			48
#define MAX_OPERATORS			49

/* precedences */

static struct Operators {
    int		Precedences;		/* Operator Precedences		*/
    int		Type;			/* Operator Association		*/
} Operators [MAX_OPERATORS] = {
    {   1,	LEFT_RIGHT_A },		/* 0 - (			*/
    { 137,	LEFT_RIGHT_A },		/* 1 - )			*/
    {   2,	RIGHT_LEFT_A },		/* 2 - !			*/
    {   2,	RIGHT_LEFT_A },		/* 3 - ~			*/
    {   2,	RIGHT_LEFT_A },		/* 4 - x++			*/
    {   2,	RIGHT_LEFT_A },		/* 5 - x--			*/
    {   2,	RIGHT_LEFT_A },		/* 6 - +			*/
    {   2,	RIGHT_LEFT_A },		/* 7 - -			*/
    {   4,	LEFT_RIGHT_A },		/* 8 - &			*/
    {   5,	LEFT_RIGHT_A },		/* 9 - ^			*/
    {   6,	LEFT_RIGHT_A },		/* 10 - |			*/
    {   7,	LEFT_RIGHT_A },		/* 11 - *			*/
    {   7,	LEFT_RIGHT_A },		/* 12 - /			*/
    {   7,	LEFT_RIGHT_A },		/* 13 - %			*/
    {   8,	LEFT_RIGHT_A },		/* 14 - +			*/
    {   8,	LEFT_RIGHT_A },		/* 15 - -			*/
    {   3,	LEFT_RIGHT_A },		/* 16 - <			*/
    {   3,	LEFT_RIGHT_A },		/* 17 - >			*/
    {   9,	LEFT_RIGHT_A },		/* 18 - < (less than)		*/
    {   9,	LEFT_RIGHT_A },		/* 19 - <= (less than equal)	*/
    {   9,	LEFT_RIGHT_A },		/* 20 - > (greater than)	*/
    {   9,	LEFT_RIGHT_A },		/* 21 - >= (greater than equal) */
    {  10,	LEFT_RIGHT_A },		/* 22 - ==			*/
    {  10,	LEFT_RIGHT_A },		/* 23 - !=			*/
    {  11,	BOOL_A },		/* 24 - &&			*/
    {  12,	BOOL_A },		/* 25 - ||			*/
    {  12,	LEFT_RIGHT_A },		/* 26 - ^^			*/
    {  13,	RIGHT_LEFT_A },		/* 27 - ?			*/
    {  13,	RIGHT_LEFT_A },		/* 28 - :			*/
    {  14,	RIGHT_LEFT_A },		/* 29 - = 			*/
    {  14,	RIGHT_LEFT_A },		/* 30 - +=			*/
    {  14,	RIGHT_LEFT_A },		/* 31 - -=			*/
    {  14,	RIGHT_LEFT_A },		/* 32 - *=			*/
    {  14,	RIGHT_LEFT_A },		/* 33 - /=			*/
    {  14,	RIGHT_LEFT_A },		/* 34 - %=			*/
    {  14,	RIGHT_LEFT_A },		/* 35 - &=			*/
    {  14,	RIGHT_LEFT_A },		/* 36 - ^=			*/
    {  14,	RIGHT_LEFT_A },		/* 37 - |=			*/
    {  14,	RIGHT_LEFT_A },		/* 38 - <=			*/
    {  14,	RIGHT_LEFT_A },		/* 39 - >=			*/
    {  14,	BOOL_A },		/* 40 - &&=			*/
    {  14,	BOOL_A },		/* 41 - ||=			*/
    {  14,	RIGHT_LEFT_A },		/* 42 - ^^=			*/
    {  15,	RIGHT_LEFT_A },		/* 43 - ,			*/
    { 200,	RIGHT_LEFT_A },		/* 44 - End of input		*/
    {   2,	RIGHT_LEFT_A },		/* 45 - ++x			*/
    {   2,	RIGHT_LEFT_A },		/* 46 - --x			*/
    {   0,	LEFT_RIGHT_A },		/* 47 - Numeric value		*/
    {   0,	LEFT_RIGHT_A }		/* 48 - Variable		*/
};



/*
 * Functions
 */

static void F_LOCAL	PushOnToStack (long, int);
static long F_LOCAL	SetVariableValue (int, long);
static bool F_LOCAL	CheckNotZero (long);
static void F_LOCAL	ProcessOperator (int);
static void F_LOCAL	ProcessBinaryOperator (int);
static void F_LOCAL	ParseMathsExpression (int);
static int F_LOCAL	MathsLexicalAnalyser (void);
static int F_LOCAL	DecideSingleOperator (int, int);
static int F_LOCAL	DecideDoubleOperator (char, int, int, int, int);
static int F_LOCAL	DecideSignOperator (char, int, int, int, int, int);
static char * F_LOCAL	ExtractNameAndIndex (int, int *);

/*
 * Analyse the string
 */

static int F_LOCAL MathsLexicalAnalyser (void)
{
    while (TRUE)
    {
	switch (*(CStringp++))
	{
	    case '+':
		return DecideSignOperator ('+', OP_PRE_PLUS,
						OP_POST_PLUS,
						OP_PLUS_EQUAL,
						OP_UNARY_PLUS,
						OP_PLUS);

	    case '-':
		return DecideSignOperator ('+', OP_PRE_MINUS,
						OP_POST_MINUS,
						OP_MINUS_EQUAL,
						OP_UNARY_MINUS,
						OP_MINUS);

	    case CHAR_OPEN_PARATHENSIS:
		RecogniseUnaryOperator = TRUE;
		return OP_OPEN_PAR;

	    case CHAR_CLOSE_PARATHENSIS:
		return OP_CLOSE_PAR;

	    case '!':
		if (*CStringp == '=')
		{
		    RecogniseUnaryOperator = TRUE;
		    CStringp++;
		    return OP_NOT_EQUALS;
		}

		return OP_NOT;

	    case CHAR_TILDE:
		return OP_COMPLEMENT;

	    case '&':
		return DecideDoubleOperator ('&', OP_LOGICAL_AND_SET,
						  OP_BINARY_AND_SET,
						  OP_LOGICAL_AND_EQUALS,
						  OP_BINARY_AND_EQUALS);

	    case '|':
		return DecideDoubleOperator ('|', OP_LOGICAL_OR_SET,
						  OP_BINARY_OR_SET,
						  OP_LOGICAL_OR_EQUALS,
						  OP_BINARY_OR_EQUALS);

	    case CHAR_XOR:
		return DecideDoubleOperator (CHAR_XOR, OP_LOGICAL_XOR_SET,
						  OP_BINARY_XOR_SET,
						  OP_LOGICAL_XOR_EQUALS,
						  OP_BINARY_XOR_EQUALS);

	    case '*':
		return DecideSingleOperator (OP_MULTIPLY_EQUAL, OP_MULTIPLY);

	    case '/':
		return DecideSingleOperator (OP_DIVIDE_EQUAL, OP_DIVIDE);

	    case '%':
		return DecideSingleOperator (OP_MODULUS_EQUAL, OP_MODULUS);

	    case '<':
		return DecideDoubleOperator ('<', OP_SHIFT_LEFT_EQUAL,
						  OP_LESS_EQUALS,
						  OP_SHIFT_LEFT,
						  OP_LESS);

	    case '>':
		return DecideDoubleOperator ('>', OP_SHIFT_RIGHT_EQUAL,
						  OP_GREATER_EQUALS,
						  OP_SHIFT_RIGHT,
						  OP_GREATER);

	    case CHAR_ASSIGN:
		return DecideSingleOperator (OP_EQUALS, OP_SET);

	    case '?':
		RecogniseUnaryOperator = TRUE;
		return OP_QUESTION_MARK;

	    case ':':
		RecogniseUnaryOperator = TRUE;
		return OP_COLON;

	    case ',':
		RecogniseUnaryOperator = TRUE;
		return OP_COMMA;

	    case 0:
		RecogniseUnaryOperator = TRUE;
		CStringp--;
		return OP_END_OF_INPUT;

	    default:
		if (isspace (*(CStringp - 1)))
		    break;

/* Check for a numeric value */

		if (isdigit (*--CStringp))
		{
		    char	*End;
		    int		base = 10;

		    RecogniseUnaryOperator = FALSE;

/* Which format are we in? base#number or number.  Base is a decimal number
 * so we can check for a decimal base and look at the end character to see
 * if it was a # sign.  If it was, this is a base#number.  Otherwise, its
 * just a number
 */

		    strtol (CStringp, &End, 10);

		    if (*End == '#')
		    {
			LastNumberBase = (int)strtol (CStringp, &CStringp, 10);
			base = LastNumberBase;
			CStringp++;
		    }

		    YYLongValue = strtol (CStringp, &CStringp, base);
		    return OP_NUMERIC_VALUE;
		}

/* Check for a variable */

		if (IS_VariableFC ((int)*CStringp))
		{
		    char	*p, q;
		    char	*eb;

		    p = CStringp;

		    if (NumberOfVariables == MAX_VARIABLES)
		    {
			ShellErrorMessage ("too many identifiers");
			ExpansionErrorDetected = TRUE;
			return OP_END_OF_INPUT;
		    }

		    RecogniseUnaryOperator = FALSE;

		    while (IS_VariableSC ((int)(*++CStringp)))
			continue;

/* Save the variable name.  Check for array index and skip over. */

		    if ((*CStringp == CHAR_OPEN_BRACKETS) &&
			((eb = strchr (CStringp,
				       CHAR_CLOSE_BRACKETS)) != (char *)NULL))
			CStringp = eb + 1;

/* Save the string */

		    q = *CStringp;
		    *CStringp = 0;
		    ListOfVariableNames[YYIntValue = NumberOfVariables++] =
					StringCopy (p);
		    *CStringp = q;
		    return OP_VARIABLE;
		}

		return OP_END_OF_INPUT;
	}
    }
}

/*
 * Stick variable on the stack
 */

static void F_LOCAL PushOnToStack (long val, int lval)
{
    if (StackPointer == STACK_SIZE - 1)
    {
	ShellErrorMessage ("stack overflow");
	ExpansionErrorDetected = TRUE;
    }

    else
	StackPointer++;

    stack[StackPointer].val = val;
    stack[StackPointer].lval = lval;
}

/*
 * Get a variable value
 */

static long F_LOCAL SetVariableValue (int s, long v)
{
    char	*vn;
    int		index;

    if (s == -1 || s >= NumberOfVariables)
    {
	ExpansionErrorDetected = TRUE;
	ShellErrorMessage ("lvalue required");
	return 0;
    }

    if (JustParsing)
	return v;

    vn = ExtractNameAndIndex (s, &index);
    SetVariableArrayFromNumeric (vn, index, v);
    return v;
}

/*
 * Check for Division by zero
 */

static bool F_LOCAL CheckNotZero (long a)
{
    if (a)
	return TRUE;

    ShellErrorMessage ("division by zero");
    ExpansionErrorDetected = TRUE;
    return FALSE;
}

/*
 * Process operator
 */

static void F_LOCAL ProcessOperator (int what)
{
    long	a, b, c;
    int		lv;

    if (StackPointer < 0)
    {
	ShellErrorMessage ("bad math expression - stack empty");
	ExpansionErrorDetected = TRUE;
	return;
    }

    switch (what)
    {
	case OP_NOT:
	    stack[StackPointer].val = !stack[StackPointer].val;
	    stack[StackPointer].lval= -1;
	    break;

	case OP_COMPLEMENT:
	    stack[StackPointer].val = ~stack[StackPointer].val;
	    stack[StackPointer].lval= -1;
	    break;

	case OP_POST_PLUS:
	    SetVariableValue (stack[StackPointer].lval,
			       stack[StackPointer].val + 1);
	    break;

	case OP_POST_MINUS:
	    SetVariableValue (stack[StackPointer].lval,
			       stack[StackPointer].val - 1);
	    break;

	case OP_UNARY_PLUS:
	    stack[StackPointer].lval= -1;
	    break;

	case OP_UNARY_MINUS:
	    stack[StackPointer].val = -stack[StackPointer].val;
	    stack[StackPointer].lval= -1;
	    break;

	case OP_BINARY_AND_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a & b);
	    break;

	case OP_BINARY_XOR_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a ^ b);
	    break;

	case OP_BINARY_OR_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a | b);
	    break;

	case OP_MULTIPLY:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a * b);
	    break;

	case OP_DIVIDE:
	    POP_2_VALUES ();

	    if (CheckNotZero (b))
		 PUSH_VALUE_ON_STACK (a / b);

	    break;

	case OP_MODULUS:
	    POP_2_VALUES ();

	    if (CheckNotZero (b))
		PUSH_VALUE_ON_STACK (a % b);

	    break;

	case OP_PLUS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a + b);
	    break;

	case OP_MINUS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a - b);
	    break;

	case OP_SHIFT_LEFT:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a  <<  b);
	    break;

	case OP_SHIFT_RIGHT:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a >> b);
	    break;

	case OP_LESS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a < b);
	    break;

	case OP_LESS_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a <= b);
	    break;

	case OP_GREATER:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a > b);
	    break;

	case OP_GREATER_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a >= b);
	    break;

	case OP_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a == b);
	    break;

	case OP_NOT_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a != b);
	    break;

	case OP_LOGICAL_AND_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a && b);
	    break;

	case OP_LOGICAL_OR_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (a || b);
	    break;

	case OP_LOGICAL_XOR_EQUALS:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK ((a && !b) || (!a && b));
	    break;

	case OP_QUESTION_MARK:
	    c = stack[StackPointer--].val;
	    POP_2_VALUES ();

	    PUSH_VALUE_ON_STACK ((a) ? b : c);
	    break;

	case OP_COLON:
	    break;

	case OP_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (b);
	    break;

	case OP_PLUS_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a + b);
	    break;

	case OP_MINUS_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a - b);
	    break;

	case OP_MULTIPLY_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a * b);
	    break;

	case OP_DIVIDE_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;

	    if (CheckNotZero (b))
		SET_VALUE_ON_STACK (a / b);

	    break;

	case OP_MODULUS_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;

	    if (CheckNotZero (b))
		 SET_VALUE_ON_STACK (a % b);

	    break;

	case OP_BINARY_AND_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a & b);
	    break;

	case OP_BINARY_XOR_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a ^ b);
	    break;

	case OP_BINARY_OR_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a | b);
	    break;

	case OP_SHIFT_LEFT_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a << b);
	    break;

	case OP_SHIFT_RIGHT_EQUAL:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a >> b);
	    break;

	case OP_LOGICAL_AND_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a && b);
	    break;

	case OP_LOGICAL_OR_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK (a || b);
	    break;

	case OP_LOGICAL_XOR_SET:
	    POP_2_VALUES ();
	    lv = stack[StackPointer + 1].lval;
	    SET_VALUE_ON_STACK ((a && !b) || (!a && b));
	    break;

	case OP_COMMA:
	    POP_2_VALUES ();
	    PUSH_VALUE_ON_STACK (b);
	    break;

	case OP_PRE_PLUS:
	    stack[StackPointer].val =
		SetVariableValue (stack[StackPointer].lval,
				  stack[StackPointer].val + 1);
	    break;

	case OP_PRE_MINUS:
	    stack[StackPointer].val =
		SetVariableValue (stack[StackPointer].lval,
				  stack[StackPointer].val - 1);
	    break;

	default:
	    ShellErrorMessage ("out of integers");
	    ExpansionErrorDetected = TRUE;
	    return;
    }
}

/*
 * Handle binary operators
 */

static void F_LOCAL ProcessBinaryOperator (int tk)
{
    switch (tk)
    {
	case OP_LOGICAL_AND_EQUALS:
	case OP_LOGICAL_AND_SET:
	    if (!stack[StackPointer].val)
		JustParsing++;

	    break;

	case OP_LOGICAL_OR_EQUALS:
	case OP_LOGICAL_OR_SET:
	    if (stack[StackPointer].val)
		JustParsing++;

	    break;
    }
}

/*
 * Common processing
 */

long EvaluateMathsExpression (char *s)
{
    int		i;

    for (i = 0; i != MAX_VARIABLES; i++)
	ListOfVariableNames[i] = (char *)NULL;

    LastNumberBase = -1;		/* Reset base			*/
    NumberOfVariables = 0;
    CStringp = s;
    StackPointer = -1;
    RecogniseUnaryOperator = TRUE;
    ParseMathsExpression (MAX_PRECEDENCE);

    if (StackPointer)
    {
	ShellErrorMessage ("bad math expression - unbalanced stack");
	ExpansionErrorDetected = TRUE;
    }

    if (*CStringp)
    {
	ShellErrorMessage ("bad math expression - illegal character: %c",
			   *CStringp);
	ExpansionErrorDetected = TRUE;
    }

    for (i = 0; i != NumberOfVariables; i++)
	ReleaseMemoryCell ((void *)ListOfVariableNames[i]);

    return stack[0].val;
}

/*
 * operator-precedence parse the string and execute
 */

static void F_LOCAL ParseMathsExpression (int pc)
{
    char	*vn;
    int		index;

    LastToken = MathsLexicalAnalyser ();

    while (Operators[LastToken].Precedences <= pc)
    {
	if (LastToken == OP_NUMERIC_VALUE)
	    PushOnToStack (YYLongValue, -1);

	else if (LastToken == OP_VARIABLE)
	{
	    vn = ExtractNameAndIndex (YYIntValue, &index);
	    PushOnToStack (GetVariableArrayAsNumeric (vn, index), YYIntValue);
	}

	else if (LastToken == OP_OPEN_PAR)
	{
	    ParseMathsExpression (MAX_PRECEDENCE);

	    if (LastToken != OP_CLOSE_PAR)
	    {
		ShellErrorMessage ("unmatched ()'s");
		return;
	    }
	}

	else if (LastToken == OP_QUESTION_MARK)
	{
	    long	q = stack[StackPointer].val;

	    if (!q)
		JustParsing++;

	    ParseMathsExpression (Operators[OP_QUESTION_MARK].Precedences - 1);

	    if (!q)
		JustParsing--;

	    else
		JustParsing++;

	    ParseMathsExpression (Operators[OP_QUESTION_MARK].Precedences);

	    if (q)
		JustParsing--;

	    ProcessOperator (OP_QUESTION_MARK);
	    continue;
	}

	else
	{
	    int		otok = LastToken;
	    int		onoeval = JustParsing;

	    if (Operators[otok].Type == BOOL_A)
		ProcessBinaryOperator (otok);

	    ParseMathsExpression (Operators[otok].Precedences -
				  (Operators[otok].Type != RIGHT_LEFT_A));
	    JustParsing = onoeval;
	    ProcessOperator (otok);
	    continue;
	}

	LastToken = MathsLexicalAnalyser ();
    }
}

/*
 * Decide on 'op=', 'opop' or 'op'
 */

static int F_LOCAL DecideDoubleOperator (char op,
					 int dequal,
					 int equal,
				         int twice,
					 int single)
{
    RecogniseUnaryOperator = TRUE;

    if (*CStringp == op)
    {
	if (*(++CStringp) == CHAR_ASSIGN)
	{
	    CStringp++;
	    return dequal;
	}

	return twice;
    }

    else if (*CStringp == CHAR_ASSIGN)
    {
	CStringp++;
	return equal;
    }

    return single;
}

/*
 * Decide on single operator 'op' or 'op='
 */

static int F_LOCAL DecideSingleOperator (int equal, int single)
{
    RecogniseUnaryOperator = TRUE;

    if (*CStringp != CHAR_ASSIGN)
	return single;

    CStringp++;
    return equal;
}

/*
 * Handle +=, ++, + or -=, --, -
 */

static int F_LOCAL DecideSignOperator (char sign, int pre_op, int post_op,
				    int equals, int unary_op, int op)
{
    if ((*CStringp == sign) && (RecogniseUnaryOperator || !isalnum (*CStringp)))
    {
	CStringp++;
	return RecogniseUnaryOperator ? pre_op : post_op;
    }

    if (*CStringp == CHAR_ASSIGN)
    {
	RecogniseUnaryOperator = TRUE;
	CStringp++;
	return equals;
    }

    return RecogniseUnaryOperator ? unary_op : op;
}

/*
 * Validate maths expression without the error message generated a
 * command.  This forces the processing to drop down the stack rather than
 * doing a deep-exit on error.
 */

bool ValidMathsExpression (char *string, long *value)
{
    ErrorPoint 	Save_ERP = e.ErrorReturnPoint;
    bool	Save_EED = ExpansionErrorDetected;
    bool	Result;

/* save the environment */

    e.ErrorReturnPoint = (ErrorPoint)NULL;
    ExpansionErrorDetected = FALSE;

/* Validate the number */

    *value = EvaluateMathsExpression (string);
    Result = ExpansionErrorDetected;

/* Restore environment */

    e.ErrorReturnPoint = Save_ERP;
    ExpansionErrorDetected = Save_EED;
    return Result;
}

/*
 * Extract a variable name and possible index
 */

static char * F_LOCAL ExtractNameAndIndex (int s, int *index)
{
    long	IndValue;
    char	*Vp;
    char	*sp = StringCopy (ListOfVariableNames[s]);

    if (!GetVariableName (sp, &IndValue, &Vp, (bool *)NULL) || (*Vp))
	PrintErrorMessage (BasicErrorMessage, ListOfVariableNames[s],
			   LIT_BadID);

    if (IndValue == -1)
	PrintErrorMessage (LIT_BadArray, ListOfVariableNames[s]);

    *index = (int)IndValue;
    return sp;
}
