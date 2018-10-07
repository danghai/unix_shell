
########################################################################
function makefit(line,LL) {
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#
#	Makefit takes a string and breaks it into as many lines as
#	necessary to fit the words into the given line length (LL).
#	The line is split at spaces. Makefit does the printing, it
#	returns TRUE if it had to split the line, and FALSE if it
#	didn't.
#	
#	Usage:
#		makefit(text,LineLength)
#
	splitline = "FALSE"

	if (line == "")
		{
		return splitline
		}
	if (length(line) <= LL)
		{
		printf("%s\n",line)
		return splitline
		}
	else
		{
		split(line,word," ")
		i = 1
		wordc=0
		while (i in word)
			{
			wordc = i
			i ++
			}
		i=0
		testwd = ""
		for ( i = 1; i <= wordc ; i ++)
			{
			if ( (length(testwd) + length(word[i])) <= LL )
				{
				if (testwd == "")
					{
					testwd = testwd word[i]
					}
				else
					{
					testwd = testwd " " word[i]
					}
				}
			else
				{
				if (splitline=="FALSE")
					{
					printf("%s\n",testwd)
					}
				else
					{
					printf("%s\n",testwd)
					}
				testwd=word[i]
				splitline = "TRUE"
				}
			}

		if (splitline=="FALSE")
			{
			printf("%s\n",testwd)
			}
		else
			{
			printf("%s\n",testwd)
			}
		}
	return splitline
}
