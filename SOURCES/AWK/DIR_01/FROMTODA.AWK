BEGIN { # data
  Days[1] = 31 # jan
  Days[2] = 28
  Days[3] = 31  
  Days[4] = 30
  Days[5] = 31
  Days[6] = 30
  Days[7] = 31
  Days[8] = 31
  Days[9] = 30
  Days[10] = 31
  Days[11] = 30
  Days[12] = 31 # dec

  Month[1] = "Jan"
  Month[2] = "Feb"
  Month[3] = "Mar"
  Month[4] = "Apr"
  Month[5] = "May"
  Month[6] = "Jun"
  Month[7] = "Jul"
  Month[8] = "Aug"
  Month[9] = "Sep"
  Month[10] = "Oct"
  Month[11] = "Nov"
  Month[12] = "Dec"
}

function is_leap(year)
{
  return year % 4 == 0 &&
	 ( year % 100 != 0 ||
	   year % 400 == 0 )
}


# Convert a date to an absolute day.
# Jan 1, 0001 is day 1
# Jan 2, 0001 is day 2 ...
function date_to_abs_day(month, day, year,
   abs_day, i)
{
   year -= 1
   abs_day = 365*year

   # adjust for leap years
   abs_day +=   int(year/4) - int(year/100) + int(year/400)  

   # add for months of input year
   for( i = 1 ; i < month ; i++) abs_day += Days[i]
   if ( month > 2 && is_leap(year+1) ) abs_day++

   # add days of input month
   return abs_day + day
}

# Convert absolute day to month:day:year

function abs_day_to_date( abs_day,
   full_years, days_left, feb_days, month, next_days_left)
  
{
   full_years = int(abs_day/365)
   days_left = abs_day % 365

   # adjust for leap years
   days_left -=   int(full_years/4) - int(full_years/100) \
		  + int(full_years/400) 
   while( days_left <= 0 )
      days_left += 365 + is_leap(full_years--)

   if ( days_left <= Days[1] ) month = 1  
   else
   {
      days_left -= Days[1]
      feb_days = 28 + is_leap(full_years+1)
      month = 2

      if ( days_left > feb_days )
      {
	 days_left -= feb_days

	 while(1)
	 {
	    month++
	    if ((next_days_left = days_left - Days[month]) <= 0)
		  break

	    days_left = next_days_left
	 }
      }
   }
   return sprintf("%d/%02d/%04d", month, days_left, full_years+1)
}

BEGIN {
   amt = ARGV[1]
   # guard against integer overflow
   if ( amt > 2^31 )
   {
      print "Eternity"
      exit
   }

   FS = "/"

   "date +%D" | getline

   # is date consistent across systems?
   # since I dont know guard against it
   if ( NF != 3 || $1 < 1 || $1 > 12 ||
	$2 < 1 || $2 > 31 || $3 < 0 || $3 > 99 )
   {
      print "unexpected output from \"date +%D\""
      print "expected mm/dd/yy"
      exit
   }

   # this line will break on Jan 01 2000
   abs_day = date_to_abs_day($1,$2,1900+$3)

   abs_day += int(amt)
   if ( abs_day < 1 ) # cannot go back past Jan 01 0001
   {
      print "A long time ago"
      exit
   }

   split(abs_day_to_date(abs_day), X)

   print Month[X[1]], X[2], X[3]
}




