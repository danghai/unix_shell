#	SQL_Functions.awk
#
#	A Groom B Clarke	4/94
#
#	Modified	B Clarke 12/94	PrintColumns added
#					PrintColumnsFile added
#					Comments
#	Modified	N Wild	 01/95	Made Portable
#
#	Generic Awk functions for SQL reporting
#
#    ***************************************************
#    *                                                 *
#    *        USE RIGHTS AND DISCLAIMER                *
#    *                                                 *
#    * THIS FILE CONTAINS AWK FUNCTIONS PREPARED FOR   *
#    * THE CHRISTCHURCH CITY COUNCIL, NZ.  FOR THIS    *
#    * REASON THEY ARE NOT NECESSARILY GENERIC AND MAY *
#    * REQUIRE AMENDMENT TO MEET THE REQUIREMENTS OF   *
#    * YOUR ORGANISTIONAL OR NATIONAL STANDARDS.       *
#    *                                                 *
#    * THESE AWK FUNCTIONS ARE FREE TO BE USED WITHOUT *
#    * WITHOUT REFERENCE OR RECOGNITION TO THE AUTHORS *
#    * OR THEIR EMPLOYERS.  NO LIABILITY IS ASSUMED BY *
#    * THE ABOVE FOR THE CONSEQUENCES OF ANY FAILURE   *
#    * OF ANY OF THE CONTENTS.  SOME OF THE CONTENTS   *
#    * MAY NOT HAVE BEEN TESTED RIGOROUSLY OR AT ALL.  *
#    * THE CONTENTS WILL NOT BE SUPPORTED.  IF YOU     *
#    * WISH TO CONTACT THE AUTHORS WITH QUERIES,       *
#    * CORRECTIONS, IDEAS OR TO SUPPLY EXAMPLES OF     *
#    * YOUR OWN CODE, YOUR RESPONSE WILL BE GRATEFULLY *
#    * ACCEPTED BY CLARKEB@CCC.GOVT.NZ                 *
#    *                                                 *
#    ***************************************************
#
#	---------------------------------------------------------
#
#	Trims extra Spaces in String
#
function TrimSpaces(string) {
        gsub("^ +", "", string);
        gsub(" +", " ", string);
        gsub(" +$", "", string);
        return string;
}
#
#	---------------------------------------------------------
#
#	Converts UCase To Capitalised
#
function LowerWord(string) {
	numwords = split(string, word, " ");
	whole_str = "";
	for (i=1; i<=numwords; i++) {
		if (match(word[i], "^PO$") == 0) {
			word[i] = tolower(word[i]);
			temp = toupper(substr(word[i], 1, 1));
			sub("^.", temp, word[i]);
		}
		if (match(word[i], "-.") != 0) {
			temp = substr(word[i], RSTART, RLENGTH);
			sub(temp, toupper(temp), word[i]);
		}			
		whole_str = whole_str""word[i]" " ;
	}
	return whole_str;
}

#
#	---------------------------------------------------------
#
#	Converts date ansi to Full date
#
function nice_date(date_str) {
	if (match(date_str,"1858") != 0)
		date_str="Null";
	else	{
     	months[1]="January";
     	months[2]="February";
     	months[3]="March";
     	months[4]="April";
     	months[5]="May";
     	months[6]="June";
     	months[7]="July";
     	months[8]="August";
     	months[9]="September";
     	months[10]="October";
     	months[11]="November";
     	months[12]="December";
     	gsub("-0", "-", date_str);
     	split(date_str, array, "-");
     	month_str=months[array[2]];
     	date_str=sprintf("%s %s %s",array[3],month_str,array[1]);
	}
	return date_str;
}

#
#	---------------------------------------------------------
#
#	Puts Linefeeds in strings > 80 chars
#
function Linefeed(string) {
	posn = 1;
	len = length(string);
		if (len < 80) {
			print string;
		}
		else {
			while (posn + 80 < len) {
				extr = substr(string, posn, 80);
				match(extr, " [^ ]*$");
				posn += RSTART;
				print substr(extr, 1, RSTART);
			}
			print substr(string, posn, len - posn + 1);
		}
}


#
#	---------------------------------------------------------
#
#	Prints Fields to predefined columns
#
function PrintColumns(strings, cols) {
	tot_length = 0;
	for (i in strings) {
		if (cols[i] > 1)
			printf ("%*s%s", cols[i] - tot_length - 1, " ", strings[i]);
		else
			printf ("%s", strings[i]);
		tot_length = cols[i] + length(strings[i]) - 1;
	}
	printf ("\n");
}

#
#	---------------------------------------------------------
#
#	Prints fields to predefined columnns in output file
#
function PrintColumnsFile(strings, cols, file) {
	tot_length = 0;
	for (i in strings) {
		if (cols[i] > 1)
			printf ("%*s%s", cols[i] - tot_length - 1, " ", strings[i]) >> file;
		else
			printf ("%s", strings[i]) >> file;
		tot_length = cols[i] + length(strings[i]) - 1;
	}
	printf ("\n") >> file;
}


