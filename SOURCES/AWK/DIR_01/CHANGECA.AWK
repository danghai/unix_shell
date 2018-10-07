######################################################################
function changecase(string,newcase)
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#
#	Changes the case of the given string to the specified case
#	Example:
#		LINE = changecase($0,"upper")

{
	
	if ( newcase != "upper" && newcase != "lower" ) 
		{
		return(NULL)
		}
	else
		{
		RESULT = ""
		Ucase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		Lcase = "abcdefghijklmnopqrstuvwxyz"
	
		for(i=1;i <= length(string);i++)
			{
			if ( newcase == "upper" )
				{
				if( (cpos = index(Lcase,c = substr(string,i,1))) > 0)
					{
					c = substr(Ucase,cpos,1);
					}
				}
			else
				{
				if( (cpos = index(Ucase,c = substr(string,i,1))) > 0)
					{
					c = substr(Lcase,cpos,1);
					}
				}
			RESULT = RESULT c;
			}
		return (RESULT)
		}
}
