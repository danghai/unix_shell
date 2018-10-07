/* fgrep.c - grep program built around matcher.
   Copyright 1989 Free Software Foundation
		  Written August 1989 by Mike Haertel.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   The author may be reached (Email) at the address mike@ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation. */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif /*__TURBOC__*/

#include "kwset.h"

#define NCHAR (UCHAR_MAX + 1)

#if defined(__TURBOC__) || defined(__BORLANDC__)
int _stklen = 16384;
#endif

/*  from getopt()  */
extern char *optarg;
extern int optind;

/* For error messages. */
static int error_seen;

/* Flags controlling the style of output. */
static int out_silent;		/* Suppress all normal output. */
static int out_invert;		/* Print nonmatching stuff. */
static int out_file;		/* Print filenames. */
static int out_line;		/* Print line numbers. */
static int out_byte;		/* Print byte offsets. */
static int out_before;		/* Lines of leading context. */
static int out_after;		/* Lines of trailing context. */

/* Print MESG and possibly the error string for ERRNUM.  Remember
   that something awful happened. */
static void
error(mesg, errnum)
const char *mesg;
int errnum;
{
	if ( errnum )
		fprintf(stderr, "fgrep: %s: %s\n", mesg, strerror(errnum));
	else
		fprintf(stderr, "fgrep: %s\n", mesg);
	error_seen = 1;
}

/* Like error(), but die horribly after printing. */
static void
fatal(mesg, errnum)
const char *mesg;
int errnum;
{
	error(mesg, errnum);
	exit(2);
}

/* Interface to handle errors and fix library lossage. */
static void *
xmalloc(size)
size_t size;
{
	void *result;

	result = malloc(size);
	if ( size && !result )
		fatal("memory exhausted", 0);
	return ( result );
}

/* Interface to handle errors and fix some library lossage. */
static void *
xrealloc(ptr, size)
void *ptr;
size_t size;
{
	void *result;

	if ( ptr )
		result = realloc(ptr, size);
	else
		result = malloc(size);
	if ( size && !result )
		fatal("memory exhausted", 0);
	return ( result );
}

/* Compiled search pattern. */
kwset_t kwset;

/* Flags controlling how pattern matching is performed. */
static int match_fold;		/* Fold all letters to one case. */
static int match_words;		/* Match only whole words. */
static int match_lines;		/* Match only whole lines. */

static void
compile(pattern, size)
const char *pattern;
size_t size;
{
	const char *beg;
	const char *lim;
	const char *err;
	static char trans[NCHAR];
	int i;

	if ( match_fold )
		for ( i = 0; i < NCHAR; i++ )
			trans[i] = tolower(i);

	if ( (kwset = kwsalloc(match_fold ? trans : NULL)) == (kwset_t *)0 )
		fatal("memory exhausted", 0);

	beg = pattern;
	do
	{
		for ( lim = beg; (lim < (pattern + size)) && (*lim != '\n');
									lim++ )
			;
		if ( (err = kwsincr(kwset, beg, lim - beg)) != NULL )
			fatal(err, 0);
		if ( lim < (pattern + size) )
			lim++;
		beg = lim;
	}
	while ( beg < (pattern + size) )
		;
	if ( (err = kwsprep(kwset)) != NULL )
		fatal(err, 0);
}

static char *
execute(buf, size)
char *buf;
size_t size;
{
	char *beg;
	char *try;
	size_t len;
	struct kwsmatch kwsmatch;

	for ( beg = buf; beg <= (buf + size); beg++ )
	{
		if ( (beg = kwsexec(kwset, beg, buf + size - beg, &kwsmatch))
								== NULL )
			return ( NULL );
		len = kwsmatch.size[0];
		if ( match_lines )
		{
			/* Check that the match starts at the beginning of a line */
			if ( (beg > buf) && (beg[-1] != '\n') )
				continue;

#ifndef __MSDOS__
			/* Check that the match ends at the end of a line */
			if ( ((beg + len) < (buf + size)) &&
			     (*(beg + len) != '\n') )
				continue;
#else
			/* Check that the match ends at the end of a line */
			/* A line can end with either '\n' or '\r\n' */
			{
			char *p = beg + len;
			char *q = buf + size;

			if ( p < q )
				switch ( *p )
				{
				case '\n':
					break;
				case '\r':
					if ( ((p + 1) < q) &&
					     (*(p + 1) != '\n') )
						continue;
					break;
				default:
					continue;
				}
			}
#endif

			return ( beg );
		}
		else if ( match_words )
			for ( try = beg; len && try; )
			{
				if ( (try > buf) &&
				     (isalnum(try[-1]) || !isalnum(*try)) )
					goto retry;
				if ( ((try + len) < (buf + size)) &&
				     (isalnum(try[len]) || !isalnum(try[len - 1])) )
					goto retry;
				return ( try );
retry:
				if ( --len )
					try = kwsexec(kwset, beg, len, &kwsmatch);
				else
					break;
				len = kwsmatch.size[0];
			}
		else
			return ( beg );
	}

	return ( NULL );
}

/* Hairy buffering mechanism to efficiently support all the options. */
static char *bufbeg;		/* Beginning of user-visible portion. */
static char *buflim;		/* Limit of user-visible portion. */
static char *buf;		/* Pointer to base of buffer. */
static size_t bufalloc;		/* Allocated size of buffer. */
static size_t bufcc;		/* Count of characters in buffer. */
static unsigned long int buftotalcc;
				/* Total character count since reset. */
static char *buflast;		/* Pointer after last character printed. */
static int bufgap;		/* Weird flag indicating buflast is a lie. */
static unsigned long int buftotalnl;
				/* Count of newlines before last character. */
static int bufpending;		/* Lines of pending output at buflast. */
static int bufdesc;		/* File descriptor to read from. */
static int bufeof;		/* Flag indicating EOF reached. */
static const char *buffile;	/* File name for messages. */

/* Scan and count the newlines prior to LIM in the buffer. */
static void
nlscan(lim)
char *lim;
{
	char *p;

	for ( p = buflast; p < lim; p++ )
		if ( *p == '\n' )
			buftotalnl++;
	buflast = lim;
}

/* Print the line beginning at BEG, using SEP to separate optional label
   fields from the text of the line.  Return the size of the line. */
static size_t
prline(beg, sep)
register char *beg;
register char sep;
{
  register size_t cc;
  register char c;
  static int err;
  
  cc = 0;

  if (out_silent || err)
    while (beg < buflim)
      {
	++cc;
	if (*beg++ == '\n')
	  break;
      }
  else
    {
      if (out_file)
	printf("%s%c", buffile, sep);
      if (out_line)
	{
	  nlscan(beg);
	  printf("%lu%c", buftotalnl + 1, sep);
	}
      if (out_byte)
	printf("%lu%c", buftotalcc + (beg - buf), sep);

      while (beg < buflim)
	{
	  ++cc;
	  c = *beg++;
#ifndef __MSDOS__
	  putchar(c);
#else
	  /* Convert any CR-LF sequences to \n */
	  if (c != '\r' || (beg < buflim && *beg != '\n'))
	    putchar(c);
#endif
	  if (c == '\n')
	    break;
	}
      if (ferror(stdout))
	{
	  error("output error", errno);
	  err = 1;
	}
    }

  if (out_line)
    nlscan(beg);
  else
    buflast = beg;
  bufgap = 0;

  return cc;
}

/* Print pending bytes of last trailing context prior to LIM. */
static void
prpending(lim)
register char *lim;
{
  while (buflast < lim && bufpending)
    {
      --bufpending;
      prline(buflast, '-');
    }
}

/* Print the lines between BEG and LIM.  Deal with context crap.
   Return the count of lines between BEG and LIM. */
static unsigned long
prtext(beg, lim)
char *beg;
char *lim;
{
	static int used;
	char *p;
	int i;
	unsigned long n;

	prpending(beg);

	p = beg;
	for ( i = 0; i < out_before; i++ )
		if ( p > buflast )
			do
				p--;
			while ( (p > buflast) && (p[-1] != '\n') );

	if ( (out_before || out_after) && used && ((p > buflast) || bufgap) )
		puts("--");

	while ( p < beg )
		p += prline(p, '-');

	n = 0;
	while ( p < lim )
	{
		n++;
		p += prline(p, ':');
	}

	bufpending = out_after;
	used = 1;

	return ( n );
}

/* Fill the user-visible portion of the buffer, returning a byte count. */
static int
fillbuf()
{
  register char *b, *d, *l;
  int i, cc;
  size_t discard, save;

  prpending(buflim);

  b = buflim;
  for (i = 0; i < out_before; ++i)
    if (b > buflast)
      do
	--b;
      while (b > buflast && b[-1] != '\n');

  if (buflast < b)
    bufgap = 1;
  if (out_line)
    nlscan(b);

  discard = b - buf;
  save = buflim - b;

  if (b > buf)
    {
      d = buf;
      l = buf + bufcc;
      while (b < l)
	*d++ = *b++;
    }

  bufcc -= discard;
  buftotalcc += discard;

  do
    {
      if (!bufeof)
	{
	  if (bufcc > bufalloc / 2)
	    buf = xrealloc(buf, bufalloc *= 2);
	  cc = read(bufdesc, buf + bufcc, bufalloc - bufcc);
	  if (cc < 0)
	    {
	      error(buffile, errno);
	      bufeof = 1;
	    }
	  else
	    {
	      bufeof = !cc;
	      bufcc += cc;
	    }
	}
      bufbeg = buf + save;
      for (l = buf + bufcc; l > bufbeg && l[-1] != '\n'; --l)
	;
      buflim = l;
      buflast = buf;
    }
  while (!bufeof && bufbeg == buflim);

  if (bufeof)
    buflim = buf + bufcc;

  return buflim - bufbeg;
}

/* One-time initialization. */
static void
initbuf()
{
  bufalloc = 8192;
  buf = xmalloc(bufalloc);
}

/* Reset the buffer for a new file. */
static void
resetbuf(desc, file)
int desc;
const char *file;
{
  bufbeg = buf;
  buflim = buf;
  bufcc = 0;
  buftotalcc = 0;
  buflast = buf;
  bufgap = 0;
  buftotalnl = 0;
  bufpending = 0;
  bufdesc = desc;
  bufeof = 0;
  buffile = file;
}

/* Scan the user-visible portion of the buffer, calling prtext() for
   matching lines (or between matching lines if OUT_INVERT is true).
   Return a count of lines printed. */
static unsigned long
grepbuf()
{
	unsigned long total;
	char *p;
	char *b;
	char *l;

	total = 0;
	p = bufbeg;
	while ( (b = execute(p, buflim - p)) != NULL )
	{
		if ( (b == buflim) &&
		     ((b > bufbeg) && (b[-1] == '\n') || (b == bufbeg)) )
			break;
		while ( (b > bufbeg) && (b[-1] != '\n') )
			b--;
		l = b + 1;
		while ( (l < buflim) && (l[-1] != '\n') )
			l++;
		if ( !out_invert )
			total += prtext(b, l);
		else if ( p < b )
			total += prtext(p, b);
		p = l;
	}
	if ( out_invert && (p < buflim) )
		total += prtext(p, buflim);
	return ( total );
}

/* Scan the given file, returning a count of lines printed. */
static unsigned long
grep(desc, file)
int desc;
const char *file;
{
	unsigned long total;

	total = 0;

	resetbuf(desc, file);

	while ( fillbuf() )
		total += grepbuf();

	return ( total );
}

static void
usage()
{
	fprintf(stderr,
"Usage: fgrep [-[AB]<num>] [-[CVchilnsvwx]] [-[ef]] <expr> [<files...>]\n");
	exit(2);
}

int
main(argc, argv)
int argc;
char *argv[];
{
	char *keys;
	size_t keycc;
	size_t keyalloc;
	int count_matches;
	int no_filenames;
	int list_files;
	int opt;
	int cc;
	int desc;
	int status;
	unsigned long count;
	FILE *fp;

	setmode(fileno(stdin), O_BINARY);
	setmode(fileno(stdout), O_TEXT);

	keys = NULL;
	count_matches = 0;
	no_filenames = 0;
	list_files = 0;

	while ( (opt = getopt(argc, argv, "0123456789A:B:CVbce:f:hilnsvwxy"))
								!= EOF )
		switch (opt)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			out_before = (10 * out_before) + (opt - '0');
			out_after = (10 * out_after) + (opt - '0');
			break;
		case 'A':
			out_after = atoi(optarg);
			if ( out_after < 0 )
				usage();
			break;
		case 'B':
			out_before = atoi(optarg);
			if ( out_before < 0 )
				usage();
			break;
		case 'C':
			out_before = out_after = 2;
			break;
		case 'V':
			fprintf(stderr, "GNU fgrep, version 1.1dos\n");
			break;
		case 'b':
			out_byte = 1;
			break;
		case 'c':
			out_silent = 1;
			count_matches = 1;
			break;
		case 'e':
			if ( keys )
				usage();
			keys = optarg;
			keycc = strlen(keys);
			break;
		case 'f':
			if ( keys )
				usage();
			fp = strcmp(optarg, "-") ? fopen(optarg, "r") : stdin;
			if (!fp)
				fatal(optarg, errno);
			/* This file has to be in text mode */
			setmode(fileno(fp), O_TEXT);
			keyalloc = 1024;
			keys = xmalloc(keyalloc);
			keycc = 0;
			while ( !feof(fp) &&
				((cc = fread(keys + keycc, 1, keyalloc - keycc,
								fp)) > 0) )
			{
				keycc += cc;
				if ( keycc == keyalloc )
					keys = xrealloc(keys, keyalloc *= 2);
			}
			if ( fp != stdin )
				fclose(fp);
			else
				setmode(fileno(stdin), O_BINARY);
			break;
		case 'h':
			no_filenames = 1;
			break;
		case 'i':
		case 'y':			/* For old-timers . . . */
			match_fold = 1;
			break;
		case 'l':
			out_silent = 1;
			list_files = 1;
			break;
		case 'n':
			out_line = 1;
			break;
		case 's':
			out_silent = 1;
			break;
		case 'v':
			out_invert = 1;
			break;
		case 'w':
			match_words = 1;
			break;
		case 'x':
			match_lines = 1;
			break;
		default:
			usage();
			break;
		}

	if ( !keys )
		if ( optind < argc )
		{
			keys = argv[optind++];
			keycc = strlen(keys);
		}
		else
			usage();

	compile(keys, keycc);

	if ( ((argc - optind) > 1) && !no_filenames )
		out_file = 1;

	status = 1;
	initbuf();

	if ( optind < argc )
	{
		while ( optind < argc )
		{
			desc = strcmp(argv[optind], "-") ?
						open(argv[optind], 0) : 0;
			if ( desc < 0 )
				error(argv[optind], errno);
			else
			{
				count = grep(desc, argv[optind]);
				if ( count_matches )
				{
					if ( out_file )
						printf("%s:", argv[optind]);
					printf("%lu\n", count);
				}
				if ( count )
				{
					status = 0;
					if ( list_files )
						printf("%s\n", argv[optind]);
				}
			}

			if ( desc )
				close(desc);
			optind++;
		}
	}
	else
	{
		count = grep(0, "<stdin>");
		if ( count_matches )
			printf("%lu\n", count);
		if ( count )
		{
			status = 0;
			if ( list_files )
				printf("%s\n", argv[optind]);
		}
	}

	exit(error_seen ? 2 : status);
}
