#ifndef _LANGINFO_H
#define _LANGINFO_H

/*
 *		LANGINFO.H
 *
 * langinfo.h - NLS Language information items
 *
 */

#include <sys/proto.h>

/* String Maxinum lengths */

#define NL_LDAY		12		/* Short month string length	*/
#define NL_SDAY		3		/* Short month string length	*/
#define NL_LMON		12		/* Short month string length	*/
#define NL_SMON		4		/* Short month string length	*/
#define NL_YESNO	6		/* Yes/No string length		*/

/* nl_langinfo items which can be got */

#define D_T_FMT		0		/* Get date/time string		*/

/* Week day names */

#define DAY_1		1		/* First day of week		*/
#define DAY_2		2
#define DAY_3		3
#define DAY_4		4
#define DAY_5		5
#define DAY_6		6
#define DAY_7		7		/* Seventh day of week		*/

/* Abbreviated week day names */

#define ABDAY_1		11		/* First day of week		*/
#define ABDAY_2		12
#define ABDAY_3		13
#define ABDAY_4		14
#define ABDAY_5		15
#define ABDAY_6		16
#define ABDAY_7		17		/* Seventh day of week		*/

/* Month names */

#define MON_1		21		/* First month			*/
#define MON_2		22
#define MON_3		23
#define MON_4		24
#define MON_5		25
#define MON_6		26
#define MON_7		27
#define MON_8		28
#define MON_9		29
#define MON_10		30
#define MON_11		31
#define MON_12		32		/* Twelfth month		*/

/* Abbreviated Month names */

#define ABMON_1		41		/* First month			*/
#define ABMON_2		42
#define ABMON_3		43
#define ABMON_4		44
#define ABMON_5		45
#define ABMON_6		46
#define ABMON_7		47
#define ABMON_8		48
#define ABMON_9		49
#define ABMON_10	50
#define ABMON_11	51
#define ABMON_12	52		/* Twelfth month		*/

/* Miscellaneous */

#define RADIXCHAR	60		/* Radix character		*/
#define THOUSEP		61		/* Thousand separator		*/
#define YESSTR		62		/* Yes string			*/
#define NOSTR		63		/* NO string			*/
#define CRNCYST		64		/* Currency symbol		*/
#define TIMEAM		66		/* AM symbol			*/
#define TIMEPM		67		/* PM symbol			*/

extern char * _CDECL	nl_langinfo	_PROTO ((nl_item));
#endif
