/*
        This is machine generated code, from the unipen.awk
	parser. The code was generated with the awkcc package
	provided by Chris Ramming (jcr@research.att.com),
	Copyright (c) 1991 AT&T, all Rights Reserved.

*/

#include <stdio.h>
#include <math.h>
#include "ear.h"
#include "hash.h"
#include "awk.h"
#include "y.tab.h"
#include "header.h"
VARP	tmpvar;
void	END();
int	i;

void	AF_make_tabs();
VARP	AF_get_key();
void	AF_get_def_type();
VARP	AF_check_key();
void	AF_print_key();
VARP	AF_get_arg();
void	AF_get_tab_type();
VARP	AF_check_arg_type();
void	AF_print_arg();
void	AF_aprint();
void	AF_warning();
VARP	AF_min();
char	tmpstr0[CONVSIZ];
VARP	tmpvar0;
VARP	tmpvar1;
VARP	tmpvar2;
VARP	tmpvar3;
VARP	tmpvar4;
VARP	tmpvar5;
VARP	tmpvar6;
VARP	tmpvar7;
VARP	tmpvar8;
VARP	funtmp0=(VARP) NULL;
VARP	Au_i;
VARP	Au_key;
HTBP	Aa_KEYTAB;
HTBP	Aa_RESTAB;
VARP	Au_l;
VARP	Au_print_lisp_code;
VARP	Au_OK;
VARP	Au_KEY;
VARP	Au_TYPESTR;
VARP	Au_TYPE;
extern fa	*reg1_19;
fa	*reg1_19;
extern fa	*reg1_20;
fa	*reg1_20;
extern fa	*reg1_21;
fa	*reg1_21;
extern fa	*reg1_22;
fa	*reg1_22;
HTBP	Aa_ARGARR;
VARP	Au_ARGNUM;
VARP	Au_IARG;
double	Af_WARNING;
VARP	Au_DEFKEY;
VARP	Au_RES;
extern fa	*reg35;
fa	*reg35;
VARP	Au_ARG;
VARP	Au_arg;
extern fa	*reg42;
fa	*reg42;
extern fa	*reg43;
fa	*reg43;
extern fa	*reg44;
fa	*reg44;
VARP	Au_type;
extern fa	*reg46;
fa	*reg46;
VARP	Au_typeck;
extern fa	*reg48;
fa	*reg48;
extern fa	*reg49;
fa	*reg49;
extern fa	*reg50;
fa	*reg50;
VARP	Au_argnum;
extern fa	*reg1_55;
fa	*reg1_55;
extern fa	*reg1_56;
fa	*reg1_56;
VARP	Au_elem;
double	Af_FNR;
VAR	partmp0;
VAR	partmp1;
VAR	partmp2;
VAR	partmp3;

void
init_vars()
{
VARP	m_var();
HTBP	m_array();

VARP	tmpv;
Au_i=m_var();
Au_key=m_var();
Aa_KEYTAB=m_array();
Aa_RESTAB=m_array();
Au_l=m_var();
Au_print_lisp_code=m_var();
Au_OK=m_var();
Au_KEY=m_var();
Au_TYPESTR=m_var();
Au_TYPE=m_var();
reg1_19=mdfa(reparse("^[ \\t]*"), 1);
reg1_20=mdfa(reparse("\\t"), 1);
reg1_21=mdfa(reparse("[\\t]"), 1);
reg1_22=mdfa(reparse("[\\ ]*"), 1);
Aa_ARGARR=m_array();
Au_ARGNUM=m_var();
Au_IARG=m_var();
Af_WARNING=0.0;
Au_DEFKEY=m_var();
Au_RES=m_var();
reg35=mdfa(reparse("^[\\[][NSFRL.+][\\]]$"), 0);
Au_ARG=m_var();
Au_arg=m_var();
reg42=mdfa(reparse("\\\\\"$"), 0);
reg43=mdfa(reparse("\"$"), 0);
reg44=mdfa(reparse("^.\\.$"), 0);
Au_type=m_var();
reg46=mdfa(reparse("^[+-]?[0-9]*\\.?[0-9]+$"), 0);
Au_typeck=m_var();
reg48=mdfa(reparse("^#[0-9]+$"), 0);
reg49=mdfa(reparse("^((([0-9]+(:[0-9]+)?-[0-9]+(:[0-9]+)?)|([0-9]+)),?)*$"), 0);
reg50=mdfa(reparse("^\".*\"$"), 0);
Au_argnum=m_var();
reg1_55=mdfa(reparse("\\\\"), 1);
reg1_56=mdfa(reparse("\""), 1);
Au_elem=m_var();
partmp0.string=m_str(1, "");
partmp1.string=m_str(1, "");
partmp2.string=m_str(1, "");
partmp3.string=m_str(1, "");
tmpvar0=m_var();
tmpvar1=m_var();
tmpvar2=m_var();
tmpvar3=m_var();
tmpvar4=m_var();
tmpvar5=m_var();
tmpvar6=m_var();
tmpvar7=m_var();
tmpvar8=m_var();
INPIPES=m_array();
OUTPIPES=m_array();
INFILES=m_array();
OUTFILES=m_array();
}

void
free_vars()
{
free_var(Au_i);
free_var(Au_key);
free_array(Aa_KEYTAB);
free_array(Aa_RESTAB);
free_var(Au_l);
free_var(Au_print_lisp_code);
free_var(Au_OK);
free_var(Au_KEY);
free_var(Au_TYPESTR);
free_var(Au_TYPE);
freefa(reg1_19);
freefa(reg1_20);
freefa(reg1_21);
freefa(reg1_22);
free_array(Aa_ARGARR);
free_var(Au_ARGNUM);
free_var(Au_IARG);
free_var(Au_DEFKEY);
free_var(Au_RES);
freefa(reg35);
free_var(Au_ARG);
free_var(Au_arg);
freefa(reg42);
freefa(reg43);
freefa(reg44);
free_var(Au_type);
freefa(reg46);
free_var(Au_typeck);
freefa(reg48);
freefa(reg49);
freefa(reg50);
free_var(Au_argnum);
freefa(reg1_55);
freefa(reg1_56);
free_var(Au_elem);
free_str(partmp0.string);
free_str(partmp1.string);
free_str(partmp2.string);
free_str(partmp3.string);
free_var(tmpvar0);
free_var(tmpvar1);
free_var(tmpvar2);
free_var(tmpvar3);
free_var(tmpvar4);
free_var(tmpvar5);
free_var(tmpvar6);
free_var(tmpvar7);
free_var(tmpvar8);
free_array(INPIPES);
free_array(OUTPIPES);
free_array(INFILES);
free_array(OUTFILES);
}
extern int	upf[];

#define GETR	(rgetrec(D[0])?(splitrec(S(D[0]), upf),(Af_FNR=Af_FNR+1),1):((*S(D[0])='\0'),NF=0,0))


#define AD0	(osplitrec(D[0], upf),(pristine=1),(D[0]->cur=INITSTR))
ad0() { AD0; }
void
assign_NF()
{
int	i;
int	tmpnum;
VARP	tmpvar;
0;
}
getfnr()
{
return (int) (Af_FNR);
}
void
resetfnr()
{
(Af_FNR=0),0;
}
void
AF_make_tabs(Ap_f)
VAR	Ap_f;
{
VARP	retval;
COPYSTR(&Ap_f);
while ((XGETR(TS((&Ap_f)),1)>0))
{
if (HINDEX(GETDSTR(0,0),"."))
{
AVARS(Au_DEFKEY,(funtmp0=AF_get_key(), funtmp0));
FREEFTM(funtmp0)
ASTR(Au_KEY,"");
ASTR(Au_TYPESTR,"");
ASTR(Au_RES,"");
}
if (FS(Au_DEFKEY),(strcmp(S(Au_DEFKEY),"KEYWORD")==0))
{
if ((!boolean_true(Au_KEY)))
{
(funtmp0=AF_get_key());
FREEFTM(funtmp0)
}
if ((!boolean_true(Au_TYPESTR)))
{
(AF_get_def_type());
}
AVARS(insert(0,Au_KEY,Aa_KEYTAB),Au_TYPESTR);
}
if (FS(Au_DEFKEY),(strcmp(S(Au_DEFKEY),"RESERVE")==0))
{
if ((!boolean_true(Au_RES)))
{
AVARS(Au_RES,GETDVARS(0,1));
}
ASTR(insert(0,Au_RES,Aa_RESTAB),"");
}
}
FREESTR(Ap_f);
}

VARP
AF_get_key()
{
VARP	retval;
retval=m_var();
ASTR(Au_KEY,SUBSTR(tmpvar0,GETDSTR(0,1),2,-1,1));
SUB(tmpvar0,makedfa(GETDSTR(0,1), 1),"",GETDVARS(0,0),1);
ASTR(retval,S(Au_KEY));
return retval;
;
return retval;
}

void
AF_get_def_type()
{
VARP	retval;
ANUM(Au_i,1.0);
while (MATCH(VGETDSTR(0,Au_i),reg35))
{
FS(Au_TYPESTR),AVARS(Au_TYPESTR,(cat(tmpvar0,2,S(Au_TYPESTR), SUBSTR(tmpvar1,GETDSTR(0,((int) postid(0,Au_i))),2,1,0))));
}
}

VARP
AF_check_key()
{
VARP	retval;
retval=m_var();
if (FS(Au_KEY),(!member(S(Au_KEY),Aa_KEYTAB)))
{
(AF_warning(VASTR((&partmp0),S(cat(tmpvar0,3,"Key ", S(Au_KEY), " not defined.")))));
ANUM(retval,0.0);
return retval;
;
}
if (FS(Au_KEY),(((strcmp(S(Au_KEY),"VERSION")==0))&&((docompN(GETDVARS(0,1),1.0,NEQ,NOREV)))))
{
(AF_warning(VASTR((&partmp0),S(cat(tmpvar0,2,"wrong version number ", GETDSTR(0,1))))));
}
ANUM(retval,1.0);
return retval;
;
return retval;
}

void
AF_print_key()
{
VARP	retval;
if (boolean_true(Au_print_lisp_code))
{
FS(Au_KEY),fprintf(pfp=stdout,"\n)\n(%s",S(Au_KEY)), (ferror(pfp)&&awkperr());
}
}

VARP
AF_get_arg()
{
VARP	retval;
retval=m_var();
if ((docomp(Au_IARG,Au_ARGNUM,GT)))
{
ASTR(retval,"");
return retval;
;
}
(AF_get_tab_type());
ASTR(Au_ARG,"");
ASTR(Au_arg,"");
while (FS(Au_TYPE),FS(Au_ARG),FS(Au_arg),(((((strcmp(S(Au_ARG),"")==0))||(((HINDEX(S(Au_TYPE),"F"))&&((strcmp(S(Au_arg),"")!=0))))))||(((((HINDEX(S(Au_TYPE),"L"))&&((strcmp(S(Au_arg),"")!=0))))&&((!((NOMATCH(S(Au_arg),reg42))&&(MATCH(S(Au_arg),reg43)))))))))
{
FS(Au_IARG),AVARS(Au_arg,insert(1,Au_IARG,Aa_ARGARR));
if (FS(Au_TYPE),((HINDEX(S(Au_TYPE),"L"))||(HINDEX(S(Au_TYPE),"F"))))
{
FS(Au_arg),AVARS(Au_ARG,((FS(Au_arg),(((strcmp(S(Au_ARG),"")!=0))&&((strcmp(S(Au_arg),"")!=0))))?(FS(Au_arg),NVASTR(tmpvar0,S(cat(tmpvar1,3,S(Au_ARG), " ", S(Au_arg))))):(FS(Au_arg),NVASTR(tmpvar0,S(cat(tmpvar3,2,S(Au_ARG), S(Au_arg)))))));
}
else
{
AVARS(Au_ARG,Au_arg);
}
}
AVARS(retval,Au_ARG);
return retval;
;
return retval;
}

void
AF_get_tab_type()
{
VARP	retval;
if (FS(Au_TYPE),NOMATCH(S(Au_TYPE),reg44))
{
if ((!boolean_true(Au_TYPESTR)))
{
ASTR(Au_TYPE,".");
}
else
{
FS(Au_TYPESTR),ASTR(Au_type,SUBSTR(tmpvar0,S(Au_TYPESTR),1,1,0));
ASTR(Au_TYPESTR,SUBSTR(tmpvar0,S(Au_TYPESTR),2,-1,1));
if ((strcmp(S(Au_type),"+")==0))
{
AVARS(Au_TYPESTR,insert(0,Au_KEY,Aa_KEYTAB));
ASTR(Au_TYPE,"");
(AF_get_tab_type());
}
else
{
if ((strcmp(S(Au_type),".")==0))
{
AVARS(Au_TYPE,(cat(tmpvar0,2,S(Au_TYPE), ".")));
}
else
{
if ((strcmp(S(Au_type),"F")==0))
{
ASTR(Au_TYPE,"F.");
}
else
{
ASTR(Au_TYPE,S(Au_type));
}
}
}
}
}
}

VARP
AF_check_arg_type()
{
VARP	retval;
retval=m_var();
if (FS(Au_TYPE),HINDEX(S(Au_TYPE),"N"))
{
FS(Au_ARG),ANUM(Au_typeck,((double)MATCH(S(Au_ARG),reg46)));
}
else
{
if (((HINDEX(S(Au_TYPE),"S"))||(HINDEX(S(Au_TYPE),"F"))))
{
ANUM(Au_typeck,1.0);
}
else
{
if (HINDEX(S(Au_TYPE),"R"))
{
FS(Au_ARG),ANUM(Au_typeck,((double)((((member(S(Au_ARG),Aa_RESTAB))||(MATCH(S(Au_ARG),reg48))))||(MATCH(S(Au_ARG),reg49)))));
}
else
{
if (HINDEX(S(Au_TYPE),"L"))
{
FS(Au_ARG),ANUM(Au_typeck,((double)MATCH(S(Au_ARG),reg50)));
}
}
}
}
if (((((MATCH(S(Au_TYPE),reg44))&&((!boolean_true(Au_typeck)))))&&(boolean_true(Au_TYPESTR))))
{
FN(Au_IARG),postid(1,Au_IARG);
ASTR(Au_TYPE,"");
ANUM(Au_typeck,((double)(-1)));
}
if ((strcmp(S(Au_TYPE),".")==0))
{
FS(Au_KEY),(AF_warning(VASTR((&partmp0),S(cat(tmpvar0,2,"Too many arguments in .", S(Au_KEY))))));
}
else
{
if ((!boolean_true(Au_typeck)))
{
FN(Au_IARG),AVARS(Au_argnum,(funtmp0=AF_min(VANUM((&partmp0),(Au_IARG->num-1)), (*Au_ARGNUM)), funtmp0));
FREEFTM(funtmp0)
FS(Au_KEY),FS(Au_TYPE),FS(Au_ARG),FS(Au_argnum),(AF_warning(VASTR((&partmp0),S(cat(tmpvar0,9,"Bad argument ", S(Au_argnum), " \"", S(Au_ARG), "\" in .", S(Au_KEY), ", ", SUBSTR(tmpvar8,S(Au_TYPE),1,1,0), " expected.")))));
}
}
AVARS(retval,Au_typeck);
return retval;
;
return retval;
}

void
AF_print_arg()
{
VARP	retval;
if (FS(Au_TYPE),(((!HINDEX(S(Au_TYPE),"L")))||((!boolean_true(Au_OK)))))
{
FS(Au_ARG),GSUB(tmpvar0,reg1_55,"\\\\",Au_ARG,0);
GSUB(tmpvar0,reg1_56,"\\\"",Au_ARG,0);
if (((!HINDEX(S(Au_TYPE),"N"))||((!boolean_true(Au_OK)))))
{
AVARS(Au_ARG,(cat(tmpvar0,3,"\"", S(Au_ARG), "\"")));
}
}
if (boolean_true(Au_print_lisp_code))
{
FS(Au_ARG),fprintf(pfp=stdout,"%s ",S(Au_ARG)), (ferror(pfp)&&awkperr());
}
}

void
AF_aprint(Apa_arr,Ap_num)
HTBP	Apa_arr;VAR	Ap_num;
{
VARP	retval;
COPYSTR(&Ap_num);
if (boolean_true(Au_print_lisp_code))
{
{
int	j1;
int	l1;
ELEMP	elem1;
ELEMP	bkt1;
j1=0;
elem1=((ELEMP)NULL);
l1=Apa_arr->num;
for (;j1<Apa_arr->len&&!(bkt1=Apa_arr->tab[j1]);j1++)
;
for (;;) {
if (!l1)
break;
else if (bkt1) {
l1--;
elem1=bkt1;
} else {
for (j1++; j1<Apa_arr->len&&!(Apa_arr->tab[j1]); j1++)
;
bkt1=(Apa_arr->tab[j1]);
if (j1<Apa_arr->len) {
l1--;
elem1=bkt1;
}
else break;
}
vassign_str(Au_elem, S(elem1));
{
if (boolean_true((&Ap_num)))
{
pfp=stdout;
fputs("\"",pfp);
fputs(S(Au_elem),pfp);
fputs("\"",pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
}
else
{
pfp=stdout;
fputs(S(Au_elem),pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
}
}
LABEL1:if (bkt1) bkt1=bkt1->next; else j1--;
}
}
}
FREESTR(Ap_num);
}

void
AF_warning(Ap_s)
VAR	Ap_s;
{
VARP	retval;
VARP funtmp0;
VAR partmp0;
VAR partmp1;
VAR partmp2;
VAR partmp3;
partmp0.string=m_str(1,"");
partmp1.string=m_str(1,"");
partmp2.string=m_str(1,"");
partmp3.string=m_str(1,"");
partmp0.num=0.0;
partmp1.num=0.0;
partmp2.num=0.0;
partmp3.num=0.0;
funtmp0=(VARP) NULL;
COPYSTR(&Ap_s);
FS(Au_FILENAME),fprintf(pfp=fileget("/dev/tty", "w"),"\n*** WARNING in %s, line %d: %s\n%s\n",S(Au_FILENAME),((int)Af_FNR),TS((&Ap_s)),GETDSTR(0,0)), (ferror(pfp)&&awkperr());
num_postid(++,Af_WARNING);
FREESTR(partmp0);
FREESTR(partmp1);
FREESTR(partmp2);
FREESTR(partmp3);
FREESTR(Ap_s);
}

VARP
AF_min(Ap_a,Ap_b)
VAR	Ap_a;
VAR	Ap_b;
{
VARP	retval;
retval=m_var();
COPYSTR(&Ap_a);
COPYSTR(&Ap_b);
AVARS(retval,(((docomp((&Ap_a),(&Ap_b),LT)))?(&(*(&Ap_a))):(&(*(&Ap_b)))));
FREESTR(Ap_a);
FREESTR(Ap_b);
return retval;
;
FREESTR(Ap_a);
FREESTR(Ap_b);
return retval;
}


main(argc, argv, envp)
int	argc;
char	*argv[],*envp[];
{
#ifdef KMALLOC
extern int Mt_trace;
if (argc>2 && !strncmp(argv[1], "-m", 2)) {
Mt_trace=2;
argc--; argv++;
}
#endif
mainloop(argc, argv, envp);
}

void
USR()
{
{
num_preid(++,Af_FNR);
}
if (HINDEX(GETDSTR(0,0),"."))
{
(funtmp0=AF_get_key());
FREEFTM(funtmp0)
AVARS(Au_OK,(funtmp0=AF_check_key(), funtmp0));
FREEFTM(funtmp0)
(AF_print_key());
AVARS(Au_TYPESTR,insert(0,Au_KEY,Aa_KEYTAB));
ASTR(Au_TYPE,"");
}
{
SUB(tmpvar0,reg1_19," ",GETDVARS(0,0),1);
SUB(tmpvar0,reg1_20," ",GETDVARS(0,0),1);
SUB(tmpvar0,reg1_21," ",GETDVARS(0,0),1);
SUB(tmpvar0,reg1_22," ",GETDVARS(0,0),1);
}
if ((docompN(Au_OK,1.0,EQ,NOREV)))
{
if (boolean_true(Au_print_lisp_code))
{
fprintf(pfp=stdout,"\n"), (ferror(pfp)&&awkperr());
}
ANUM(Au_ARGNUM,((double)split(GETDSTR(0,0),Aa_ARGARR,"")));
ANUM(Au_IARG,1.0);
while ((strcmp(TS((funtmp0=AF_get_arg(), funtmp0)),"")!=0))
{
FREEFTM(funtmp0)
AVARS(Au_OK,(funtmp0=AF_check_arg_type(), funtmp0));
FREEFTM(funtmp0)
(AF_print_arg());
}
CFREEFTM(funtmp0)
}
}

void
BEGIN()
{
{
(Af_FNR=0);
}
{
(AF_make_tabs(VASTR((&partmp0),"unipen.def")));
fprintf(pfp=fileget("/dev/tty", "w"),"\n > > > > > >  UNIPEN 1.0 parser  < < < < < < <"), (ferror(pfp)&&awkperr());
fprintf(pfp=fileget("/dev/tty", "w"),"\nCopyright (c) 1994 - I. Guyon, AT&T Bell Labs -\n"), (ferror(pfp)&&awkperr());
ANUM(Au_i,0.0);
{
int	j1;
int	l1;
ELEMP	elem1;
ELEMP	bkt1;
j1=0;
elem1=((ELEMP)NULL);
l1=Aa_KEYTAB->num;
for (;j1<Aa_KEYTAB->len&&!(bkt1=Aa_KEYTAB->tab[j1]);j1++)
;
for (;;) {
if (!l1)
break;
else if (bkt1) {
l1--;
elem1=bkt1;
} else {
for (j1++; j1<Aa_KEYTAB->len&&!(Aa_KEYTAB->tab[j1]); j1++)
;
bkt1=(Aa_KEYTAB->tab[j1]);
if (j1<Aa_KEYTAB->len) {
l1--;
elem1=bkt1;
}
else break;
}
vassign_str(Au_key, S(elem1));
{
postid(0,Au_i);
}
LABEL2:if (bkt1) bkt1=bkt1->next; else j1--;
}
}
ToStr(Au_i),pfp=fileget("/dev/tty", "w");
fputs("--> ",pfp);
fputs(S(Au_i),pfp);
fputs(" keys",pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
ANUM(Au_i,0.0);
{
int	j1;
int	l1;
ELEMP	elem1;
ELEMP	bkt1;
j1=0;
elem1=((ELEMP)NULL);
l1=Aa_RESTAB->num;
for (;j1<Aa_RESTAB->len&&!(bkt1=Aa_RESTAB->tab[j1]);j1++)
;
for (;;) {
if (!l1)
break;
else if (bkt1) {
l1--;
elem1=bkt1;
} else {
for (j1++; j1<Aa_RESTAB->len&&!(Aa_RESTAB->tab[j1]); j1++)
;
bkt1=(Aa_RESTAB->tab[j1]);
if (j1<Aa_RESTAB->len) {
l1--;
elem1=bkt1;
}
else break;
}
vassign_str(Au_key, S(elem1));
{
postid(0,Au_i);
}
LABEL3:if (bkt1) bkt1=bkt1->next; else j1--;
}
}
ToStr(Au_i),pfp=fileget("/dev/tty", "w");
fputs("--> ",pfp);
fputs(S(Au_i),pfp);
fputs(" reserved strings",pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
pfp=fileget("/dev/tty", "w");
fputs("Detect syntax errors, except too few arguments",pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
AVARS(Au_print_lisp_code,((boolean_true(Au_l))?(&(*Au_l)):(NVANUM(tmpvar0,0.0))));
if (boolean_true(Au_print_lisp_code))
{
fprintf(pfp=stdout,"\n(KEYWORD\n"), (ferror(pfp)&&awkperr());
(AF_aprint(Aa_KEYTAB, VANUM((&partmp1),0.0)));
fprintf(pfp=stdout,")\n(RESERVE\n"), (ferror(pfp)&&awkperr());
(AF_aprint(Aa_RESTAB, VANUM((&partmp1),1.0)));
}
}
}

void
END()
{
notseenend=0;
ADSTR(0,""); NF=0; assign_NF();
{
if (boolean_true(Au_print_lisp_code))
{
fprintf(pfp=stdout,")\n"), (ferror(pfp)&&awkperr());
}
pfp=fileget("/dev/tty", "w");
fputs(FTOA(tmpstr0,(Af_WARNING+0)),pfp);
fputs(" warning(s).",pfp);
fputc('\n', pfp)
;
if (ferror(pfp)) awkperr();
}
}
int	upf[]={-1};
