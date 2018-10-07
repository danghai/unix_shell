#ifndef _UNISTD_H
#define _UNISTD_H

/*  unistd.h  */

#include <sys/types.h>
#include <sys/proto.h>
#include <sys/null.h>

/*  for access(2)  */

#define R_OK     	4
#define W_OK     	2
#define X_OK     	1
#define F_OK     	0

/*  for lockf()  */

#define F_ULOCK		0
#define F_LOCK		1
#define F_TLOCK		2
#define F_TEST		3

/*  for lseek(2)  */

#ifndef SEEK_SET
#define SEEK_SET	0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR	1
#endif

#ifndef SEEK_END
#define SEEK_END	2
#endif

/* STDIO definitions */

#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

/* Standard paths */

#if defined (MSDOS) || defined (__OS2__)
#define GF_PATH		"c:/etc/group"
#define PF_PATH		"c:/etc/passwd"
#define IF_PATH		"c:/include"
#define SF_PATH		"c:/etc/shadow"
#else
#define GF_PATH		"/etc/group"
#define PF_PATH		"/etc/passwd"
#define IF_PATH		"/usr/include"
#define SF_PATH		"/etc/shadow"
#endif

/* POSIX definitions */

#define _POSIX_VERSION		198803L
#undef	_POSIX_CHOWN_RESTRICTED
#undef	_POSIX_JOB_CONTROL
#define	_POSIX_NO_TRUNC		1
#define _POSIX_SAVED_IDS	1
#undef	_POSIX_VDISABLE
/* #define _XOPEN_VERSION		*/

/* sysconf values */

#define _SC_ARG_MAX		0
#define _SC_CHILD_MAX		1
#define _SC_CLK_TCK		2
#define _SC_NGROUPS_MAX		3
#define _SC_OPEN_MAX		4
#define _SC_JOB_CONTROL		5
#define _SC_SAVED_IDS		6
#define _SC_VERSION		7
#define _SC_PASS_MAX		8
#define _SC_XOPEN_VERSION	9

/* pathconf values */

#define _PC_LINK_MAX		0
#define _PC_MAX_CANON		1
#define _PC_MAX_INPUT		2
#define _PC_NAME_MAX		3
#define _PC_PATH_MAX		4
#define _PC_PIPE_BUF		5
#define _PC_CHOWN_RESTRICTED	6
#define _PC_NO_TRUNC		7
#define _PC_VDISABLE		8

/* confstring values */

#define _CS_PATH		1

/* WIN NT is a real pain!! */

#ifdef WIN32
#  define access	_access
#  define chdir		_chdir
#  define chsize	_chsize
#  define close		_close
#  define creat		_creat
#  define cwait		_cwait
#  define dup		_dup
#  define dup2		_dup2
#  define eof		_eof
#  define execl		_execl
#  define execle	_execle
#  define execlp	_execlp
#  define execlpe	_execlpe
#  define execv		_execv
#  define execve	_execve
#  define execvp	_execvp
#  define execvpe	_execvpe
#  define fcloseall	_fcloseall
#  define fdopen	_fdopen
#  define filelength	_filelength
#  define getcwd	_getcwd
#  define getpid	_getpid
#  define isatty	_isatty
#  define locking	_locking
#  define lseek		_lseek
#  define mktemp	_mktemp
#  define onexit	_onexit
#  define onexit_t	_onexit_t
#  define open		_open
#  define read		_read
#  define rmdir		_rmdir
#  define setmode	_setmode
#  define sopen		_sopen
#  define spawnl	_spawnl
#  define spawnle	_spawnle
#  define spawnlp	_spawnlp
#  define spawnlpe	_spawnlpe
#  define spawnv	_spawnv
#  define spawnve	_spawnve
#  define spawnvp	_spawnvp
#  define spawnvpe	_spawnvpe
#  define strdup	_strdup
#  define tell		_tell
#  define umask		_umask
#  define unlink	_unlink
#  define wcsdup	_wcsdup
#  define write		_write
#endif

/* Function declarations */

extern size_t _FAR_ _CDECL	confstring _PROTO ((int, char _FAR_ *, size_t));

#ifndef __WATCOMC__
extern void _FAR_ _CDECL	abort _PROTO ((void));
#endif

#if defined (MSDOS) || defined (__OS2__)
extern int _FAR_ _CDECL		chsize _PROTO ((int, off_t));
#else
extern int _FAR_ _CDECL		chsize _PROTO ((const char _FAR_ *, off_t));
#endif

extern void _FAR_ _CDECL	sync _PROTO ((void));

/* --- Process creation and execution --- */

extern pid_t _FAR_ _CDECL	fork _PROTO ((void));
extern pid_t _FAR_ _CDECL	vfork _PROTO ((void));

extern int _FAR_ _CDECL	execl	_PROTO ((const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	execle	_PROTO ((const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	execlp	_PROTO ((const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	execlpe	_PROTO ((const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	execv	_PROTO ((const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	execve	_PROTO ((const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	execvp	_PROTO ((const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	execvpe	_PROTO ((const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
extern int _FAR_ _CDECL	spawnl	_PROTO ((int, const char *,
					 const char *, ...));

extern int _FAR_ _CDECL	spawnle	_PROTO ((int, const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	spawnlp	_PROTO ((int, const char _FAR_ *,
					 const char _FAR_ *, ...));

extern int _FAR_ _CDECL	spawnlpe _PROTO ((int, const char _FAR_ *,
					  const char _FAR_ *, ...));

extern int _FAR_ _CDECL	spawnv	_PROTO ((int, const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	spawnve	_PROTO ((int, const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	spawnvp	_PROTO ((int, const char _FAR_ *,
					 const char _FAR_ * const _FAR_ *));

extern int _FAR_ _CDECL	spawnvpe _PROTO ((int, const char _FAR_ *,
					  const char _FAR_ * const _FAR_ *,
					  const char _FAR_ * const _FAR_ *));

/* Overlay functions for MSDOS */

extern int _NEAR _FAR_ _CDECL	_p_overlay;

#define P_WAIT		0
#define P_OVERLAY	_p_overlay
#define OLD_P_OVERLAY	2
#if defined (OS2) || defined (__OS2__) || defined (WIN32)
#define P_NOWAIT	1
#define P_NOWAITO	3
#define P_DETACH	4

#define WAIT_CHILD	0
#define WAIT_GRANDCHILD 1

extern int _FAR_ _CDECL cwait _PROTO ((int _FAR_ *, int, int));
extern int _FAR_ _CDECL wait _PROTO ((int _FAR_ *));
#endif
#endif

/*
 * Process termination
 */

#ifndef __WATCOMC__
extern void _FAR_ _CDECL	_exit _PROTO ((int));
extern void _FAR_ _CDECL	exit _PROTO ((int));
#endif

/*
 * Timer operations
 */

extern unsigned int _FAR_ _CDECL	alarm _PROTO ((unsigned int));
extern int _FAR_ _CDECL			pause _PROTO ((void));

#if !defined (__WATCOMC__)
#  if defined (MSDOS) || defined (__TURBOC__)
extern void  _FAR_ _CDECL		sleep _PROTO ((unsigned int));
#  else
extern unsigned int _FAR_ _CDECL	sleep _PROTO ((unsigned int));
#  endif
#endif

/*
 * Process identification
 */

extern pid_t _FAR_ _CDECL	getpid _PROTO ((void));
extern pid_t _FAR_ _CDECL	getppid _PROTO ((void));

/*
 * User identification
 */

extern uid_t _FAR_ _CDECL		getuid _PROTO ((void));
extern uid_t _FAR_ _CDECL		geteuid _PROTO ((void));
extern gid_t _FAR_ _CDECL		getgid _PROTO ((void));
extern gid_t _FAR_ _CDECL		getegid _PROTO ((void));
extern int _FAR_ _CDECL			setuid _PROTO ((uid_t));
extern int _FAR_ _CDECL			setgid _PROTO ((gid_t));
extern int _FAR_ _CDECL			getgroups _PROTO ((int, gid_t *));
extern char _FAR_ * _FAR_ _CDECL	getlogin _PROTO ((void));
extern char _FAR_ * _FAR_ _CDECL	cuserid _PROTO ((char _FAR_ *));

/*
 * Process groups
 */

extern pid_t _FAR_ _CDECL		getpgrp _PROTO ((void));
extern pid_t _FAR_ _CDECL		setsid _PROTO ((void));
extern int _FAR_ _CDECL			setpgid _PROTO ((pid_t, pid_t));

/*
 * Terminal identification
 */

extern char _FAR_ * _FAR_ _CDECL	ctermid _PROTO ((char _FAR_ *));
extern char _FAR_ * _FAR_ _CDECL	ttyname _PROTO ((int));
extern int _FAR_ _CDECL			isatty _PROTO ((int));

/*
 * Configurable system variables
 */

extern long _FAR_ _CDECL	sysconf _PROTO ((int));

/*
 * Working directory
 */

extern int _FAR_ _CDECL			chdir _PROTO ((const char _FAR_ *));
extern char _FAR_ * _FAR_ _CDECL	getcwd _PROTO ((char _FAR_ *, size_t));

/*
 * General file creation
 */

extern int _FAR_ _CDECL		link _PROTO ((const char _FAR_ *,
					      const char _FAR_ *));
#ifndef __WATCOMC__
extern int _FAR_ _CDECL		rename _PROTO ((const char _FAR_ *,
						const char _FAR_ *));
#endif
extern char _FAR_ * _FAR_ _CDECL mktemp _PROTO ((char _FAR_ *));

/*
 * File removal
 */

extern int _FAR_ _CDECL		unlink _PROTO ((const char _FAR_ *));
#ifndef __WATCOMC__
extern int _FAR_ _CDECL		remove _PROTO ((const char _FAR_ *));
#endif
extern int _FAR_ _CDECL		rmdir _PROTO ((const char _FAR_ *));

/*
 * File characteristics
 */

extern int _FAR_ _CDECL		access _PROTO ((const char _FAR_ *, int));
extern int _CDECL		chown _PROTO ((const char _FAR_ *, uid_t, gid_t));
extern long _FAR_ _CDECL	tell _PROTO ((int));
extern int _FAR_ _CDECL		eof _PROTO ((int));
extern long _FAR_ _CDECL	filelength _PROTO ((int));
extern mode_t _FAR_ _CDECL	umask _PROTO ((mode_t));

/*
 * Configurable pathname variables
 */

extern long _FAR_ _CDECL	pathconf _PROTO ((char _FAR_ *, int));
extern long _FAR_ _CDECL	fpathconf _PROTO ((int, int));

/*
 * Pipes
 */

extern int _FAR_ _CDECL		pipe _PROTO ((int[2]));

/*
 * File descriptor manipulation
 */

extern int _FAR_ _CDECL		dup _PROTO ((int));
extern int _FAR_ _CDECL		dup2 _PROTO ((int, int));

/*
 * File descriptor assignment
 */

#ifndef __WATCOMC__
extern int _FAR_ _CDECL	open _PROTO ((const char _FAR_ *, int, ...));
extern int _FAR_ _CDECL	sopen _PROTO ((const char _FAR_ *, int, int, ...));
#endif
extern int _FAR_ _CDECL	creat _PROTO ((const char _FAR_ *, mode_t));
extern int _FAR_ _CDECL	setmode _PROTO ((int, int));

/*
 * File descriptor deassignment
 */

extern int _FAR_ _CDECL	close _PROTO ((int));

/*
 * Input and output
 */

extern int _FAR_ _CDECL	read _PROTO ((int, const void _FAR_ *, unsigned int));
extern int _FAR_ _CDECL	write _PROTO ((int, const void _FAR_ *, unsigned int));

/*
 * Control operations on files
 */

extern off_t _FAR_ _CDECL	lseek _PROTO ((int, off_t, int));
extern int _FAR_ _CDECL 	locking _PROTO ((int, int, long));
#endif
