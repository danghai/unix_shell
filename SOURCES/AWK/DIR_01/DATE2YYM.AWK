########################################################################
function date2date(date1)
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#       natewild@mbnet.mb.ca    Original function modified to return
#                               date in international format.
#                               Modified 94/12/03 By Nathan T. Wild
#
# Convert output from date(1) to YY/MM/DD format
#	Thu Mar 30 15:22:00 EST 1989
#	$1  $2  $3   $4     $5  $6
#
{
	MONTH[1] = "Jan"
	MONTH[2] = "Feb"
	MONTH[3] = "Mar"
	MONTH[4] = "Apr"
	MONTH[5] = "May"
	MONTH[6] = "Jun"
	MONTH[7] = "Jul"
	MONTH[8] = "Aug"
	MONTH[9] = "Sep"
	MONTH[10] = "Oct"
	MONTH[11] = "Nov"
	MONTH[12] = "Dec"

	NUMBER[1] = "01"
	NUMBER[2] = "02"
	NUMBER[3] = "03"
	NUMBER[4] = "04"
	NUMBER[5] = "05"
	NUMBER[6] = "06"
	NUMBER[7] = "07"
	NUMBER[8] = "08"
	NUMBER[9] = "09"
	NUMBER[10] = "10"
	NUMBER[11] = "11"
	NUMBER[12] = "12"
	
	split(date1,date," ")

	YEAR = substr(date[6],3,2)

	if (date[3] > 10 )
		{
		DD = date[3]
		}
	else
		{
		DD = NUMBER[date[3]]
		}
	
	for (i=1; i<13; i++)
		{
		if (date[2] == MONTH[i] )
			{
			MM = NUMBER[i]
			}
		}

	DATE = YEAR "/" MM "/" DD
	return(DATE)
}
