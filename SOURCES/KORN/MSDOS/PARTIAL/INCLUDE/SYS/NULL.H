/*
 * Define NULL
 */

#ifndef NULL
#  ifdef MSDOS
#    if (_MSC_VER >= 600)
#      define NULL	((void *)0)

#    elif (defined(M_I86SM) || defined(M_I86MM))
#      define NULL	0

#    else
#      define NULL	0L
#    endif
#  elif defined(__STDC__)
#      define NULL	((void *)0)
#  else
#      define NULL	0
#  endif
#endif
