/*
 * Prototype definitions for Standard and Non-standard compilers
 */

#ifndef _PROTO

#define _NEAR
#define _CDECL
#define _FAR_
#define _LOADDS_

#if defined (MSDOS) && !defined (WIN32)

#  if defined(_DLL) && !defined(_MT)
#    error Cannot define _DLL without _MT
#  endif

#  ifdef _DLL
#    undef _LOADDS_
#    define _LOADDS_	_loadds
#  endif

#  ifdef _MT
#    undef _FAR_
#    define _FAR_	_far
#  endif

#  ifndef __STDC__
#    define __STDC__	1
#  endif

#  ifndef __WATCOMC__
#    undef _CDECL
#    define _CDECL	cdecl
#  endif

#  undef _NEAR
#  define _NEAR		near
#endif

#if defined (WIN32)
#  undef _CDECL
#  define _CDECL	__cdecl
#endif

#ifdef __TURBOC__
#  undef _CDECL
#  define _CDECL	_Cdecl
#endif

#if defined (__STDC__) || defined (__TURBOC__)
#  define _PROTO(p)	p
#else
#  define _PROTO(p)	()
#  undef  const
#  undef  volatile
#endif
#endif
