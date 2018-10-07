/*
 * glob - expand file name function
 */

#ifndef GLOB_H
#define GLOB_H

#include <sys/proto.h>

typedef struct glob_t {
    int		gl_flags;		/* Saved flag value		*/
    int		gl_offs;		/* Offset into paths to start	*/
    int		gl_pathc;		/* Number of entries		*/
					/* Function			*/
    int		(_CDECL *gl_ef) _PROTO ((char *, int));
    char	**gl_pathv;		/* Pointers to path		*/
} glob_t;

#define	GLOB_ERR	0x0001		/* Stop on error		*/
#define GLOB_MARK	0x0002		/* Mark directories		*/
#define	GLOB_NOSORT	0x0004		/* No sorting			*/
#define GLOB_NOCHECK	0x0008		/* No match returns pattern	*/
#define GLOB_DOOFFS	0x0010		/* Add null pointers		*/
#define GLOB_APPEND	0x0020		/* Append file names		*/

#define GLOB_ABEND	-1		/* Error			*/
#define GLOB_NOSPACE	-2		/* Out of space			*/

extern int _CDECL	glob	 _PROTO ((char *, int,
					  int (_CDECL *)(char *,
							 int), glob_t *));
extern void _CDECL	globfree _PROTO ((glob_t *));
#endif
