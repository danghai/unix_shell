/*
 * sys/types include file.  Supports MSC and IBM C Set/2
 */

#ifndef __types_h
#  define __types_h
#  define _ANSI_H_				/* Stop GCC		*/

#  if !defined(__ino_t) && !defined(_INO_T_DEFINED)
typedef unsigned short ino_t;
typedef unsigned short _ino_t;		/* i-node number (not used on DOS) */
#    define _INO_T_DEFINED
#    define __ino_t
#  endif

#  if !defined(__time_t) && !defined(_TIME_T_DEFINED) && !defined(_TIME_T) && !defined(_TIME_T_DEFINED_)
typedef long		 time_t;
#    define __time_t
#    define _TIME_T_DEFINED
#    define _TIME_T_DEFINED_
#    define _TIME_T
#  endif

#  if !defined(__dev_t) && !defined(_DEV_T_DEFINED)
typedef	short dev_t;
typedef short _dev_t;			/* device code */
#    define __dev_t
#    define _DEV_T_DEFINED
#  endif

#  if !defined(__off_t) && !defined(_OFF_T_DEFINED)
typedef long off_t;
typedef long _off_t;
#    define __off_t
#    define _OFF_T_DEFINED
#  endif

#  if !defined(_SIZE_T_DEFINED) && !defined(__size_t) && !defined(_SIZE_T) && !defined(_SIZE_T_DEFINED_) 

#    ifdef __GNUC__
typedef long unsigned int	size_t;
#    else
typedef unsigned int		size_t;
#    endif

#    define _SIZE_T_DEFINED
#    define _SIZE_T_DEFINED_
#    define _SIZE_T
#    define __size_t
#  endif

#  if !defined (_CLOCK_T_DEFINED) && !defined (_CLOCK_T)
typedef long clock_t;
#    define _CLOCK_T_DEFINED
#    define _CLOCK_T
#  endif

/*
 * IS additions
 */

#  ifndef _BOOL_T_DEFINED
typedef unsigned char	bool;	/* Boolean: 0 = false, 1 = true		*/
#    define _BOOL_T_DEFINED
#  endif


typedef unsigned short	ushort;	/* 2-byte unsigned			*/
typedef unsigned long	ulong;	/* 4-byte unsigned			*/
typedef unsigned int	uint;	/* unsigned				*/
typedef ushort		u_short;
#  ifdef __GNUC__
typedef unsigned char	u_char;
typedef unsigned long	u_long;
#  endif
typedef ushort		mode_t;
typedef int		pid_t;
typedef ushort		uid_t;
typedef ushort		gid_t;
typedef short		nlink_t;

/*
 * System Constants
 */

#  ifndef FALSE
#    define FALSE	((bool)0)	/* Boolean 'false'		*/
#  endif
#  ifndef TRUE
#    define TRUE	((bool)1)	/* Boolean 'true'		*/
#  endif

#  ifdef __GNUC__
#    define	_VA_LIST_	char *			/* va_list */
#    define	_WCHAR_T_	unsigned short		/* wchar_t */
#    define	_PTRDIFF_T_	int			/* ptr1 - ptr2 */
#  endif

#endif
