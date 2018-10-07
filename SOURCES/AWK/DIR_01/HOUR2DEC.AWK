##################################################################
function hours2dec(TIME)
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#	Convert time in HOURS:MINUTES to
#			HOURS.MINUTES with decimal minutes
#
{
	split(TIME,time2,":")
	TIME = time2[1] + (time2[2]/60)
	return(TIME)
}
