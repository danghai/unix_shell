/* MS-DOS SHELL - Show Scan codes
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited.
 *
 * This code is subject to the following copyright restrictions:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/showkey.c,v 2.5 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: showkey.c,v $
 * Revision 2.5  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 2.4  1994/02/01  10:25:20  istewart
 * Release 2.3 Beta 2, including first NT port
 *
 * Revision 2.3  1994/01/11  17:55:25  istewart
 * Release 2.3 Beta 0 patches
 *
 * Revision 2.2  1993/06/14  10:59:58  istewart
 * More changes for 223 beta
 *
 * Revision 2.1  1993/06/02  09:54:34  istewart
 * Beta 223 Updates - see Notes file
 *
 * Revision 2.0  1992/07/16  14:35:08  istewart
 * Release 2.0
 *
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#if defined (OS2) || defined (__OS2__)
#  define INCL_KBD
#  include <os2.h>

#  if defined (__OS2__)
#    include <bsedev.h>
#  endif

#elif (WIN32)
#  include <windows.h>
WORD IgnoreScanCode[] = { 0x1d, 0x38, 0x2a, 0x36, 0x3a, 0x45 };
#define IGNORE_CNT	(sizeof (IgnoreScanCode) / sizeof (IgnoreScanCode[0]))
#else
#  include <dos.h>

extern void	CheckExtendedKeyboard (void);
unsigned char	KeyInt = 0;
#endif

void main (void)
{
#if defined (OS2) || defined (__OS2__)
    KBDKEYINFO		kbci;
    KBDINFO		kbstInfo;
    unsigned		rc;
#elif defined (WIN32)
    INPUT_RECORD	Buffer;
    DWORD		NumberOfEventsRead;
    int			i;
#else
    union REGS	Key;
    union REGS	Shift;
#endif

#if defined (OS2) || defined (__OS2__)
    kbstInfo.cb = sizeof (kbstInfo);
    if (rc = KbdGetStatus (&kbstInfo, 0))	/* get current status	*/
    {
         fprintf (stderr, "KbdGetStatus failed - %d\n", rc);
	 exit (1);
    }

    kbstInfo.fsMask = (kbstInfo.fsMask &
				~(KEYBOARD_ASCII_MODE | KEYBOARD_ECHO_ON)) |
		       (KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE);

    if (rc = KbdSetStatus (&kbstInfo, 0))
         fprintf (stderr, "KbdSetStatus failed - %d\n", rc);
#elif defined (WIN32)
    SetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), 0);
#else
    CheckExtendedKeyboard ();
#endif

    puts ("Control C to terminate");

    while (TRUE)
    {
#if defined (OS2) || defined (__OS2__)

	KbdCharIn (&kbci, IO_WAIT, 0);

	printf ("Scan = 0x%.4x ", kbci.chScan);
	printf ("ASCII = 0x%.4x (%c) ", kbci.chChar, isprint (kbci.chChar) ?
		kbci.chChar : '.');

	printf ("Shift = 0x%.4x ( ", kbci.fsState);

	if (kbci.fsState & RIGHTSHIFT)
	    printf ("RIGHTSHIFT ");

	if (kbci.fsState & LEFTSHIFT)
	    printf ("LEFTSHIFT ");

	if (kbci.fsState & ALT)
	    printf ("ALT ");

	if (kbci.fsState & LEFTALT)
	    printf ("LEFTALT ");

	if (kbci.fsState & RIGHTALT)
	    printf ("RIGHTALT ");

	if (kbci.fsState & CONTROL)
	    printf ("CONTROL ");

	if (kbci.fsState & LEFTCONTROL)
	    printf ("LEFTCONTROL ");

	if (kbci.fsState & RIGHTCONTROL)
	    printf ("RIGHTCONTROL ");

	if (kbci.fsState & SCROLLLOCK_ON)
	    printf ("SCROLLLOCK_ON ");

	if (kbci.fsState & SCROLLLOCK)
	    printf ("SCROLLLOCK ");

	if (kbci.fsState & NUMLOCK_ON)
	    printf ("NUMLOCK_ON ");

	if (kbci.fsState & NUMLOCK)
	    printf ("NUMLOCK ");

	if (kbci.fsState & CAPSLOCK_ON)
	    printf ("CAPSLOCK_ON ");

	if (kbci.fsState & CAPSLOCK)
	    printf ("CAPSLOCK ");

	if (kbci.fsState & INSERT_ON)
	    printf ("INSERT_ON ");

	if (kbci.fsState & SYSREQ)
	    printf ("SYSREQ ");

	puts (")");

	if (kbci.chChar == 0x03)
	    exit (0);
#elif defined (WIN32)

	ReadConsoleInput (GetStdHandle (STD_INPUT_HANDLE), &Buffer, 1,
			  &NumberOfEventsRead);

	for (i = 0; i < IGNORE_CNT; i++)
	{
	    if (Buffer.Event.KeyEvent.wVirtualScanCode == IgnoreScanCode[i])
	        break;
	}

	if (i != IGNORE_CNT)
	    continue;

	printf ("KeyCode = 0x%.4x Scan = 0x%.4x Down %d Repeat %d\n",
		Buffer.Event.KeyEvent.wVirtualKeyCode,
		Buffer.Event.KeyEvent.wVirtualScanCode,
		Buffer.Event.KeyEvent.bKeyDown,
		Buffer.Event.KeyEvent.wRepeatCount);
	
	printf ("ASCII = 0x%.4x (%c) ", Buffer.Event.KeyEvent.uChar.AsciiChar,
		isprint (Buffer.Event.KeyEvent.uChar.AsciiChar)
		    ?  Buffer.Event.KeyEvent.uChar.AsciiChar
		    : '.');

	printf ("Shift = 0x%.4x ( ", Buffer.Event.KeyEvent.dwControlKeyState);

	if (Buffer.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
	    printf ("SHIFT_PRESSED ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED)
	    printf ("LEFTALT ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED)
	    printf ("RIGHTALT ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)
	    printf ("LEFTCONTROL ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
	    printf ("RIGHTCONTROL ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & SCROLLLOCK_ON)
	    printf ("SCROLLLOCK_ON ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & NUMLOCK_ON)
	    printf ("NUMLOCK_ON ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & CAPSLOCK_ON)
	    printf ("CAPSLOCK_ON ");

	if (Buffer.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY)
	    printf ("ENHANCED_KEY ");

	puts (")");

	if (Buffer.Event.KeyEvent.uChar.AsciiChar == 0x03)
	    exit (0);

#else
	Key.h.ah = KeyInt;
#if defined (__WATCOMC__) && defined(__386__) && !defined(__WINDOWS_386__)
	int386 (0x16, &Key, &Key);
#else
	int86 (0x16, &Key, &Key);
#endif

	Shift.h.ah = 0x02;
#if defined (__WATCOMC__) && defined(__386__) && !defined(__WINDOWS_386__)
	int386 (0x16, &Shift, &Shift);
#else
	int86 (0x16, &Shift, &Shift);
#endif

	printf ("Scan = 0x%.4x ", Key.h.ah);
	printf ("ASCII = 0x%.4x (%c) ", Key.h.al, isprint (Key.h.al) ?
		Key.h.al : '.');
	printf ("Shift = 0x%.4x ( ", Shift.h.al);

	if (Shift.h.al & 0x01)
	    printf ("Right Shift ");

	if (Shift.h.al & 0x02)
	    printf ("Left Shift ");

	if (Shift.h.al & 0x04)
	    printf ("Control ");

	if (Shift.h.al & 0x08)
	    printf ("Alt ");

	puts (")");

	if (Key.h.al == 0x03)
	    exit (0);
#endif
    }
}

/*
 * Check for extended keyboard
 */

#if !defined (OS2) && !defined (__OS2__) && !defined (WIN32)
void	CheckExtendedKeyboard ()
{
    union REGS	r;
    int		i;

    r.h.ah = 0x05;

#if defined (__WATCOMC__) && defined(__386__) && !defined(__WINDOWS_386__)
    r.x.ecx = 0xffff;
    int386 (0x16, &r, &r);
#else
    r.x.cx = 0xffff;
    int86 (0x16, &r, &r);
#endif

    if (r.h.al)
    {
	fprintf (stderr, "showkey: keyboard full!!\n");
	return;
    }

    for (i = 0; i < 16; i++)
    {
	r.h.ah = 0x10;
#if defined (__WATCOMC__) && defined(__386__) && !defined(__WINDOWS_386__)
	int386 (0x16, &r, &r);

	if (r.x.eax & 0x0ffff == 0x0ffff)
#else
	int86 (0x16, &r, &r);

	if (r.x.ax == 0xffff)
#endif
	{
	    KeyInt = 0x10;
	    puts ("showkey: Extended keyboard detected");
	    return;
	}
    }

    puts ("showkey: Normal keyboard detected");
    return;
}
#endif
