function Expanded(theUPC) {
  local newUPC = "";

	if ((substr(theUPC, 6, 1) == "0") ||						## 0, 1 or 2 Means Use Last Digit
	   (substr(theUPC, 6, 1) == "1") ||					
	   (substr(theUPC, 6, 1) == "2")) {					
		newUPC = substr(theUPC, 1, 2);						## Add In First Two Digits
		newUPC = newUPC substr(theUPC, 6, 1);					## Last digit Becomes Third Digit
		newUPC = newUPC "0000";								## Add Four Zeros After The Third Digit
		newUPC = newUPC substr(theUPC, 3, 3);					## Add Remaining 3 Digits
	}

	else if (substr(theUPC, 6, 1) == "3") {						## 3 Means Ignore Last Digit.
		newUPC = substr(theUPC, 1, 3);						## Add In First 3 Digits
		newUPC = newUPC "00000";								## Add Five Zeroes
		newUPC = newUPC substr(theUPC, 4, 2);					## Add In Remaining Two Digits
	}

	else if (substr(theUPC, 6, 1) == "4") {						## 4 Means Ignore Last Digit
		newUPC = substr(theUPC, 1, 4);						## Add In First 3 Digits
		newUPC = newUPC "00000";								## Add Five Zeroes
		newUPC = newUPC substr(theUPC, 5, 1);					## Add In Remaining Two Digits
	}

	else if ((substr(theUPC, 6, 1) == "5") ||					## 5, 6, 7, 8 & 9 Means
		   (substr(theUPC, 6, 1) == "6") ||					## Last Digit Remains		   
		   (substr(theUPC, 6, 1) == "7") ||					## Last Digit After
		   (substr(theUPC, 6, 1) == "8") ||					## Expansion...
		   (substr(theUPC, 6, 1) == "9")) {					## 
		newUPC = substr(theUPC, 1, 5);						## Add In First 3 Digits
		newUPC = newUPC "0000";								## Add Five Zeroes
		newUPC = newUPC substr(theUPC, 6, 1);					## Add In Remaining Two Digits
	}
	return newUPC;
}
