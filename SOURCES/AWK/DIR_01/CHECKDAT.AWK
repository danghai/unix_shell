function checkdata(data,type,len) {
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#
#	Check type of data. Returns FALSE if data does
#	not match intended type or specified length. Length
#	specification is optional. Returns TRUE otherwise.
#
#	Usage:
#		if ( checkdata(LineLength,"integer",2) == "FALSE" )
#
#	CLASS			DESC.
#	unsignint		unsigned integer
#	integer			optionally signed integer
#	fpnumber		optionally signed number w. opt. decimal
#	ucletters		uppercase letters
#	lcletters		lowercase letters
#	letters			mixed case letters
#	alphanum		letters + integer
#
	if (len != "" ) 
		{
		if (length(data) != len )
			{
			return "FALSE"
			}
		}
	if (type=="unsignint") 
		{
		if (data !~ /^[0-9]*$/ )
			{
			return "FALSE"
			}
		}
	if (type=="integer") 
		{
		if (data !~ /^[+-][0-9]*$/ )
			{
			return "FALSE"
			}
		}
	if (type=="fpnumber") 
		{
		if (data !~ /^[+-][0-9]*[\.][0-9]*$/ )
			{
			return "FALSE"
			}
		}
	if (type == "ucletters")
		{
		if (data !~ /^[A-Z]*$/ )
			{
			return "FALSE"
			}
		}
	if (type == "lcletters")
		{
		if (data !~ /^[a-z]*$/ )
			{
			return "FALSE"
			}
		}
	if (type == "letters")
		{
		if (data !~ /^[A-Za-z]*$/ )
			{
			return "FALSE"
			}
		}
	if (type == "alphanum" )
		{
		if (data !~ /^[A-Za-z0-9]*$/ )
			{
			return "FALSE"
			}
		}
	return TRUE
}
