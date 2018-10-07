##################################################################
function pwcheck(LOGIN,SCOPE)
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#	check if given LOGIN name is in local /etc/passwd,
#	system wide yppasswd, or both. 
#	SCOPE can have the values:
#		LOCAL
#		YP
#		BOTH
#	Usage:
#		INPassWordFile=pwcheck($1,$2)
#
#	pwcheck() returns FALSE if the SCOPE is local and
#	there is an entry in /etc/passwd "pointing" to the 
#	yellow pages, ie:
#		+smith::::::::
#	I may want to change this so that pwcheck() can be
#	used to look for passwd/YP inconsistencies (ie local entry
#	w/o corresponding YP entry).
#
{
	if (LOGIN == "" )
		{
		return(NULL)
		}
	if (SCOPE == "" )
		{
		return(NULL)
		}

	LOCALPWF = ""
	YPPWF = ""

	"grep -i " LOGIN " /etc/passwd"| getline LOCALPWF
	close ("grep -i " LOGIN " /etc/passwd")
	if ( LOCALPWF == "" || LOCALPWF ~ /^\+/) 
		{
		LOCALPWF = "FALSE"
		}
	else
		{
		LOCALPWF = "TRUE"
		}
		
	if (SCOPE == "LOCAL" ) 
		{
		return(LOCALPWF)
		}

	YPPWF = system("ypmatch " LOGIN " passwd 1> /dev/null 2> /dev/null ")
	if ( YPPWF == "1" ) 
		{
		YPPWF = "FALSE"
		}
	else
		{
		YPPWF = "TRUE"
		}

	if (SCOPE == "YP" ) 
		{
		return(YPPWF)
		}

	if (SCOPE == "BOTH")
		{
		if (YPPWF == "TRUE" && LOCALPWF == "TRUE" ) 
			{
			return("TRUE")
			}
		else
			{
			return("FALSE")
			}
		}
}
