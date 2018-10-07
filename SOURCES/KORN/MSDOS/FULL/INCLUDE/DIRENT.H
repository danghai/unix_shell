/*
 * dirent.h:  definitions for SVR3 directory access routines
 */

#ifndef _DIRENT_H
#  define _DIRENT_H

#  include <sys/types.h>
#  include <limits.h>
#  include <sys/dirent.h>
#  include <sys/proto.h>
#  include <sys/null.h>

/* Need size_t definition */

#  if !defined(_SIZE_T_DEFINED) && !defined(__size_t) && !defined(_SIZE_T) && !defined(_SIZE_T_DEFINED_) 

typedef unsigned int	size_t;

#    define _SIZE_T_DEFINED
#    define _SIZE_T_DEFINED_
#    define _SIZE_T
#    define __size_t
#  endif

#  ifndef _BOOL_T_DEFINED
typedef unsigned char	bool;
#  define _BOOL_T_DEFINED
#  endif

#  define DIRBUF	8192	/* buffer size for fs-indep. dirs	*/
				/* must in general be larger than the	*/
				/* filesystem buffer size		*/

#  if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32) || defined (__EMX__)

/* MSDOS versions and OS2 ?.x version */

struct _dircontents {
    char		*_d_entry;
    struct _dircontents	*_d_next;
};

typedef struct _dirdesc {
    int			dd_id;	/* uniquely identify each open directory */
    long		dd_loc;	/* where we are in directory entry is this */
    struct _dircontents	*dd_contents;	/* pointer to contents of dir	*/
    struct _dircontents	*dd_cp;		/* pointer to current position	*/
} DIR;

#  else

/* UNIX versions */

typedef struct
{
    int		dd_fd;		/* file descriptor			*/
    int		dd_loc;		/* offset in block			*/
    int		dd_size;	/* amount of valid data			*/
    char	*dd_buf;	/* -> directory block			*/
} DIR;				/* stream data from opendir()		*/

#  endif

/* Functions */

extern DIR _FAR_ * _FAR_ _CDECL	opendir	_PROTO ((const char _FAR_ *));
extern struct dirent _FAR_ * _FAR_ _CDECL readdir _PROTO ((DIR _FAR_ *));
extern void _FAR_ _CDECL	rewinddir _PROTO ((DIR _FAR_ *));

#  if !defined (MSDOS) && !defined (__OS2__) && !defined (WIN32)
extern int _FAR_ _CDECL		getdents _PROTO ((int, char _FAR_ *,
						  unsigned int));
#  endif

extern int _FAR_ _CDECL		closedir _PROTO ((DIR _FAR_ *));
extern void _FAR_ _CDECL	seekdir	_PROTO ((DIR _FAR_ *, off_t));
extern off_t _FAR_ _CDECL	telldir	_PROTO ((DIR _FAR_ *));

extern int _FAR_ _CDECL		chdir _PROTO ((const char _FAR_ *));
extern char _FAR_ * _FAR_ _CDECL getcwd _PROTO ((char _FAR_ *, size_t));

#  ifdef __TURBOC__
extern int _FAR_ _CDECL		mkdir _PROTO ((const char _FAR_ *));
#  elif defined (__32BIT__) && defined (__EMX__)
extern int _FAR_ _CDECL		mkdir _PROTO ((const char _FAR_ *, long));
#  else
extern int _FAR_ _CDECL		mkdir _PROTO ((const char _FAR_ *, mode_t));
#  endif

extern int _FAR_ _CDECL		rmdir _PROTO ((const char _FAR_ *));
extern int _FAR_ _CDECL		scandir _PROTO ((char _FAR_ *,
						 struct dirent ***,
						 int (_FAR_ _CDECL *)(const void _FAR_ *, const void _FAR_ *),
						 int (_FAR_ _CDECL *)(const void _FAR_ *, const void _FAR_ *)));

#  if defined (MSDOS) || defined (__OS2__) || defined (WIN32)
extern int _FAR_ _CDECL		_chdrive _PROTO ((int));
extern int _FAR_ _CDECL		_getdrive _PROTO ((void));
extern char _FAR_ * _FAR_ _CDECL _getdcwd _PROTO ((int, char _FAR_ *, int));
#  endif

#  if defined (OS2) || defined (__OS2__) || defined (WIN32) || defined (__EMX__)
extern bool			IsHPFSFileSystem _PROTO ((char _FAR_ *));
#  endif
#endif
