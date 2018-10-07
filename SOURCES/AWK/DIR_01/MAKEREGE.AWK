##################################################################
function makeRE(word)
#
#	bergman@panix.com	Please observe the GNU
#				copyleft restrictions on any
#				redistribution.
#
#	
#	makeRE	make "word" into a Regular Expression
#
#		joe ==> [Jj][Oo][Ee]*
#
#	Example:
#		pattern = makeRE(FILENAME)
#
{
	UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	LOWER = "abcdefghijklmnopqrstuvwxyz"
	wordRE = ""
	cpos = ""
	curchar = ""

	for(i=1;i <= length(word);i++)
		{
		curchar = substr(word,i,1)	
		if ( (cpos = index(LOWER,curchar)) > 0)
			{
			UC = substr(UPPER,cpos,1);
			LC = curchar;
			wordRE= wordRE "[" UC LC "]"
			}
		else
			{
			if((cpos = index(UPPER,curchar)) > 0)
				{
				LC = substr(LOWER,cpos,1);
				UC = curchar;
				wordRE= wordRE "[" UC LC "]"
				}
			else
				{
				wordRE= wordRE curchar
				}
				
			}
		}
	wordRE= wordRE "*"
	gsub(/ /,"",wordRE)
	return(wordRE)
}
