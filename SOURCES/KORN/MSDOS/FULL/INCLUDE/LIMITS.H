/*
 * Limiting values - Support for IBM C Set/2 and Microsoft C
 */

#if !defined(_LIMITS_H) && !defined (__limits_h)
#define __limits_h
#define _LIMITS_H

#ifndef _CHAR_UNSIGNED
#  define CHAR_MAX	127		/* maximum char value		*/
#  define CHAR_MIN	-127		/* mimimum char value		*/
#else
#  define CHAR_MAX	255
#  define CHAR_MIN	0
#endif

#define SCHAR_MAX        127            /* maximum signed char value	*/
#define SCHAR_MIN       -127            /* minimum signed char value	*/
#define UCHAR_MAX        255            /* maximum unsigned char value	*/
#define CHAR_BIT         8              /* number of bits in a char	*/

#define USHRT_MAX        0xffff         /* maximum unsigned short value */
#define SHRT_MAX         32767          /* maximum (signed) short value */
#define SHRT_MIN        -32767          /* minimum (signed) short value */
#define SHRT_BIT         16		/* number of bits in a short	*/

#if defined (MSDOS)
#  define UINT_MAX	0xffff		/* maximum unsigned int value	*/
#  define INT_MAX	32767		/* maximum (signed) int value	*/
#  define INT_MIN	-32767		/* minimum (signed) int value	*/
#  define INT_BIT	16		/* number of bits in a int	*/
#else
#  define UINT_MAX	0xffffffff	/* maximum unsigned int value	*/
#  define INT_MAX	2147483647	/* maximum (signed) int value	*/
#  define INT_MIN	-2147483647	/* minimum (signed) int value	*/
#  define INT_BIT	32		/* number of bits in a int	*/
#endif

#define ULONG_MAX        0xffffffff     /* maximum unsigned long value	*/
#define LONG_MAX         2147483647     /* maximum (signed) long value	*/
#define LONG_MIN        -2147483647     /* minimum (signed) long value	*/
#define LONG_BIT         32		/* number of bits in a long	*/

#if defined (OS2) || defined (__OS2__)
#  undef NAME_MAX
#  undef PATH_MAX
#  define NAME_MAX	256		/* Maximum file name length	*/
#  define PATH_MAX	256		/* Maximum path name length	*/
#elif defined (WIN32)
#  undef NAME_MAX
#  undef PATH_MAX
#  define PATH_MAX	260		/* max. length of full pathname */
#  define NAME_MAX	256		/* max. length of file name component */
#elif defined (MSDOS)
#  define NAME_MAX	13		/* Maximum file name length	*/
#  ifndef PATH_MAX
#    define PATH_MAX	128		/* Maximum path name length	*/
#  endif
#else
#  define NAME_MAX	256		/* Maximum file name length	*/
#  define PATH_MAX	1024		/* Maximum path name length	*/
#endif

#define FCHR_MAX	
#define LINK_MAX	32		/* Maximum number of links	*/
#define LOCK_MAX	0		/* Maximum number of locks	*/

#ifndef SYS_OPEN
#  ifdef MSDOS
#    define SYS_OPEN	20		/* Max # of files open by system*/
#  else
#    define SYS_OPEN	64
#  endif
#endif

#ifndef OPEN_MAX
#  define OPEN_MAX	20		/* Max # of files open by user	*/
#endif

#define STD_BLK		1024		/* Standard block size		*/
#define PIPE_MAX	5120		/* Maximum pipe size		*/
#define PIPE_BUF	PIPE_MAX

#ifndef TMP_MAX
#define TMP_MAX		32767		/* Max # of tempory files	*/
#endif

#define PID_MAX		32000		/* Process id max		*/
#define PROC_MAX
#define SYSPID_MAX	
#define UID_MAX		30000
#define MAX_CHAR
#define PASS_MAX	8		/* MAX password length		*/
#define SYS_NMLN	9
#define MAX_CANON	255
#define MAX_INPUT	255
#define ARG_MAX		2048
#define CHILD_MAX	30000

#if !defined (CLK_TCK) && !defined (__OS2__) && !defined (__TURBOC__) && !defined (__WATCOMC__) && !defined (WIN32)
#  define CLK_TCK	60
#endif

#define NGROUPS_MAX	0		/* Max number of groups per p.	*/
#define MAX_GROUPS	100		/* Max users per group		*/

#ifdef MSDOS
#  define MB_LEN_MAX	1		/* Max # bytes in multibyte	*/
					/* character			*/
#else
#  define MB_LEN_MAX	2
#endif

#define _POSIX_ARG_MAX		ARG_MAX
#define _POSIX_CHILD_MAX	CHILD_MAX
#define _POSIX_LINK_MAX		LINK_MAX
#define _POSIX_MAX_CANON	MAX_CANON
#define _POSIX_MAX_INPUT	MAX_INPUT
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	NGROUPS_MAX
#define _POSIX_OPEN_MAX		OPEN_MAX
#define _POSIX_PATH_MAX		PATH_MAX
#define _POSIX_PIPE_BUF		PIPE_BUF
#endif
