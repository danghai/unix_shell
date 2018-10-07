/*
 * Print OS/2 2.x process information
 *
 * usage: ps [ -L name ] [ -D name ] [ -SMm ] [[ -Ftnl ] pid]
 *
 *  -L Name	Load process info from file name
 *  -D Name	Dump process info to file name
 *  -S		Display Semaphore info
 *  -M		Display Shared memory info
 *  -m		Display Module info
 *  -F		Display Full Names
 *  -l		Display long info
 *  -t		Display Thread Info
 *  -a		Display All processes
 *  -n		Do not display process trees
 *  -o		Order process info
 *
 *  pid		The process id or name
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/ps.c,v 1.6 1994/08/28 15:18:03 istewart Exp $
 *
 *    $Log: ps.c,v $
 * Revision 1.6  1994/08/28  15:18:03  istewart
 * Fix display (end of list detection)
 *
 * Revision 1.5  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 1.4  1993/08/25  16:04:22  istewart
 * Formatting change
 *
 * Revision 1.3  1993/07/02  10:21:35  istewart
 * 224 Beta fixes
 *
 * Revision 1.2  1993/06/14  11:02:07  istewart
 * More changes for 223 beta
 *
 * Revision 1.1  1993/06/02  09:52:35  istewart
 * Beta 223 Updates - see Notes file
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define INCL_DOSSESMGR
#define INCL_DOSPROCESS
#define INCL_DOSMODULEMGR
#include <os2.h>

#ifdef __OS2__
#  pragma linkage		(DosQProcStatus, far16 pascal)
#  pragma linkage		(DosGetPrty, far16 pascal)

extern USHORT			DosQProcStatus (PVOID, USHORT);
extern USHORT			DosGetPrty (USHORT, PUSHORT, USHORT);

#  defined Dos32GetPrty		DosGetPrty
#  defined Dos32QProcStatus	DosQProcStatus
#  define PTR(ptr)		(ptr)

#elif defined (__EMX__)

USHORT				_THUNK_FUNCTION (Dos16GetPrty) ();
USHORT				_THUNK_FUNCTION (Dos16QProcStatus) ();

#  define PTR(ptr)		(ptr)

extern USHORT			Dos32QProcStatus (PVOID, USHORT);
extern USHORT			Dos32GetPrty (USHORT, PUSHORT, USHORT);

#else

#  define Dos32QProcStatus	DosQProcStatus
#  define Dos32GetPrty		DosGetPrty

extern USHORT APIENTRY		DosQProcStatus (PVOID, USHORT);

#define PTR(ptr)	((void *)((((ULONG)BasePS) & 0xFFFF0000L) |	\
				  (((ULONG)(ptr))  & 0x0000FFFFL) ))
#endif

/* Process Status structures */

#define PROCESS_END_INDICATOR	3

#pragma pack(1)

typedef struct _SUMMARY
{
    ULONG	ulThreadCount;		/* Number of threads in system	*/
    ULONG	ulProcessCount;		/* Number of processes in system*/
    ULONG	ulModuleCount;		/* Number of modules in system	*/
} SUMMARY, *PSUMMARY;

/*
 * Thread information
 */

typedef struct _THREADINFO
{
    ULONG	ulRecType;		/* Record type (thread = 100)	*/
    USHORT	tidWithinProcess;	/* TID within process (TID is	*/
    					/* 4 bytes!!)			*/
    USHORT	usSlot;			/* Unique thread slot number	*/
    ULONG	ulBlockId;		/* Sleep id thread is sleeping on*/
    ULONG	ulPriority;		/* Priority			*/
    ULONG	ulSysTime;		/* Thread System Time		*/
    ULONG	ulUserTime;		/* Thread User Time		*/
    UCHAR	uchState;		/* 1=ready,2=blocked,5=running	*/
    UCHAR	uchPad;			/* Filler			*/
    USHORT	usPad;			/* Filler			*/
} THREADINFO, *PTHREADINFO;

/*
 * Process information
 */

typedef struct _PROCESSINFO
{
    ULONG	ulEndIndicator;		/* 1 means not end, 3 means	*/
    					/* last entry			*/
    PTHREADINFO	ptiFirst;		/* Address of the 1st Thread	*/
    					/* Control Blk			*/
    USHORT	pid;			/* Process ID (2 bytes - PID	*/
    					/* is 4 bytes)			*/
    USHORT	pidParent;		/* Parent's process ID		*/
    ULONG	ulType;			/* Process Type			*/
    ULONG	ulStatus;		/* Process Status		*/
    ULONG	idSession;		/* Session ID			*/
    USHORT	hModRef;		/* Module handle of EXE		*/
    USHORT	usThreadCount;		/* Number of threads in this	*/
    					/* process			*/
    ULONG	ulSessionType;		/* Session Type			*/
    PVOID	pvReserved;		/* Unknown			*/
    USHORT	usSem16Count;		/* Number of 16-bit system	*/
    					/* semaphores			*/
    USHORT	usDllCount;		/* Number of Dlls used by	*/
    					/* process			*/
    USHORT	usShrMemHandles;	/* Number of shared memory	*/
    					/* handles			*/
    USHORT	usReserved;		/* Unknown			*/
    PUSHORT	pusSem16TableAddr;	/* Address of a 16-bit semaphore*/
    					/* table			*/
    PUSHORT	pusDllTableAddr;	/* Address of a Dll table	*/
    PUSHORT	pusShrMemTableAddr;	/* Address of a shared memory	*/
    					/* table			*/
} PROCESSINFO, *PPROCESSINFO;

/*
 * Semaphore info
 */

typedef struct _SEMINFO
{
    struct _SEMINFO *pNext;		/* Ptr to next block		*/
    USHORT	idOwningThread;		/* ID of owning thread?		*/
    UCHAR	fbFlags;		/* Semaphore flags		*/
    UCHAR	uchReferenceCount;	/* Number of references		*/
    UCHAR	uchRequestCount;	/* Number of requests		*/
    UCHAR	ucReserved[3];		/* Unknown			*/
    USHORT	usIndex;		/* Index			*/
    CHAR	szSemName[ 1 ];		/* ASCIIZ semaphore name	*/
} SEMINFO, *PSEMINFO;

/*
 * Shared memory info
 */

typedef struct _SHRMEMINFO
{
    struct _SHRMEMINFO	*pNext;		/* Ptr to next block		*/
    USHORT	usMemHandle;		/* Shared memory handle (?)	*/
    SEL		selMem;			/* Selector			*/
    USHORT	usReferenceCount;	/* Number of references		*/
    CHAR	szMemName[ 1 ];		/* ASCIIZ shared memory name	*/
} SHRMEMINFO, *PSHRMEMINFO;

/*
 * Module info
 */

typedef struct _MODINFO
{
    struct _MODINFO	*pNext;	/* Ptr to next block (NULL on last one)	*/
    USHORT	hMod;		/* Module handle			*/
    USHORT	usModType;	/* Module type (0=16bit,1=32bit)	*/
    ULONG	ulModRefCount;	/* Count of module references		*/
    ULONG	ulSegmentCount;	/* Number of segments in module		*/
    ULONG	ulDontKnow1;	/*					*/
    PSZ		szModName;	/* Addr of fully qualified module name	*/
    USHORT	usModRef[ 1 ];	/* Handles of module references		*/
} MODINFO, *PMODINFO;

/*
 * header
 */

typedef struct _BUFFHEADER
{
    PSUMMARY	psumm;		/* SUMMARY section ptr			*/
    PPROCESSINFO ppi;		/* PROCESS section ptr			*/
    PSEMINFO	psi;		/* SEM section ptr (add 16 to offset)	*/
    PVOID	pDontKnow1;	/*					*/
    PSHRMEMINFO	psmi;		/* SHARED MEMORY section ptr		*/
    PMODINFO	pmi;		/* MODULE section ptr			*/
    PVOID	pDontKnow2;	/*					*/
    PVOID	pDontKnow3;	/*					*/
} BUFFHEADER, *PBUFFHEADER;

#pragma pack()

/*
 * Associated functions
 */

void		PrintProcessEntry (PPROCESSINFO, USHORT, PBUFFHEADER);
void		RemoveProcessEntry (USHORT);
int		SortProcesses (const void *, const void *);
void		DisplayAllProcess (PBUFFHEADER);
PPROCESSINFO	FindPidEntry (PBUFFHEADER, USHORT);
void		DisplaySession (PBUFFHEADER);
void	 	DisplayProcessTree (USHORT, USHORT, PBUFFHEADER);
void	 	DisplayProcess (PBUFFHEADER);
void		PrintSharedMemoryInfo (PSHRMEMINFO);
void		PrintSemaphoreInfo (PSEMINFO);
void		PrintTaskHeader (void);
void		PrintThreadInfo (PTHREADINFO, USHORT);
void		PrintUsage (void);
void		PrintModuleInfo (PMODINFO);
char		*ConvertPathToFormat (char *);
char		*GetModuleName (USHORT);
void		main (int, char **);
#ifdef __OS2__
int		getopt (int, char **, char *);
#endif

/*
 * Globals
 */

#ifdef __OS2__
int		optind = 1;
char		*optarg = (char *)NULL;
#endif

bool		RequireHeader = TRUE;
size_t		count = 0;		/* Number of processes		*/
PPROCESSINFO	*ListOfProcesses;	/* Process list			*/
bool		F_flag = FALSE;		/* Full file names		*/
bool		t_flag = FALSE;		/* Print Thread info		*/
bool		o_flag = FALSE;		/* Order info 			*/
bool		n_flag = FALSE;		/* Do not Print Process tree	*/
bool		l_flag = FALSE;		/* Full flag			*/
char		*ProcessName = (char *)NULL;

#ifndef __OS2__
PBUFFHEADER	 BasePS;
#endif

/*
 * OS/2 Process Types
 */

char		*ProcessTypes[] = {
    " DEF", "FULL", " WIN", "  PM", " VDM",
    " GRP", " DLL", "WVDM", " PDD", " VDD"
};

char		*ThreadState[] = {
    "", "Ready", "Blocked", "", "", "Running"
};

/*
 * Display Process Tree
 */

void	DisplayProcessTree (USHORT pid, USHORT indent, PBUFFHEADER ps)
{
    PPROCESSINFO	ppiLocal = PTR (ps->ppi);

    while (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR)
    {
	if ((indent && (ppiLocal->pidParent == pid)) ||
	    ((!indent) && (ppiLocal->pid == pid)))
	    PrintProcessEntry (ppiLocal, indent, ps);

/* Go to the next Process Block */

        ppiLocal = (PPROCESSINFO) PTR (ppiLocal->ptiFirst +
				       ppiLocal->usThreadCount);
    }
}

/*
 * Print the Process Tree
 */

void	main (int argc, char **argv)
{
    USHORT		rc;
    PBUFFHEADER		ps;
    int			c;
    bool		S_flag = FALSE;
    bool		M_flag = FALSE;
    bool		m_flag = FALSE;
    bool		a_flag = FALSE;
    char		*D_FileName = (char *)NULL;
    char		*L_FileName = (char *)NULL;

    while ((c = getopt (argc, argv, "oalmMSnFtD:L:")) != EOF)
    {
        switch (c)
	{
	    case 'F':	F_flag = TRUE;		break;
	    case 't':	t_flag = TRUE;		break;
	    case 'o':	o_flag = TRUE;		break;
	    case 'S':	S_flag = TRUE;		break;
    	    case 'M':	M_flag = TRUE;		break;
	    case 'n':	n_flag = TRUE;		break;
	    case 'm':	m_flag = TRUE;		break;
	    case 'l':	l_flag = TRUE;		break;
	    case 'a':	a_flag = TRUE;		break;
	    case 'D':	D_FileName = optarg;	break;
	    case 'L':	L_FileName = optarg;	break;

	    default:	PrintUsage ();
	}
    }

/* Get process info */

    if ((ps = (PBUFFHEADER)malloc (0x8000)) == (PBUFFHEADER)NULL)
    {
        fputs ("ps: Out of memory\n", stderr);
	exit (1);
    }

#ifndef __OS2__
    BasePS = ps;
#endif

/* Load information */

    if (L_FileName != (char *)NULL)
    {
	int	Fid = open (L_FileName, O_RDONLY | O_BINARY);

	if (Fid < 0)
	{
	    fprintf (stderr, "ps: Cannot open %s (%d)\n", L_FileName, errno);
	    exit (1);
	}

	if ((unsigned)read (Fid, ps, 0x8000) != 0x8000)
	{
	    fprintf (stderr, "ps: Read error on %s (%d)\n", L_FileName, errno);
	    exit (1);
	}

	close (Fid);
    }

/* Get info from system */

    else if (rc = Dos32QProcStatus (ps, 0x8000))
    {
	fprintf (stderr, "ps: DosQProcStatus failed (%d)\n", rc);
	exit (1);
    }

/* Dump the info */

    if (D_FileName != (char *)NULL)
    {
        int	Fid = open (D_FileName,
			    O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
			    S_IWRITE | S_IREAD);

	if (Fid < 0)
	{
	    fprintf (stderr, "ps: Cannot create %s (%d)\n", D_FileName, errno);
	    exit (1);
	}

	if ((unsigned)write (Fid, ps, 0x8000) != 0x8000)
	    fprintf (stderr, "ps: Write error on %s (%d)\n", D_FileName, errno);

	close (Fid);
    }

/* Advance */

    argc -= optind;
    argv += optind;

/* Print Semaphore info ? */

    if (S_flag)
	PrintSemaphoreInfo (PTR (ps->psi));

/* Print Shared memory info ? */

    if (M_flag)
	PrintSharedMemoryInfo (PTR (ps->psmi));

/* Print Module info ? */

    if (m_flag)
	PrintModuleInfo (PTR (ps->pmi));

/* Print all process info ? */

    if (a_flag)
	DisplayAllProcess (ps);

/* anything else */

    if (((a_flag | S_flag | M_flag | m_flag) || (D_FileName != (char *)NULL)) &&
        !argc)
	exit (0);

/* Session Info */

    if (!argc)
    {
	DisplaySession (ps);
        exit (0);
    }

/* A specific process ? */

    if (argc != 1)
        PrintUsage ();

    PrintTaskHeader ();

    if (isdigit (**argv))
	DisplayProcessTree (atoi (argv[0]), 0, ps);

    else
    {
	ProcessName = *argv;
	DisplayProcess (ps);
    }

    exit (0);
}

/*
 * Print usage
 */

void	PrintUsage (void)
{
    fputs ("usage: ps [ -L name ] [ -D name ] [ -SMm ] [[ -oalFtn ] pid]\n",
    	   stderr);
    exit (1);
}

/*
 * Covert to UNIX format
 */

char *ConvertPathToFormat (char *path)
{
    char	*s = path;

    while ((path = strchr (path, '\\')) != (char *)NULL)
	*path = '/';

    return s;
}

/*
 * Get Module name
 */

char	*GetModuleName (USHORT hModRef)
{
    static char		name[PATH_MAX + NAME_MAX + 3];
    char		*pName;

#ifdef __OS2__
    if (DosQueryModuleName (hModRef, sizeof (name), name))
#else
    if (DosGetModName (hModRef, sizeof (name), name))
#endif
	strcpy(name, "<unknown>");
	
    ConvertPathToFormat (name);

    return (!F_flag && ((pName = strrchr (name, '/')) != (char *)NULL))
	   ? ++pName : name;
}

/*
 * Getopt
 */

#ifdef __OS2__
int	getopt (int argc, char **argv, char *optstring)
{
    int		cur_option;		/* Current option		*/
    char	*cp;			/* Character pointer		*/
    static int	GetOptionPosition = 1;

    if (GetOptionPosition == 1)
    {

/* Check for out of range, correct start character and not single */

	if ((optind >= argc) || (*argv[optind] != '-') || !argv[optind][1])
	    return EOF;

	if (!strcmp (argv[optind], "--"))
	    return EOF;
    }

/* Get the current character from the current argument vector */

    cur_option = argv[optind][GetOptionPosition];

/* Validate it */

    if ((cur_option == ':') ||
	((cp = strchr (optstring, cur_option)) == (char *)NULL))
    {

/* Move to the next offset */

	if (!argv[optind][++GetOptionPosition])
	{
	    optind++;
	    GetOptionPosition = 1;
	}

	return '?';
    }

/* Parameters following ? */

    optarg = (char *)NULL;

    if (*(++cp) == ':')
    {
	if (argv[optind][GetOptionPosition + 1])
	    optarg = &argv[optind++][GetOptionPosition + 1];

	else if (++optind >= argc)
	{
	    optarg = (char *)NULL;
	    GetOptionPosition = 1;
	    return '?';
	}

	else
	    optarg = argv[optind++];

	GetOptionPosition = 1;
    }

    else if (!argv[optind][++GetOptionPosition])
    {
	GetOptionPosition = 1;
	optind++;
    }

    return cur_option;
}
#endif

/*
 * PrintThread Info
 */

void	PrintThreadInfo (PTHREADINFO ptiFirst, USHORT usThreadCount)
{
    USHORT	i;

    puts ("\t\t TID Slot Sleep         Pri  STim  UTim State");

    for (i = 0; i < usThreadCount; i++)
    {
	printf ("\t\t%4d %4d 0x%.8lx 0x%.4lx %5ld %5ld %s (0x%x)\n",
		ptiFirst->tidWithinProcess,
		ptiFirst->usSlot,
		ptiFirst->ulBlockId,
		ptiFirst->ulPriority,
		ptiFirst->ulSysTime,
		ptiFirst->ulUserTime,
		ptiFirst->uchState <= 5 ? ThreadState[ptiFirst->uchState]
					: "unknown",
		ptiFirst->uchState);

        ptiFirst++;
    }

    RequireHeader = TRUE;
}

/*
 * Print Semaphore Information
 */

void	PrintSemaphoreInfo (PSEMINFO psi)
{
    psi = (PSEMINFO)(((char *)psi) + 16);
    puts ("   TID Flags RefC ReqC   Index Name");

    while (psi != (PSEMINFO)NULL)
    {
	psi = PTR (psi);

	if (!psi->pNext)
	    break;

	printf ("0x%.4x  0x%.2x  %3d  %3d  0x%.4x \\S%s (0x%.2x%.2x%.2x)\n",
		psi->idOwningThread,
		psi->fbFlags,
		psi->uchReferenceCount,
		psi->uchRequestCount,
		psi->usIndex,
		psi->szSemName,
		psi->ucReserved[0],
		psi->ucReserved[1],
		psi->ucReserved[2]);

	psi = psi->pNext;
    }
}

/*
 * Print Shared Memory Information
 */

void	PrintSharedMemoryInfo (PSHRMEMINFO psmi)
{
    puts ("   Hdl    Sel RefC Name");

    while (psmi != (PSHRMEMINFO)NULL)
    {
	psmi = PTR (psmi);

	if (!psmi->pNext)
	    break;

	printf ("0x%.4x 0x%.4x  %3d %s\n",
		psmi->usMemHandle,
		psmi->selMem,
		psmi->usReferenceCount,
		psmi->szMemName);

	psmi = psmi->pNext;
    }
}

/*
 * Print Module Information
 */

void	PrintModuleInfo (PMODINFO pmi)
{
    puts ("Type   RefC SegC     hMod Name");

    while (pmi != (PMODINFO)NULL)
    {
	pmi = PTR (pmi);

	if (!pmi->pNext)
	    break;

	printf ("  %2d %6ld  %3ld [0x%.4x] %s\n",
		pmi->usModType ? 32 : 16,
		pmi->ulModRefCount,
		pmi->ulSegmentCount,
    		pmi->hMod,
    		PTR (pmi->szModName));

	pmi = pmi->pNext;
    }
}

/*
 * Display a process by name
 */

void	DisplayProcess (PBUFFHEADER ps)
{
    PPROCESSINFO	ppiLocal = PTR (ps->ppi);
    char		*pName;

    strlwr (ProcessName);

    while (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR)
    {
	pName = strlwr (GetModuleName (ppiLocal->hModRef));

	if (strstr (pName, ProcessName) != (char *)NULL)
	    PrintProcessEntry (ppiLocal, 0, ps);

        ppiLocal = (PPROCESSINFO) PTR (ppiLocal->ptiFirst +
				       ppiLocal->usThreadCount);
    }
}

/*
 * Print An entry in the process table
 */

void	 PrintProcessEntry (PPROCESSINFO ppiLocal, USHORT indent,
			    PBUFFHEADER ps)
{
    USHORT		prty;

    if (Dos32GetPrty (PRTYS_PROCESS, &prty, ppiLocal->pid))
	prty = 0;

    PrintTaskHeader ();

    if (l_flag)
	printf ("%5d %5d %3d %s  0x%.3lx %5ld 0x%.3lx %5d %5d %5d 0x%.3x 0x%.4x %*s%s\n",
		ppiLocal->pid,
		ppiLocal->pidParent,
		ppiLocal->usThreadCount,
		ppiLocal->ulType <= 9 ? ProcessTypes[ppiLocal->ulType] : " ?? ",
		ppiLocal->ulStatus,
		ppiLocal->idSession,
		ppiLocal->ulSessionType,
		ppiLocal->usSem16Count,
		ppiLocal->usDllCount,
		ppiLocal->usShrMemHandles,
		ppiLocal->usReserved,
		prty, indent, "", GetModuleName (ppiLocal->hModRef));

    else
	printf ("%5d %5d %3d %s  0x%.3lx %5ld 0x%.4x %*s%s\n",
		ppiLocal->pid,
		ppiLocal->pidParent,
		ppiLocal->usThreadCount,
		ppiLocal->ulType <= 9 ? ProcessTypes[ppiLocal->ulType] : " ?? ",
		ppiLocal->ulStatus,
		ppiLocal->idSession,
		prty, indent, "", GetModuleName (ppiLocal->hModRef));

    if (t_flag)
	PrintThreadInfo (PTR (ppiLocal->ptiFirst), ppiLocal->usThreadCount);

/* Remove repeats on display all */

    if (count)
	RemoveProcessEntry (ppiLocal->pid);

    if (!n_flag)
	DisplayProcessTree (ppiLocal->pid, indent + 2, ps);
}

/*
 * Output Task header
 */

void	PrintTaskHeader (void)
{
    if (!RequireHeader)
        return;

    if (l_flag)
	puts ("\n  PID  PPID  TC TYPE STATUS   SID STYPE   Sem  DLLs  Smem          PRI Program");

    else
	puts ("\n  PID  PPID  TC TYPE STATUS   SID    PRI Program");

    RequireHeader = FALSE;
}

/*
 * Display All process by name
 */

void	DisplayAllProcess (PBUFFHEADER ps)
{
    PPROCESSINFO	ppiLocal = PTR (ps->ppi);
    PSUMMARY		pSumm = PTR (ps->psumm);
    size_t		i;

/* Count the proceses */

    count = 0;

    while (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR)
    {
	count++;
        ppiLocal = (PPROCESSINFO) PTR (ppiLocal->ptiFirst +
				       ppiLocal->usThreadCount);
    }

/* Build array */

    if ((ListOfProcesses = (PPROCESSINFO *)malloc (sizeof (PPROCESSINFO) *
						   count))
			    == (PPROCESSINFO *)NULL)
    {
        fputs ("ps: Out of memory\n", stderr);
	exit (1);
    }

/* Build the list */

    ppiLocal = PTR (ps->ppi);
    count = 0;

    while (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR)
    {
	ListOfProcesses[count++] = ppiLocal;
        ppiLocal = (PPROCESSINFO) PTR (ppiLocal->ptiFirst +
				       ppiLocal->usThreadCount);
    }

    if (o_flag)
	qsort ((void *)ListOfProcesses, count, sizeof (PPROCESSINFO),
	       SortProcesses);

/* Display list - no repeats */

    for (i = 0; i < count; i++)
    {
	if (ListOfProcesses[i] != (PPROCESSINFO)NULL)
	    PrintProcessEntry (ListOfProcesses[i], 0, ps);
	
	ListOfProcesses[i] = (PPROCESSINFO)NULL;
    }

    printf ("System reports: %ld Threads, %ld Processes, %ld Modules\n",
    	    pSumm->ulThreadCount, pSumm->ulProcessCount, pSumm->ulModuleCount);
}

/*
 * Sort function
 */

int	SortProcesses (const void *p1, const void *p2)
{
    if ((*(PPROCESSINFO *)p1)->pid > (*(PPROCESSINFO *)p2)->pid)
        return 1;

    else if ((*(PPROCESSINFO *)p1)->pid < (*(PPROCESSINFO *)p2)->pid)
        return -1;

    return 0;
}

/*
 * Remove entry function
 */

void	RemoveProcessEntry (USHORT pid)
{
    size_t	i;

    for (i = 0; i < count; i++)
    {
	if ((ListOfProcesses[i] != (PPROCESSINFO)NULL) &&
	    (ListOfProcesses[i]->pid == pid))
	    ListOfProcesses[i] = (PPROCESSINFO)NULL;
    }
}

/*
 * Display the processes in the current session
 */

void	DisplaySession (PBUFFHEADER ps)
{
    PPROCESSINFO	ppiMy;
    ULONG		MySession;
    USHORT		MyPid = getpid();
    USHORT		ParentPid = MyPid;
    USHORT		SessionParentPid = MyPid;

/* Find the parent for the current session */

    while (TRUE)
    {
	if ((ppiMy = FindPidEntry (ps, ParentPid)) == (PPROCESSINFO)NULL)
	{
	    if (ParentPid != MyPid)
	        break;

	    fprintf (stderr,
		     "ps: Cannot find process information for self (%d)\n",
		     ParentPid);
	    exit (1);
	}

/* First time round ? - yes save session number */

	if (ParentPid == MyPid)
	    MySession = ppiMy->idSession;

	else if (MySession != ppiMy->idSession)
	    break;

/* Still in same session, now find the parent */

	SessionParentPid = ParentPid;
	ParentPid = ppiMy->pidParent;
    }

/* Now display the session info */

    DisplayProcessTree (SessionParentPid, 0, ps);
}


/*
 * Find the entry for a PID
 */

PPROCESSINFO	FindPidEntry (PBUFFHEADER ps, USHORT pid)
{
    PPROCESSINFO	ppiLocal = PTR (ps->ppi);

    while (ppiLocal->ulEndIndicator != PROCESS_END_INDICATOR)
    {
	if (ppiLocal->pid == pid)
	    return ppiLocal;

        ppiLocal = (PPROCESSINFO) PTR (ppiLocal->ptiFirst +
				       ppiLocal->usThreadCount);
    }

    return (PPROCESSINFO)NULL;
}

/*
 * Specials for EMX
 */

#if defined (__EMX__)
USHORT	Dos32GetPrty (USHORT usScope, PUSHORT pusPriority, USHORT pid)
{
    return ((USHORT)
	    (_THUNK_PROLOG (2 + 4 + 2);
	     _THUNK_SHORT (usScope);
	     _THUNK_FLAT (pusPriority);
	     _THUNK_SHORT (pid);
	     _THUNK_CALL (Dos16GetPrty)));
}

USHORT	Dos32QProcStatus (PVOID pvProcData, USHORT usSize)
{
    return ((USHORT)
	    (_THUNK_PROLOG (4 + 2);
	     _THUNK_FLAT (pvProcData);
	     _THUNK_SHORT (usSize);
	     _THUNK_CALL (Dos16QProcStatus)));
}
#endif
