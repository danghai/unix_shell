/* <sys/dirent.h> -- file system independent directory entry (SVR3) */
#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
#  if defined (OS2) || defined (__OS2__) || defined (WIN32)
#    define MAXNAMLEN	255	/* maximum filename length		*/
#  else
#    define MAXNAMLEN	13	/* maximum filename length		*/
#  endif
#else
#  define MAXNAMLEN	512	/* maximum filename length		*/
#endif

#ifndef NAME_MAX
#define	NAME_MAX	(MAXNAMLEN - 1)
#endif

struct dirent			/* data from getdents()/readdir()	*/
{
    ino_t	d_ino;		/* inode number of entry		*/
    off_t	d_off;		/* offset of disk directory entry	*/
    ushort	d_reclen;	/* length of this record		*/
#if defined (MSDOS) || defined (__OS2__) || defined (__TURBOC__) || defined (WIN32)
    char	d_name[MAXNAMLEN + 1];
#else
    char	d_name[1];	/* name of file				*/
#endif
};

#ifdef BSD_SYSV			/* (e.g., when compiling getdents.c)	*/
extern struct dirent	__dirent;	/* (not actually used) */

				/* The following is portable, although	*/
				/* rather silly.			*/
#define	DIRENTBASESIZ		(__dirent.d_name - (char *)&__dirent.d_ino)

#else

/* The following nonportable ugliness could have been avoided by defining
 * DIRENTSIZ and DIRENTBASESIZ to also have (struct dirent *) arguments.
 * There shouldn't be any problem if you avoid using the DIRENTSIZ() macro.
 */

#define	DIRENTBASESIZ		(((struct dirent *)0)->d_name \
				- (char *)&((struct dirent *)0)->d_ino)
#endif

#define	DIRENTSIZ(namlen)	((DIRENTBASESIZ + sizeof(long) + (namlen)) \
				/ sizeof(long) * sizeof(long))

#endif
