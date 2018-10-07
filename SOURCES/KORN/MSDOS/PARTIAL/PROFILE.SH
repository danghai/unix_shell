#!sh
PATH1=".;..;c:/bin;c:/system/dos;c:/system/windows"
PATH2=".;..;c:/bin;w:/;c:/system/dos;c:/system/windows;z:/public;z:/winword;z:/excel"
PATH=$PATH2
CDPATH=".;..;/;c:/;c:/system/windows;c:/usr/ian;c:/usr/src;c:/usr;f:/;c:/system"
INCLUDE=c:/usr/include
LIB=c:/usr/lib
COMSPEC=c:/system/dos/command.com
TMP=z:/tmp
TZ=CET0CET
EDITOR=vi
PS1="[s[1;20H[1;33;41m[%p] %d %t[32;44m[u%n %e> "
HOME=c:/
LINK="/FARCALL/NOLOGO/STACK:0x8000"
TERM=ibmpc-color
TERMCAP=c:/etc/termcap
BETA=V205Beta
CL1="-AL -Zid -W3 -nologo" 
CL2="-AL -Olt -W3 -nologo"
CL=$CL1
EXTENDED_LINE=c:/bin/extend.lst
EXINIT="set aw ai sm dm sw=4 wm=5 dir=$TMP"
LOGNAME="Ian_Stewartson"
MAIL="y:/uupc/mail/is"
MAILCHECK=600
DATE=`date '+%.2y%.2m%.2d'`
MAILBOX=z:/uupc/home/is/$DATE
UUPCSYSRC=y:/uupc/lib/uupc.rc
UUPCUSRRC=y:/uupc/home/is/uupc.rc
export MAILBOX EXINIT TERMCAP TERM LOGNAME MAIL UUPCSYSRC UUPCUSRRC CDPATH
export MAILCHECK MAILBOX LINK TIMEIN TIMEOUT BETA
