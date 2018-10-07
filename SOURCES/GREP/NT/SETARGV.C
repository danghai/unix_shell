/*
 *	setargv.c - parse arguments
 *
 *	$Header: C:/SRC/RCS/setargv.c 1.11 1994/01/25 23:20:37 few Exp $
 *
 *	This code evolved out of a requirement for expansion of wildcard
 *	arguments given on the command line.  None of the (currently)
 *	available wildcard matchers did the job for me.  I want my "argv"
 *	to arrive correctly expanded.  I also use several different compilers 
 *	and operating systems.  This requires some intricate gyrations, as 
 *	you can discover below.
 *
 *	This version can expand wildcard arguments either by globbing or 
 *	by using system facilities.  Expanded filename arguments are usually 
 *	converted to lower case, although this can be overridden.  Arguments 
 *	that are quoted (with either " or ') are left alone, but the quote 
 *	characters are stripped.
 *	#ifdef UNIXBS
 *	 Any single character may be escaped with a backslash (\).
 *	#else
 *	 Quote characters may be escaped with a backslash (\).
 *	#endif
 *
 *	"@name" processing is also handled by this package.  In order to
 *	overcome the 126 byte command line limitation of MS-DOS, several
 *	tool makers have agreed to pass lengthy command lines either in
 *	an environment variable or a temporary file.  This mechanism comes
 *	to play only if the first non-blank character of a command line
 *	is '@'.  First the environment is searched for a tag matching
 *	"name", otherwise "name" is assumed to be the name of a temporary
 *	file containing the command line.  This code extends the "@file"
 *	mechanism by reading up to 'MAXLINE' bytes from a file, where
 *	individual arguments may be separated by any whitespace character
 *	(including newlines).
 *
 *	MKS-style extended arguments which are passed in the environment
 *	are also interpreted.  If MKS-style arguments are selected, this
 *	module also provides _setenvp() which ensures that these arguments
 *	are stripped from the environment.  Several tools (make programs,
 *	shells, etc.) pass lengthy arguments via the MKS method, but do
 *	not perform wildcard expansion.  There is a problem in combining 
 *	tools that expect MKS-style arguments to be already expanded with 
 *	tools that don't expect expansion -- it's hard to tell what you 
 *	really meant.  In this case, nested quoting ("'arg'") may be 
 *	required to get wildcard arguments into your program.
 *
 *	Finally (I think), 4DOS stores command lines in the environment
 *	variable CMDLINE.
 *
 *	Even though OS/2 allows lengthy command lines, the three methods
 *	of passing long command lines are included in the OS/2 version,
 *	for use in "compatibility mode" programs.
 *
 *	The Windows/NT version uses an undocumented external variable 
 *	and may be incomplete.  It works for me.
 *
 *	In all cases the "correct" command line is retrieved before any
 *	filename expansion is performed.  This allows all calling programs
 *	(shells, make, etc.) to pass a simple version of the command line.
 *
 *	Seven flags select the compiler and operating system:
 *		__TURBOC__	Turbo-C 2.0
 *		__TURBOCCC__	Turbo-C++ 1.0
 *		__BORLANDC__	Borland C++ 2.0/3.x
 *		__MSC__		Microsoft C 5.10, 7.0 or 8.0 (not 6.00)
 *		__MSDOS__	MS-DOS 3.x/4.x/5.x/6.x
 *		__OS2__		OS/2 1.2/1.3/2.0
 *		__NT__		Windows/NT
 *	Note that when using Turbo C, Turbo C++, or Borland C++, the flags
 *	__TURBOC__, __TURBOCCC__, __BORLANDC__, and __MSDOS__ are set
 *	automatically.
 *
 *	Ten additional conditional-compilation flags are provided:
 *		ATARGS:		Handle extended arguments passed by the
 *				"@name" protocol.
 *		CMDLINE:	Handle 4DOS-style arguments passed in the
 *				"CMDLINE" environment variable.
 *		FIXARG0:	Convert argv[0] to lower case and switch
 *				all backslashes to slashes (cosmetic).
 *		GLOB:		Use filename globbing instead of standard
 *				system wildcard matching.
 *		LCFAT:		Force FAT filenames to lowercase under 
 *				Windows NT, when UPPERCASE is also defined.
 *		MATCHCASE:	Only match globbed arguments when their
 *				character case matches exactly.
 *		MKSARGS:	Look for MKS-style extended arguments
 *				passed in the environment.
 *		SORTARGS:	Use qsort() to sort the expansions of
 *				each argument.
 *		UNIXBS		Treat backslashes in the Unix fashion (as
 *				a single-character escape) rather than as
 *				directory separator characters.
 *		UPPERCASE:	Leave matched arguments as uppercase, instead
 *				of converting to lowercase.
 *
 *	A test program is provided at the end of this file, and is built
 *	if 'TEST' is defined.  A few possibilities:
 *	 tcc -DTEST -DFIXARG0 -DGLOB -DSORTARGS setargv.c
 *	 cl -D__MSC__ -D__MSDOS__ -DTEST -DMKSARGS setargv.c -link /NOE/ST:8192
 *	 cl -D__MSC__ -D__OS2__ -G2 -DTEST -DGLOB -DSORTARGS setargv.c \ 
 *							    -link /NOE/ST:8192
 *	 cl -D__MSC__ -D__NT__ -DTEST -DFIXARG0 -DGLOB -DUPPERCASE -DLCFAT \
 *							-DSORTARGS setargv.c
 *	or, since VC++ broke the '-link' option:
 *	 cl -D__MSDOS__ -D__MSC__ -DTEST -DATARGS -DFIXARG0 -DGLOB -DMKSARGS \ 
 *					-DSORTARGS -F 2000/NOE -c setargv.c
 *
 *	Note that this code requires just under 6K of stack space (along with
 *	a substantial amount of allocated memory), which exceeds the default
 *	of 4K built into most runtime code.
 *
 *	Notes on using Microsoft C 6.00:
 *		In version 6.00 of Microsoft's C compiler, the memory
 *		allocation initialization was changed to occur sometime
 *		after _setargv() is called.  This causes a situation where
 *		(sometimes) the first malloc() call made by this code
 *		enters an infinite loop.  I have only seen this problem on
 *		MS-DOS; OS/2 memory allocation initialization seems to be
 *		handled differently.  The problem does not seem to occur
 *		with MSC 7.0 or 8.0.
 *
 *	Notes on 4DOS-style 'CMDLINE=' arguments:
 *		This code does not remove the 'CMDLINE=...' string from the
 *		environment.  If a program using this module calls another
 *		program that recognizes 'CMDLINE=', it is possible for the
 *		child program to receive the original program's command line
 *		(if the caller's environment is passed unchanged).  The
 *		'CMDLINE=...' string can be removed by a
 *			putenv("CMDLINE=");
 *		statement in your program.
 *
 *	Further enhancements greatly appreciated.
 *
 *	This code placed in the public domain on 25-Jan-94 by the original 
 *	author, Frank Whaley (few@autodesk.com).
 *
 *	The author would like to thank W. Metzenthen of Monash University
 *	in Australia (apm233m@vaxc.cc.monash.edu.au) for his considerable
 *	assistance in producing the Turbo-C++ 1.0 modifications.  I have
 *	paraphrased his notes below.
 *	----------
 *	Turbo C++ uses a different mechanism for its startup code.  Instead
 *	of a built-in call to _setargv(), it links in much of its startup
 *	code only if needed.  For example, if a program does not reference
 *	_argv or _argc, and main() is declared with no formal parameters, then
 *	the setargv code will not be linked in.
 *
 *	If a program does reference _argv or _argc, then the appropriate code
 *	is linked automatically.
 *
 *	If main() is declared with formal parameters then the object code
 *	produced by tcc (or tc) will include a reference to __setargv__.
 *	In this case we need to rename _setargv() to _setargv__().
 *
 *	In addition, it is necessary to have a short block of data placed
 *	in a data segment named INITDATA in the group INIT in order for the
 *	startup code to function correctly.  This is handled by resorting to
 *	in-line assembly code.
 *
 *	Similarly, if MKSARGS is used, _setenvp__() must be defined instead
 *	of _setenvp().
 */

#define CHUNK	128	/*  pointer allocation chunksize  */
#define MAXLINE	4096	/*  max unexpanded command line length  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#ifdef __TURBOC__
	/*  TC++, BC++  */
/*  Normally the following test would be '#if __TURBOC__ >= 0x0200', however
 *  I have applied an allegedly "official" Borland patch to TCC.EXE which
 *  causes __TURBOC__ to be defined as 0x0200.  Now I can't think of a test
 *  that will tell me if I'm using TC++.
 */
#if WORKING_TURBO_CPLUSPLUS_TEST || defined(__BORLANDC__)
#pragma inline
#define __TURBOCCC__
#define _setargv		_setargv__
#define _setenvp		_setenvp__
#define ARGC			_C0argc
#define ARGV			_C0argv
#ifdef MKSARGS
char **environ;
#endif /*MKSARGS*/
extern char **_C0environ;
#else /*__TURBOCCC__*/
#define ARGC			_argc
#define ARGV			_argv
#endif /*__TURBOCCC__*/

#include <alloc.h>
#include <dir.h>
#include <dos.h>
#include <mem.h>
typedef struct ffblk FIND_T;
#define findclose(f)
#define FAR			__far
#endif /*__TURBOC__*/

#if defined(__MSC__) && defined(__MSDOS__)
#include <dos.h>
#include <malloc.h>
#include <memory.h>
#define ARGC			__argc
#define ARGV			__argv
#ifndef MK_FP
#define MK_FP(seg,ofs)		((void __far *)(((unsigned long)(seg)<<16)|\
							(unsigned)(ofs)))
#endif
typedef struct find_t FIND_T;
#define findfirst(n,f,a)	_dos_findfirst(n,a,f)
#define findnext(f)		_dos_findnext(f)
#define findclose(f)
#define ff_name			name
#define ff_attrib		attrib
#define FA_RDONLY		_A_RDONLY
#define FA_HIDDEN		_A_HIDDEN
#define FA_SYSTEM		_A_SYSTEM
#define FA_DIREC		_A_SUBDIR
#define FAR			__far
#endif /*__MSC__&&__MSDOS__*/

#if defined(__MSC__) && defined(__OS2__)
#define INCL_DOS
#include <os2.h>
#include <malloc.h>
#include <memory.h>
#define ARGC			__argc
#define ARGV			__argv
typedef struct
{
	HDIR ft_handle;
	FILEFINDBUF ft_ffb;
	int ft_count;
} FIND_T;
#define findfirst(n,f,a)	((f)->ft_handle=0xFFFF,(f)->ft_count=1,\
					DosFindFirst(n,&(f)->ft_handle,a,\
					&(f)->ft_ffb,sizeof((f)->ft_ffb),\
					&(f)->ft_count,0L))
#define findnext(f)		DosFindNext((f)->ft_handle,&(f)->ft_ffb,\
					sizeof((f)->ft_ffb),&(f)->ft_count)
#define findclose(f)		DosFindClose((f)->ft_handle);
#define ff_attrib		ft_ffb.attrFile
#define ff_name			ft_ffb.achName
#define FA_RDONLY		0x01
#define FA_HIDDEN		0x02
#define FA_SYSTEM		0x04
#define FA_DIREC		0x10
#define FAR			__far
#endif /*__MSC__&&__OS2__*/

	/*  MSC and NT  */
#if defined(__MSC__) && defined(__NT__)
#include <windows.h>
#include <direct.h>
#include <io.h>
#define ARGC			__argc
#define ARGV			__argv
extern char *_acmdln;
typedef struct
{
	HANDLE ft_hdl;
	WIN32_FIND_DATA ft_ffb;
} FIND_T;
#define findfirst(n,f,a)	(((f)->ft_hdl=FindFirstFile(n,&(f)->ft_ffb))\
					==INVALID_HANDLE_VALUE)
#define findnext(f)		!FindNextFile((f)->ft_hdl,&(f)->ft_ffb)
#define findclose(f)		FindClose((f)->ft_hdl)
#define ff_attrib		ft_ffb.dwFileAttributes
#define ff_name			ft_ffb.cFileName
#define FA_RDONLY		FILE_ATTRIBUTE_READONLY
#define FA_HIDDEN		FILE_ATTRIBUTE_HIDDEN
#define FA_SYSTEM		FILE_ATTRIBUTE_SYSTEM
#define FA_DIREC		FILE_ATTRIBUTE_DIRECTORY
#define strdup			_strdup
#define strlwr			_strlwr
#define write			_write
#endif /*__MSC__&&__NT__*/

	/*  mask for all interesting files  */
#define ALL		(FA_RDONLY+FA_HIDDEN+FA_SYSTEM+FA_DIREC)

	/*  let's do some things with macros...  */
#ifdef MATCHCASE
#define CHARMATCH(a,b)	((a)==(b))
#else /*MATCHCASE*/
#define CHARMATCH(a,b)	(lowercase(a)==lowercase(b))
#endif /*MATCHCASE*/
#ifdef UPPERCASE
#define CONCAT		strcat
#else /*UPPERCASE*/
#define CONCAT		lwrcat
#endif /*UPPERCASE*/
#define ISBLANK(c)	(((c)==' ')||((c)=='\t')||((c)=='\r')||((c)=='\n'))
#define ISQUOTE(c)	(((c)=='"')||((c)=='\''))
#ifdef GLOB
#define ISWILD(c)	(((c)=='?')||((c)=='*')||((c)=='['))
#else /*GLOB*/
#define ISWILD(c)	(((c)=='?')||((c)=='*'))
#endif /*GLOB*/

static char *cmdLine;		/*  -> local copy of command line  */
static char dirSep[] = "/";	/*  current directory separator  */
static int maxArgs;		/*  current max arg count  */

	/*  error messages  */
static char memFail[] = "Memory shortage\n";

extern int ARGC;		/*  number of args  */
extern char **ARGV;		/*  arg ptrs  */

	/*  forward declarations  */
#if defined(ATARGS) || defined(CMDLINE)
static char FAR *pickEnvVar(const char *name);
#endif /*ATARGS||CMDLINE*/
#ifdef FIXARG0
static char lowercase(char c);
#endif /*FIXARG0*/
#ifdef GLOB
static char **fileGlob(char *pattern);
#endif /*GLOB*/
static int hasWildcard(void);
static void pickRegular(void);
static void pickWild(void);
static void *xMalloc(size_t size);
static void *xRealloc(void *block, size_t size);
static char *xStrdup(char *s);


/*
-*	_setargv - set argument vector
 */
#ifdef __TURBOCCC__
	void FAR
#else /*__TURBOCCC__*/
	void
#endif /*__TURBOCCC__*/
_setargv(void)
{
	char buf[MAXLINE];	/*  working buffer  */
	char FAR *farPtr;	/*  generic far ptr  */
	int len;		/*  command line length  */
#ifdef MKSARGS
	char FAR *mksPtr;	/*  ptr to MKS-style arguments  */
#endif /*MKSARGS*/
#ifdef __OS2__
	USHORT selEnviron;	/*  environment segment selector  */
	USHORT usOffsetCmd;	/*  offset to command line  */
#endif /*__OS2__*/

	/*  get working vector  */
	ARGV = xMalloc(CHUNK * sizeof(char *));
	maxArgs = CHUNK;

	/*  get program name  */	
#ifdef __NT__
	cmdLine = buf;
	farPtr = _acmdln;
	while ( *farPtr && !ISBLANK(*farPtr) )
	{
#ifdef FIXARG0
		*cmdLine = lowercase(*farPtr++);
		if ( *cmdLine == '\\' )
			*cmdLine = '/';
		cmdLine++;
#else /*FIXARG0*/
		*cmdLine++ = *farPtr++;
#endif /*FIXARG0*/
	}
#else /*__NT__*/
	/*  copy program name from environment  */
	cmdLine = buf;
#ifdef __OS2__
	DosGetEnv(&selEnviron, &usOffsetCmd);
	farPtr = MAKEP(selEnviron, usOffsetCmd);
#endif /*__OS2__*/
#ifdef __MSDOS__
	farPtr = MK_FP(*(int FAR *)MK_FP(_psp, 0x2c), 0);
	while ( *farPtr )
		if ( !*++farPtr )
			farPtr++;
	farPtr += 3;
#endif /*__MSDOS__*/

	while ( *farPtr )
	{
#ifdef FIXARG0
		*cmdLine = lowercase(*farPtr++);
		if ( *cmdLine == '\\' )
			*cmdLine = '/';
		cmdLine++;
#else /*FIXARG0*/
		*cmdLine++ = *farPtr++;
#endif /*FIXARG0*/
	}
#endif /*__NT__*/
	*cmdLine = '\0';
	ARGV[0] = xStrdup(buf);

#ifdef MKSARGS
	/*  get MKS-style arguments if there  */
#ifdef __NT__
	mksPtr = (char *)GetEnvironmentStrings();
#endif /*__NT__*/
#ifdef __OS2__
	mksPtr = MAKEP(selEnviron, 0);
#endif /*__OS2__*/
#ifdef __MSDOS__
	mksPtr = MK_FP(*(int FAR *)MK_FP(_psp, 0x2c), 0);
#endif /*__MSDOS__*/

	if ( *mksPtr == '~' )
	{
		/*  skip over argv[0]  */
		while ( *mksPtr++ )
			;
		cmdLine = buf;
		while ( *mksPtr == '~' )
		{
			mksPtr++;
			while ( *mksPtr )
				*cmdLine++ = *mksPtr++;
			if ( *++mksPtr == '~' )
				*cmdLine++ = ' ';
		}
		*cmdLine = '\0';
	}
	else
#endif /*MKSARGS*/
	{
		/*  copy cmd line from PSP  */
		cmdLine = buf;
#if defined(__OS2__) || defined(__NT__)
		farPtr++;
		/*  remove leading blanks  */
		while ( ISBLANK(*farPtr) )
			farPtr++;
		/*  copy  */
		while ( *cmdLine++ = *farPtr++ )
			;
#else /*__OS2__*/
		farPtr = MK_FP(_psp, 0x80);
		len = *farPtr++;
		/*  remove leading blanks  */
		while ( len && ISBLANK(*farPtr) )
		{
			farPtr++;
			len--;
		}
		/*  copy  */
		while ( len )
		{
			*cmdLine++ = *farPtr++;
			len--;
		}
		*cmdLine = 0;
#endif /*__OS2__*/

#ifdef ATARGS
		/*  handle "@name"  */
		if ( buf[0] == '@' )
		{
			FILE *fp;

			/*  try environment first  */
			if ( (farPtr = pickEnvVar(buf + 1)) != NULL )
			{
				for ( cmdLine = buf;
				      (*cmdLine++ = *farPtr++) != '\0'; )
					;
			}
			else
			{
				/*  open '@' file  */
				if ( (fp = fopen(buf + 1, "rt")) != (FILE *)0 )
				{
					int length;
					/*  load the buffer  */
					length = fread(buf, sizeof(*buf),
							sizeof(buf) - 1, fp);
					buf[length] = '\0';
					/*  close up  */
					fclose(fp);
				}
			}
		}
#endif /*ATARGS*/

#ifdef CMDLINE
		if ( (farPtr = pickEnvVar("CMDLINE")) != NULL )
		{
			/*  skip over program name  */
			while ( *farPtr && !ISBLANK(*farPtr) )
				farPtr++;
			for ( cmdLine = buf;
			      (*cmdLine++ = *farPtr++) != '\0'; )
				;
		}
#endif /*CMDLINE*/
	}

	/*  let's get down to some expansion  */
	ARGC = 1;
	cmdLine = buf;
	while ( *cmdLine )
	{
		/*  deblank  */
		while ( ISBLANK(*cmdLine) )
			cmdLine++;

		/*  pick next argument  */
		while ( *cmdLine )
		{
			if ( hasWildcard() )
				pickWild();
			else
				pickRegular();

			/*  deblank  */
			while ( ISBLANK(*cmdLine) )
				cmdLine++;
		}
	}
/*
 *  This final realloc() should be done to save a bit of memory (the
 *  end of the last block allocated to ARGV.  However, both TC's and
 *  MSC's realloc() functions sometimes break here.  So we waste a
 *  few bytes...
 */
#if 0
	/*  shrink vector -- this might be overkill  */
	ARGV = (char **)xRealloc(ARGV, ARGC * sizeof(char *));
#endif
}

#ifdef __TURBOCCC__
asm	_INIT_	Segment Word Public 'INITDATA'
asm		DB	1
asm		DB	16
asm		DD	__setargv__
asm	_INIT_	EndS
#endif /*__TURBOCCC__*/

/*
 *	does current argument contain a globbing char ??
 */
	static int
hasWildcard(void)
{
	char *s = cmdLine;	/*  work ptr  */
	char quote;	/*  current quote char  */

	while ( *s && !ISBLANK(*s) )
	{
		/*  skip over quoted sections  */
		if ( ISQUOTE(*s) )
		{
			quote = *s++;
			while ( *s && (*s != quote) )
			{
#ifdef UNIXBS
				if ( *s == '\\' )
					s++;
#else /*UNIXBS*/
				if ( (*s == '\\') && (*(s + 1) == quote) )
					s++;
#endif /*UNIXBS*/
				s++;
			}
		}
		else if ( ISWILD(*s) )
			return ( 1 );
#ifdef UNIXBS
		else if ( *s == '\\' )
			s++;
#else /*UNIXBS*/
		else if ( (*s == '\\') && ISQUOTE(*(s + 1)) )
			s++;
#endif /*UNIXBS*/
		s++;
	}

	return ( 0 );
}

/*
 *	allocate memory with test
 */
	static void *
xMalloc(size_t size)
{
	void *mem;

	if ( (mem = malloc(size)) == NULL )
	{
		write(2, memFail, sizeof(memFail));
		abort();
	}

	return ( mem );
}

/*
 *	re-allocate memory with test
 */
	static void *
xRealloc(void *block, size_t size)
{
	void *mem;

	if ( (mem = realloc(block, size)) == NULL )
	{
		write(2, memFail, sizeof(memFail));
		abort();
	}

	return ( mem );
}

/*
 *	dup string with test
 */
	static char *
xStrdup(char *s)
{
	char *t;

	if ( (t = strdup(s)) == NULL )
	{
		write(2, memFail, sizeof(memFail));
		abort();
	}

	return ( t );
}

/*
 *	pick a regular argument from command line
 */
	static void
pickRegular(void)
{
	char buf[128];	/*  argument buffer  */
	char *bp = buf;	/*  work ptr  */
	char quote;	/*  current quote char  */

	/*  copy argument (minus quotes) into local buffer  */
	while ( *cmdLine && !ISBLANK(*cmdLine) )
	{
		if ( ISQUOTE(*cmdLine) )
		{
			quote = *cmdLine++;
			while ( *cmdLine && (*cmdLine != quote) )
			{
#ifdef UNIXBS
				if ( *cmdLine == '\\' )
					cmdLine++;
#else /*UNIXBS*/
				if ( (*cmdLine == '\\') &&
				     (*(cmdLine + 1) == quote) )
					cmdLine++;
#endif /*UNIXBS*/
				*bp++ = *cmdLine++;
			}
			if ( *cmdLine )
				cmdLine++;
		}
		else
		{
#ifdef UNIXBS
			if ( *cmdLine == '\\' )
				cmdLine++;
#else /*UNIXBS*/
			if ( (*cmdLine == '\\') && ISQUOTE(*(cmdLine + 1)) )
				cmdLine++;
#endif /*UNIXBS*/
			*bp++ = *cmdLine++;
		}
	}
	*bp = '\0';

	/*  skip over terminator char  */
	if ( *cmdLine )
		cmdLine++;

	/*  store ptr to copy of string  */
	ARGV[ARGC++] = xStrdup(buf);
	/*  reallocate vector if necessary  */
	if ( ARGC >= maxArgs )
	{
		maxArgs += CHUNK;
		ARGV = xRealloc(ARGV, maxArgs * sizeof(char *));
	}
}

/*
 *	safe 'tolower' function
 */
	static char
lowercase(char c)
{
	return ( (char)tolower(c) );
}

#if !defined(UPPERCASE) && !defined(GLOB)
/*
 *	concatenate strings, convert to lower case
 */
	static char *
lwrcat(char *s, char *t)
{
	char *cp = s;

	while ( *cp )
		cp++;
	while ( (*cp++ = lowercase(*t++)) != '\0' )
		;

	return ( s );
}
#endif /*!UPPERCASE&&!GLOB*/

#if defined(ATARGS) || defined(CMDLINE)
/*
 *	check for environment variable
 */
	static int
envMatch(char FAR *ptr, const char *name)
{
	int len = strlen(name);

	while ( len-- )
		if ( *ptr++ != *name++ )
			return ( 0 );

	return ( 1 );
}
/*
 *	find 'real' environment variable
 */
	static char FAR *
pickEnvVar(const char *name)
{
#ifdef __OS2__
	USHORT selEnviron;	/*  environment segment selector  */
	USHORT usOffsetCmd;	/*  (dummy) offset to command line  */
	char FAR *ptr;		/*  ptr into environment  */

	DosGetEnv(&selEnviron, &usOffsetCmd);
	ptr = MAKEP(selEnviron, 0);
#else /*__OS2__*/
	char FAR *ptr = MK_FP(*(int FAR *)MK_FP(_psp, 0x2c), 0);
#endif /*__OS2__*/

	while ( *ptr )
	{
		if ( envMatch(ptr, name) )
		{
			ptr += strlen(name);
			while ( *ptr == ' ')
				ptr++;
			if ( *ptr++ == '=' )
				return ( ptr );
		}
		while ( *ptr++ )
			;
	}

	return ( NULL );
}
#endif /*ATARGS||CMDLINE*/

/*
 *	pick pathname from argument
 */
	static void
pickPath(char *arg, char *path)
{
	char *t;
	int n;

	/*  find beginning of basename  */
	for ( t = arg + strlen(arg) - 1; t >= arg; t-- )
		if ( (*t == '/') ||
#ifndef UNIXBS
		     (*t == '\\') ||
#endif /*!UNIXBS*/
		     (*t == ':') )
			break;

	/*  pick off path  */
	for ( n = (t - arg) + 1, t = arg; n--; )
		*path++ = lowercase(*t++);
	*path = '\0';
}

#ifdef SORTARGS
/*
 *	comparison routine for qsort()
 */
	static int
myCmp(const void *l, const void *r)
{
	return ( strcmp(*(char **)l, *(char **)r) );
}
#endif /*SORTARGS*/

/*
 *	get wildcard argument from command line
 */
	static void
pickWild(void)
{
	char path[256];
	char srch[256];
	char *s = srch;
#ifdef SORTARGS
	int first = ARGC;	/*  first element to sort  */
	int nmatched = 0;	/*  number matched on this arg  */
#endif /*SORTARGS*/
#ifdef GLOB
	char **array;
	int i;
#else /*GLOB*/
	FIND_T f;
#endif /*GLOB*/

	/*  pick search string  */
	while ( *cmdLine && !ISBLANK(*cmdLine) )
		*s++ = *cmdLine++;
	*s = '\0';

	pickPath(srch, path);

#ifdef GLOB
	/*  glob pattern  */
	array = fileGlob(srch);
	/*  copy new names to argv  */
	for ( i = 0; array[i] != NULL; i++ )
	{
		ARGV[ARGC++] = array[i];
		/*  reallocate vector if necessary  */
		if ( ARGC >= maxArgs )
		{
			maxArgs += CHUNK;
			ARGV = xRealloc(ARGV, maxArgs * sizeof(char *));
		}
	}
	free(array);

	/*  get anything ??  */
	if ( i == 0 )
	{
		/*  pass unexpanded argument  */
		ARGV[ARGC++] = xStrdup(srch);
		/*  reallocate vector if necessary  */
		if ( ARGC >= maxArgs )
		{
			maxArgs += CHUNK;
			ARGV = xRealloc(ARGV, maxArgs * sizeof(char *));
		}
		i = 1;
	}
#ifdef SORTARGS
	nmatched = i;
#endif /*SORTARGS*/
#else /*GLOB*/
	if ( findfirst(srch, &f, ALL) )
	{
		/*  no match, pass argument as is  */
		ARGV[ARGC++] = xStrdup(srch);
		/*  reallocate vector if necessary  */
		if ( ARGC >= maxArgs )
		{
			maxArgs += CHUNK;
			ARGV = xRealloc(ARGV, maxArgs * sizeof(char *));
		}
		return;
	}

	do
	{
		/*  add name if not "." or ".."  */
		if ( f.ff_name[0] != '.' )
		{
			strcpy(srch, path);
			CONCAT(srch, f.ff_name);
			ARGV[ARGC++] = xStrdup(srch);
			/*  reallocate vector if necessary  */
			if ( ARGC >= maxArgs )
			{
				maxArgs += CHUNK;
				ARGV = xRealloc(ARGV,
						maxArgs * sizeof(char *));
			}
#ifdef SORTARGS
			nmatched++;
#endif /*SORTARGS*/
		}
	}
	while ( !findnext(&f) );
	findclose(&f);
#endif /*GLOB*/

#ifdef SORTARGS
	/*  sort these entries  */
	qsort(&ARGV[first], nmatched, sizeof(char *), myCmp);
#endif /*SORTARGS*/
}

#ifdef GLOB
/*
 *	does pattern contain any globbing chars ??
 */
	static int
hasGlobChar(char *pattern)
{
	while ( *pattern )
	{
		switch ( *pattern++ )
		{
		case '?':
		case '[':
		case '*':
			return ( 1 );

#ifdef UNIXBS
		case '\\':
			if ( !*pattern++ )
				return ( 0 );
#endif /*UNIXBS*/
		}
	}
	return ( 0 );
}

/*
 *	is file a directory ??
 */
	static int
isDir(char *file)
{
	FIND_T ff;

	/*  what does DOS think it is ??  */
	if ( findfirst(file, &ff, ALL) )
		return ( 0 );
	findclose(&ff);
	return ( ff.ff_attrib & FA_DIREC );
}

/*
 *	match pattern against text
 */
	static int
patternMatch(char *pat, char *txt)
{
	int ret;

	for ( ; *pat; txt++, pat++ )
	{
		/*  shortcut if out of text  */
		if ( (*txt == '\0') && (*pat != '*') )
			return ( -1 );
		switch ( *pat )
		{
		case '*':	/*  match zero or more chars  */
			/*  trailing '*' matches everything  */
			if ( *++pat == '\0' )
				return ( 1 );
			/*  try matching balance of pattern  */
			while ( (ret = patternMatch(pat, txt++)) == 0 )
				;
			return ( ret );

		case '[':	/*  match character class  */
		{
			int last;
			int matched;
			int invert;

			if ( (invert = (pat[1] == '!')) != 0 )
				pat++;
			for ( last = 256, matched = 0;
			      *++pat && (*pat != ']');
			      last = *pat )
				if ( (*pat == '-') ?
#ifdef MATCHCASE
				     (*txt <= *++pat) && (*txt >= last) :
				     (*txt == *pat) )
#else /*MATCHCASE*/
				     (lowercase(*txt) <= lowercase(*++pat)) &&
				     (lowercase(*txt) >=
				     		lowercase((char)last)) :
				     CHARMATCH(*txt, *pat) )
#endif /*MATCHCASE*/
					matched = 1;
			if ( matched == invert )
				return ( 0 );
			break;
		}
#ifdef UNIXBS
		case '\\':	/*  literal match next char  */
			pat++;
			/*FALLTHRU*/
#endif /*UNIXBS*/
		default:	/*  match single char  */
			if ( !CHARMATCH(*txt, *pat) )
				return ( 0 );
			/*FALLTHRU*/
		case '?':	/*  match any char  */
			break;

		}
	}

	/*  matched if end of pattern and text  */
	return ( *txt == '\0' );
}

/*
 *	return vector of names in given dir that match pattern
 */
	static char **
dirGlob(char *pat, char *dir)
{
	char **names;		/*  temporary vector of names  */
	char search[256];	/*  search string  */
	int count = 0;		/*  number of matched filenames  */
	int maxNames;		/*  available entries in 'names'  */
	int pathLen;		/*  length of path in 'search'  */
	FIND_T ff;		/*  findfile() buffer  */

	/*  get temporary vector  */
	names = xMalloc(CHUNK * sizeof(char *));
	maxNames = CHUNK;
	/*  build our search string  */
	strcpy(search, dir);
	/*  save path length for later  */
	pathLen = strlen(search);
	/*  ensure trailing dir separator  */
	if ( search[0] &&
	     (search[pathLen - 1] != '/') &&
#ifndef UNIXBS
	     (search[pathLen - 1] != '\\') &&
#endif /*!UNIXBS*/
	     (search[pathLen - 1] != ':') )
	{
		strcat(search, dirSep);
		pathLen++;
	}
	/*  search for all files  */
	strcat(search, "*.*");

	/*  add names to vector  */
	if ( !findfirst(search, &ff, ALL) )
		do
		{
#if defined(LCFAT) && defined(__NT__) && defined(UPPERCASE)
			/*  convert to lowercase if FAT filesystem  */
			if ( (ff.ft_ffb.ftCreationTime.dwLowDateTime == 0) &&
			     (ff.ft_ffb.ftCreationTime.dwHighDateTime == 0) )
				strlwr(ff.ff_name);
#endif /*LCFAT&&__NT__&&UPPERCASE*/
#ifndef UPPERCASE
			/*  cosmetic  */
			strlwr(ff.ff_name);
#endif /*!UPPERCASE*/
			/*  try to match pattern, skipping "." and ".."  */
			if ( (ff.ff_name[0] != '.') &&
			     (patternMatch(pat, ff.ff_name) == 1) )
			{
				char file[256];
				/*  build complete filename  */
				strncpy(file, search, pathLen);
				strcpy(file + pathLen, ff.ff_name);
				/*  save a copy  */
				names[count++] = xStrdup(file);
				if ( count >= maxNames )
					names = xRealloc(names,
					 (maxNames += CHUNK) * sizeof(char *));
			}
		}
		while ( !findnext(&ff) );
	findclose(&ff);

	/*  terminate  */
	names[count++] = NULL;

	/*  shrink vector and return  */
	return ( (char **)xRealloc(names, (count * sizeof(char *))) );
}

/*
 *	return vector of names that match pattern
 */
	static char **
fileGlob(char *pattern)
{
	char **vec;
	int vecSize;
	char dirName[256];
	int dirLen;
	char *fileName;

	/*  get initial vector  */
	vecSize = 1;
	vec = (char **)xMalloc(sizeof(char *));
	vec[0] = NULL;

	/*  pick the basename  */
	fileName = pattern + strlen(pattern) - 1;
	while ( fileName >= pattern )
	{
		if ( (*fileName == '/') ||
#ifndef UNIXBS
		     (*fileName == '\\') ||
#endif /*!UNIXBS*/
		     (*fileName == ':') )
			break;
		fileName--;
	}
	/*  save their chosen directory separator  */
	if ( (*fileName == '/') || (*fileName == '\\') )
		dirSep[0] = *fileName;
	/*  copy directory name  */
	dirLen = (fileName - pattern) + 1;
	memcpy(dirName, pattern, dirLen);
	dirName[dirLen] = '\0';
	fileName++;

	/*  if dir name contains globbing chars, expand previous levels  */
	if ( hasGlobChar(dirName) )
	{
		char **dirs;
		int i;

#ifdef UNIXBS
		if ( dirName[dirLen - 1] == '/' )
#else /*UNIXBS*/
		if ( (dirName[dirLen - 1] == '/') ||
		     (dirName[dirLen - 1] == '\\') )
#endif /*UNIXBS*/
			dirName[dirLen - 1] = '\0';

		dirs = fileGlob(dirName);

		/*  now glob each directory  */
		for ( i = 0; dirs[i] != NULL; i++ )
		{
			char **array;

			if ( isDir(dirs[i]) )
			{
				int j;
				/*  glob this dir  */
				array = dirGlob(fileName, dirs[i]);
				/*  count number of entries  */
				for ( j = 0; array[j] != NULL; j++ )
					;
				/*  realloc vector  */
				vec = (char **)xRealloc(vec,
					       (vecSize + j) * sizeof(char *));
				/*  append to vector  */
				for ( j = 0; array[j] != NULL; j++ )
					vec[vecSize++ - 1] = array[j];
				vec[vecSize - 1] = NULL;
				free(array);
			}
		}
		/*  free the dirs  */
		for ( i = 0; dirs[i]; i++ )
			free(dirs[i]);
		free(dirs);

		return ( vec );
	}

	/* If there is only a directory name, return it. */
	if ( *fileName == '\0' )
	{
		vec = (char **)xRealloc(vec, 2 * sizeof(char *));
		vec[0] = xStrdup(dirName);
		vec[1] = NULL;
		return ( vec );
	}

	/*  we didn't need a vector  */
	free(vec);
	/*  just return what dirGlob() returns  */
	return ( dirGlob(fileName, dirName) );
}
#endif /*GLOB*/

#ifdef MKSARGS
/*
-*	_setenvp - build vector of environment strings
 */
#ifdef __TURBOCCC__
	void FAR
#else /*__TURBOCCC__*/
	void
#endif /*__TURBOCCC__*/
_setenvp()
{
#if defined(__TURBOC__) && !defined(__TURBOCCC__)
	extern int _envLng;	/*  size of actual environment  */
	extern int _envSize;	/*  size of ptrs to strings  */
#else /*__TURBOC__&&!__TURBOCCC__*/
	int _envLng;		/*  size of actual environment  */
	int _envSize;		/*  size of ptrs to strings  */
#endif /*__TURBOC__&&!__TURBOCCC__*/
	char FAR *farPtr;
	char FAR *wrkPtr;
	char *env;
	int i;
#ifdef __OS2__
	USHORT selEnviron;
	USHORT usOffsetCmd;
#endif /*__OS2__*/

#ifdef __NT__
	wrkPtr = farPtr = GetEnvironmentStrings();
#endif /*__NT__*/
#ifdef __OS2__
	DosGetEnv(&selEnviron, &usOffsetCmd);
	wrkPtr = farPtr = MAKEP(selEnviron, 0);
#endif /*__OS2__*/
#ifdef __MSDOS__
	wrkPtr = farPtr = MK_FP(*(int FAR *)MK_FP(_psp, 0x2c), 0);
#endif /*__MSDOS__*/

	/*  find size of environment without MKS args  */
	_envLng = _envSize = 0;
	while ( *wrkPtr )
	{
		if ( *wrkPtr != '~' )
		{
			while ( *wrkPtr++ )
				_envLng++;
			_envLng++;
			_envSize += sizeof(char *);
		}
		else
			while ( *wrkPtr++ )
				;
	}

	/*  bump a little  */
	_envSize += (sizeof(char *) * 10);

	/*  allocate space  */
	environ = xMalloc(_envSize);
	i = 0;

#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__) || \
    defined(M_I86SM) || defined(M_I86MM)
	/*  copy and parse strings  */
	env = xMalloc(_envLng);
	while ( *farPtr )
	{
		if ( *farPtr != '~' )
		{
			environ[i++] = env;
			while ( (*env++ = *farPtr++) != '\0' )
				;
		}
		else
			while ( *farPtr++ )
				;
	}
#else /*__TINY__||__SMALL__||__MEDIUM__||M_I86SM||M_I86MM*/
	/*  parse strings  */
	while ( *farPtr )
	{
		if ( *farPtr != '~' )
			environ[i++] = farPtr;
		while ( *farPtr++ )
			;
	}
#endif /*__TINY__||__SMALL__||__MEDIUM__||M_I86SM||M_I86MM*/

	environ[i] = NULL;

#ifdef __TURBOCCC__
	_C0environ = environ;
#endif /*__TURBOCCC__*/

}

#ifdef __TURBOCCC__
asm	_INIT_	Segment Word Public 'INITDATA'
asm		DB	1
asm		DB	2
asm		DD	__setenvp__
asm	_INIT_	EndS
#endif /*__TURBOCCC__*/

#endif /*MKSARGS*/

#ifdef TEST
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
unsigned _stklen = 8192;
#endif /*__COMPACT__||__LARGE__||__HUGE__*/
/*
-*	main - _setargv() and _setenvp() test, print arglist and environment
 */
	void
main(int argc, char *argv[], char *envp[])
{
	int i;

	printf("-----\n");
	for ( i = 0; i < argc; i++ )
		printf("argv[%04d]=[%s]\n", i, argv[i]);
	printf("-----\n");
#ifndef SKIPENV
	for ( i = 0; envp[i]; i++ )
		printf("envp[%04d]=[%s]\n", i, envp[i]);
	printf("-----\n");
#endif /*SKIPENV*/
}
#endif /*TEST*/

/*  END of setargv.c  */
