#ifndef _NL_TYPES_H
#define _NL_TYPES_H

#include <sys/proto.h>

/* nl_types.h - NLS Data Types */

#define NL_SETD		1	/* Default set number		*/
#define NL_ARGMAX	20	/* Max args to printf		*/

typedef long	nl_catd;	/* Catalogue file ID		*/
typedef int	nl_item;	/* nl_langinfo item		*/

/* Function definitions */

extern char * _CDECL	nl_langinfo	_PROTO ((nl_item));
extern int _CDECL	nl_init		_PROTO ((void));
extern nl_catd _CDECL	catopen		_PROTO ((char *, int));
extern int _CDECL	catclose	_PROTO ((nl_catd));
extern char * _CDECL	catgets		_PROTO ((nl_catd, int, int, char *));
extern char * _CDECL	catgetmsg	_PROTO ((nl_catd, int, int, char *,
						 int));
#endif
