# Archived By Nathan T. Wild [natewild@mbnet.mb.ca]
# Provided/Written By: bob@trebor.iglou.com

mawk '
BEGIN {
  # get into an array the numbers of days of the months:
  split("31 28 31 30 31 30 31 31 30 31 30 31",M_Len) ; M_Len[0] = 0
  # get into an array the number of days in a year prior to each month:
  for (i=1;i<=12;i++) J_Days[i] = M_Len[i-1] + J_Days[i-1]
  # get into an array the weekday name abbreviations:
  split("Mon Tue Wed Thu Fri Sat",W_Days) ; W_Days[0] = "Sun"
}
# skip blank lines:
/^[ \t]*$/ { next }
{
  # get the field containing the date into a variable:
  YMD = $1
  # get the fields of the date into an array:
  split(YMD,ymd,"/")
  # if the year changes, get information for the new year:
  if ( ymd[1] != Year ) {
    # remember the year:
    Year = ymd[1]
    # check for it being a Leap Year:
    Leap_Day = Leap_Year(Year)
    # determine on what day of the week its 0th day falls on:
    Day0 = Get_Day0(Year)
  }
  # the day number in the week will be the sum of:
  #  the day of the week number of day 0 of the year,
  #  + number of days in the year prior to the first day of the month,
  #  + the the date in the month,
  #  + the value of Leap_Day for months after February.
  # modulo 7
  if ( ymd[2]+0 <= 2 ) DOW = ( Day0 + J_Days[ymd[2]+0] + ymd[3] ) % 7
  else DOW = ( Day0 + J_Days[ymd[2]+0] +ymd[3] + Leap_Day ) % 7
  # count the the occurances for each day of the week:
  Count[DOW]++
  # count the weeks:
  This_Week[DOW] = ymd[1] ymd[2] ymd[3]
  if ( This_Week[DOW] != Remember_Week[DOW] ) {
     Weeks[DOW]++
     Remember_Week[DOW] = This_Week[DOW]
  }
}
END {
  # find the greatest value for Total_Weeks:
  for (DOW=0;DOW<=6;DOW++) {
    if ( Weeks[DOW] > Total_Weeks ) Total_Weeks = Weeks[DOW]
  }
  print "\t==============="
  print "\tFor", Total_Weeks, "weeks:"
  print "\t==============="
  print "\tDAY       COUNT\n\t===       ====="
  for (DOW=0;DOW<=6;DOW++) {
    printf( "\t%s: %10d\n", W_Days[DOW], Count[DOW] )
    Total += Count[DOW]
  }
  print   "\t          -----"
  printf( "\t     %10d\n", Total )
  print "\n\tDAY        AVG\n\t===       ====="
  for (DOW=0;DOW<=6;DOW++)
    printf( "\t%s: %10.2f\n", W_Days[DOW], Count[DOW] / Total_Weeks )
}
function Get_Day0(YR) {
  # get the number of days in the first week of the current year
  # by printing a calendar for January of the year and getting
  # the number of fields in its third line:
  CMD1 = "/usr/bin/cal 1 "
  CMD2 = sprintf( " | mawk %cNR==3{print NF;exit}%c", 39, 39 )
  CMD  = CMD1 YR CMD2
  CMD | getline Days_in_First_Week
  # return the weekday number (Sunday=0) for the day before the
  # first day of the year (ie: Day 0):
  return ( 7 - Days_in_First_Week - 1 )
}
function Leap_Year(YR) {
  # return 1 if the year is a Leap Year, else return 0
  if ( YR % 4 == 0 && ( YR % 100 != 0 || YR % 400 == 0 ) ) return 1
  else return 0
} '

