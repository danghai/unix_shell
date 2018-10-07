/*
 * dirent.h:  definitions for SVR3 directory access routines
 */

#ifndef _DIRENT_H
#  define _DIRENT_H

#  include <sys/types.h>
#  include <sys/dirent.h>

#  define _CDECL
#  define _FAR_

#  if defined (MSDOS) && !defined (WIN32)
#    ifndef __STDC__
#      define __STDC__	1
#    endif

#    ifndef __WATCOMC__
#      undef _CDECL
#      define _CDECL	cdecl
#    endif
#  endif

#  if defined (WIN32)
#    undef _CDECL
#    define _CDECL	__cdecl
#  endif

#  ifdef __TURBOC__
#    undef _CDECL
#    define _CDECL	_Cdecl
#  endif


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

#  if defined (OS2) || defined (__OS2__) || defined (__EMX__)
#    undef NAME_MAX
#    undef PATH_MAX
#    define NAME_MAX	256		/* Maximum file name length	*/
#    define PATH_MAX	256		/* Maximum path name length	*/
#  elif defined (WIN32)
#    undef NAME_MAX
#    undef PATH_MAX
#    define PATH_MAX	260		/* max. length of full pathname */
#    define NAME_MAX	256		/* max. length of file name component */
#  elif defined (MSDOS)
#    define NAME_MAX	13		/* Maximum file name length	*/
#    ifndef PATH_MAX
#      define PATH_MAX	128		/* Maximum path name length	*/
#    endif
#  else
#    undef NAME_MAX
#    undef PATH_MAX
#    define NAME_MAX	256		/* Maximum file name length	*/
#    define PATH_MAX	1024		/* Maximum path name length	*/
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

extern DIR _FAR_ * _FAR_ _CDECL	opendir	(const char _FAR_ *);
extern struct dirent _FAR_ * _FAR_ _CDECL readdir (DIR _FAR_ *);
extern void _FAR_ _CDECL	rewinddir (DIR _FAR_ *);

#  if !defined (MSDOS) && !defined (__OS2__) && !defined (WIN32)
extern int _FAR_ _CDECL		getdents (int, char _FAR_ *, unsigned int);
#  endif

extern int _FAR_ _CDECL		closedir (DIR _FAR_ *);
extern void _FAR_ _CDECL	seekdir	(DIR _FAR_ *, off_t);
extern off_t _FAR_ _CDECL	telldir	(DIR _FAR_ *);

extern int _FAR_ _CDECL		chdir (const char _FAR_ *);
extern char _FAR_ * _FAR_ _CDECL getcwd (char _FAR_ *, size_t);

#  ifdef __TURBOC__
extern int _FAR_ _CDECL		mkdir (const char _FAR_ *);
#  elif defined (__32BIT__) && defined (__EMX__)
extern int _FAR_ _CDECL		mkdir (const char _FAR_ *, long);
#  else
extern int _FAR_ _CDECL		mkdir (const char _FAR_ *, mode_t);
#  endif

extern int _FAR_ _CDECL		rmdir (const char _FAR_ *);
extern int _FAR_ _CDECL		scandir (char _FAR_ *,
					 struct dirent ***,
					 int (_FAR_ _CDECL *)(const void _FAR_ *, const void _FAR_ *),
					 int (_FAR_ _CDECL *)(const void _FAR_ *, const void _FAR_ *));

#  if defined (MSDOS) || defined (__OS2__) || defined (WIN32)
extern int _FAR_ _CDECL		_chdrive (int);
extern int _FAR_ _CDECL		_getdrive (void);
extern char _FAR_ * _FAR_ _CDECL _getdcwd (int, char _FAR_ *, int);
#  endif

#  if defined (OS2) || defined (__OS2__) || defined (WIN32) || defined (__EMX__)
extern bool			IsHPFSFileSystem (char _FAR_ *);
#  endif
#endif
