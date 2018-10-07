
			    WinXs Version 1.4
		      Copyright	(C) 1996 Mick Meaden
			   All Rights Reserved

       INTRODUCTION

       WinXs for Windows  is  a	 shareware  program  that  provides
       UNIX-like  tool functionality under Windows 3.1,	Windows	for
       Workgroups and Windows 95. To run WinXs,	 you  will  need  a
       386-based  PC  (or  later),  at least 4mb of main memory	and
       approximately 1.8mb or 3.0 mb of disk space  to  install	the
       software	(see below).

       WinXs comes in 16-bit and 32-bit	versions.  The 16-bit  ver-
       sion will run on	Windows	3.1, Windows for Workgroups or Win-
       dows 95;	the 32-bit version will	only  run  on  Windows	95.
       The two versions	are functionally identical, albeit that	for
       integration with	Explorer, or  to  take	advantage  of  long
       filenames and pre-emptive scheduling on Windows 95, you will
       need to run the 32-bit version.	This version also  requires
       3.0  mb	of  disk  space	for installation, as opposed to	the
       1.8mb required for the 16-bit version.

       CONTENTS

       Features	of this	version	of WinXs include:

	 i.  Cat for Windows.  Concatenates one	or more	 files	and
	     appends  them to a	named output file, which is created
	     if	it does	not already exist.

	ii.  Col for Windows.  Filters reverse line-feeds and vari-
	     ous other control characters.  Optionally,	this filter
	     can also be used to remove	backspace sequences  or	 to
	     convert spaces to tabs.

       iii.  Compress and Uncompress.  Windows version of the  UNIX
	     compress(1) and uncompress(1) commands, which are use-
	     ful when downloading files	from the Internet  or  when
	     transferring large	files to and from UNIX systems.

	iv.  Dos2unix and Unix2dos.  Filters for converting between
	     DOS  and  UNIX text file formats; that is,	CR-LF (DOS)
	     and LF (UNIX).

	 v.  Du	for Windows.  A	Windows	implementation of the  UNIX
	     du(1)  command,  which recursively	lists the number of
	     blocks allocated to files and directories.

	vi.  Expand for	Windows.  Performs the opposite	function of
	     col(1) and	expands	tabs to	spaces.

       vii.  File Compare.  A set of programs that perform  various
	     file  comparisons,	 including binary comparisons based
	     on	the  UNIX  cmp(1)  program,  directory	comparisons
	     based  on	dircmp(1),  and	differential text file com-
	     parisons based on diff(1).

       viii. File for Windows.	This program attempts to  determine
	     the  contents of a	file or	files by examination of	the
	     Windows  registration  database,  a  table	 of   magic
	     numbers  uniquely	identifying file types,	and finally
	     the contents of the file itself.

	ix.  Fmt for Windows.  A simple	text processing	utility	for
	     performing	 modest	 tasks such as formatting mail mes-
	     sages.  Text can be formatted using  block,  indented,
	     crown or centered paragraphs, optionally with left	and
	     right text	justification.

	 x.  Fold for Windows.	Folds lines from its  input  files,
	     breaking  the  lines  to  have  a maximum column width
	     (after tab	and  backspace	processing)  or	 a  maximum
	     number  of	 bytes.	 Optionally, lines can be broken at
	     the last blank character within the  specified  column
	     or	byte width.

	xi.  Grep for Windows.	A pattern  searching  program  that
	     scans one or more text files for a	specified character
	     sequence.	Grep uses basic	regular	expression  pattern
	     matching, as defined in X/Open Portability	Guide Issue
	     4.

       xii.  Grex for Windows.	Is a simple stream editor that sup-
	     ports  various  ed(1)-like	 editing  commands.   These
	     include text matching and substitution, text deletion,
	     line deletion, and	multi-line text	insertion.

       xiii. Gzip and Gunzip.  Provides	a Windows interface to	GNU
	     gzip(1),  which  is a separate DOS	program	that can be
	     obtained free of charge under the terms and conditions
	     of	 the GNU software license (see associated help file
	     for further details).

       xiv.  Hd	for Windows.  A	file viewer that will  display	any
	     type  and size of file in hexadecimal format.  Hd sup-
	     ports the same scrolling, search and text marking com-
	     mands  as	More.	It also	supports printing and print
	     previewing.

	xv.  Head and Tail.  These tools display the first,  middle
	     or	last part of one or more text files.  The amount of
	     text displayed (in	numbers	of lines) is specified	via
	     the associated dialog.

       xvi.  More for Windows.	A file viewer that supports more(1)
	     functionality  and	 file movement commands.  This is a
	     full featured Windows MDI	(multi-document	 interface)
	     application,  with	 print	and  print previewing using
	     pr(1), basic regular expression searching,	text  mark-
	     ing,  configurable	 display and printer fonts, escapes
	     to	your favourite editor, rendering  with	or  without
	     line numbers, text	or hexadecimal display,	etc..  More
	     supports File Manager drag	and drop and  will  display
	     files  of any size, depending on virtual store availa-
	     bility.

       xvii. Pax for Windows.  Provides	a Windows interface for	the
	     creation,	reading	 and  writing of cpio, ustar or	tar
	     portable archives.	 The archive formats  supported	 by
	     this  utility  are	 fully compatible with the extended
	     cpio and extended ustar  archive  formats	defined	 by
	     X/Open  and  POSIX.  Pax can be used for local archiv-
	     ing, or it	can be	used  for  information	interchange
	     with UNIX systems.

       xviii. Sed for Windows.	A stream  editor  that	applies	 an
	     editing  script  to  one  or  more	 input	text files,
	     overwriting the input files with the results  or  con-
	     catenating	 the  output to	a single named output file.
	     The command syntax	supported by  this  tool  is  fully
	     compatible	 with  the  stream  editor  command  syntax
	     defined   in   ISO/IEC   9945-2:	1993,	Information
	     Technology	- Portable   Operating	 System	  Interface
	     (POSIX) - Part 2: Shell and Utilities.

       xix.  Split for Windows.	 A  Windows  version  of  the  UNIX
	     split(1) command, which breaks a file up into a speci-
	     fied multiple of  line,  byte,  kilobyte  or  megabyte
	     pieces.   This version also provides an Unsplit facil-
	     ity for combining output files generated by a previous
	     call to Split back	into a single, large file.

	xx.  Strings for Windows.  This	utility	is commonly used on
	     UNIX  systems  to	search	object	files for copyright
	     statements, error messages, etc..	This  version  will
	     scan  files  of any size and content, and will option-
	     ally display file offsets of located strings.

       xxi.  Tr	for  Windows.	Substitutes,  deletes  or  squeezes
	     characters	 from its input.  This is a general purpose
	     filter that can be	used for such things as	 converting
	     the  case	of characters, deleting	control	characters,
	     breaking files up into  word  lists,  reducing  multi-
	     character sequences to single characters, etc..

       xxii. Uuencode and Uudecode.  Windows versions of  the  UNIX
	     commands  of  the	same name.  Useful when	downloading
	     stuff from	the Internet, or when using UNIX mail.

       xxiii. Wc for Windows.  A Windows version of the	UNIX  wc(1)
	     command,  which  counts the number	of lines, words	and
	     bytes in one or more text files.

       xxiv. Winxsdll.dll.  This DLL adds an extra menu	to the Win-
	     dows  File	 Manager menu bar, allowing the	above WinXs
	     programs to be initiated directly	from  File  Manager
	     (see below	for details).

       xxv.  WinXs Shell (32-bit version only).	 This is  activated
	     from the Folders context menu within Explorer and pro-
	     vides A UNIX-like directory display, plus easy  access
	     to	 any  of  the WinXs tools.  The	WinXs shell is also
	     available from the	context	menu  associated  with	the
	     Start button on Taskbar.

       The WinXs utilities  can	 be  accessed  directly	 from  File
       Manager,	 or  from  Explorer and	the Taskbar (32-bit version
       only).  To use More as the default text file viewer or  text
       file print program on your system, run WinXs setup and check
       the appropriate configuration options (see  setup  help	for
       further details).

       INSTALLING WINXS

       Create a	directory (e.g., c:\winxs), copy the files in  this
       distribution  thereto  (unzipping  as  necessary),  and	run
       SETUP.EXE.  This	will add Win&Xs	to the	File  Manager  menu
       bar, add	the WinXs shell	to the Folders context menu (32-bit
       version only), associate	a text editor  for  use	 from  File
       Manager,	and allow configuration	of other options present in
       this version of the product.

       SETUP.EXE can be	re-run at any  time  to	 change	 the  above
       options.

       NOTE.  If File Manager is running when setup is executed, it
       must  be	stopped	and restarted before changes affecting File
       Manager (e.g., adding  WinXs  to	 the  menu  bar)  can  take
       effect.

       If you prefer, you  can	try  out  the  WinXs  tools  before
       integrating  the	 product with File Manager or Explorer,	and
       before making any changes to other system files.	  Create  a
       WinXs directory,	as indicated above, and	unzip the distribu-
       tion file.  DO NOT RUN THE SETUP	PROGRAM. Select	 the  WinXs
       directory  in  File  Manager or Explorer, and simply double-
       click any of the	.EXE files to try out the tools.  The first
       screen  displayed  by  each tool	will contain a Help button;
       click this to display the associated manual page.

       The following table indicates which  tools  are	located	 in
       which .EXE files:

	  ______________________________________________________
	 |  .EXE File	|   Tool			       |
	 |		|				       |
	 |______________|______________________________________|
	 |		|				       |
	 |  DIFF.EXE	|   cmp(1), dircmp(1), diff(1).	       |
	 |		|				       |
	 |  DOSCONV.EXE	|   dos2unix(1), unix2dos(1).	       |
	 |		|   uuencode(1), uudecode(1),	       |
	 |		|   compress(1), uncompress(1),	       |
	 |		|   gzip(1), gunzip(1).		       |
	 |		|				       |
	 |  FILTERS.EXE	|   cat(1), col(1), expand(1),	       |
	 |		|   fold(1), fmt(1), grex(1),	       |
	 |		|   tr(1).			       |
	 |		|				       |
	 |  GENUTS.EXE	|   du(1), file(1), head(1),	       |
	 |		|   split(1), strings(1), tail(1),     |
	 |		|   wc(1).			       |
	 |		|				       |
	 |  GREP.EXE	|   grep(1).			       |
	 |		|				       |
	 |  HD.EXE	|   hd(1).			       |
	 |		|				       |
	 |  MORE.EXE	|   more(1), pr(1).		       |
	 |		|				       |
	 |  SED.EXE	|   sed(1).			       |
	 |		|				       |
	 |  WINPAX.EXE	|   pax(1).			       |
	 |		|				       |
	 |  WINXSSH.EXE	|   WinXs Shell	(32-bit	version	only). |
	 |______________|______________________________________|

       The SETUP.EXE program can be run	at any	stage  to  complete
       the installation	process, or not	at all.

       UPGRADING A PREVIOUS VERSION OF WINXS

       Because this release updates the	WinXs DLL, it is  important
       to ensure that File Manager IS NOT running when you upgrade.
       Copy the	ZIP file to your WinXs directory, unzip	using pkun-
       zip,  WinZip  (or equivalent) and restart File Manager.	Re-
       run the WinXs setup program if you want	to  add	 the  WinXs
       shell  to  Explorer  context menus, or if you want to change
       any of the configuration	options.

       UNINSTALLATION

       The uninstall process has been made as clean and	 simple	 as
       possible.   Re-run the setup program and	click the Uninstall
       button; this will undo all changes made to system files	and
       remove any WinXs	.ini files from	your Windows directory.	 To
       complete	the uninstall process, manually	 delete	 the  WinXs
       installation directory and all its contents.

       WINDOWS 95

       This version of WinXs can be  installed	on  Windows  95	 as
       indicated above.	 Optionally, the setup program will add	the
       WinXs tools to the Windows 95 File Manager menu bar, and	the
       WinXs Shell to Folder context menus in Explorer (32-bit ver-
       sion only).  It is also recommended that	 the  default  text
       file  viewer  and  print	 program  be changed in	the Windows
       Registry	to use the more.exe tool supplied with this package
       (see setup help for details).

       REGISTRATION

       This distribution contains an unregistered shareware version
       of  the product (see the	file license.txt for details of	how
       to register).  This is a	fully functional  version  intended
       for  evaluation.	  Note,	however, that options affecting	the
       operation of more, hd and  pr  can  only	 be  saved  in	the
       registered  version.  Also, multiple input filenames are	not
       supported in the	unregistered version.

       REPORTING PROBLEMS

       Please email comments and/or bug	reports	to:

	    100345.2641@compuserve.com
