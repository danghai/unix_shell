#
#	Printer.awk
#
#	B Clarke	7/94
#
#	Creates PCL5 escape sequences for Laser Printer
#
#	Modified	B Clarke 12/94	Comments added
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
#
#    ---------------------------------------------------
#
#	Fixed-Width Font
#
function FixedFont() {
	printf "\033(s0P\033)s0P";
}

#
#	---------------------------------------------------
#
#	Proportional Width Font
#
function ScaledFont() {
	printf "\033(s1P\033)s1P";
}

#
#	---------------------------------------------------
#
#	Set Text Size and Line spacing
#	Quick and dirty hack 
#
function TextPoints(SIZE) {
	printf "\033(s" SIZE "V\033)s" SIZE "V";
	VMI=SIZE*0.7;
	VMI=int(VMI);
	printf "\033&l" VMI "C";
}

#
#	---------------------------------------------------
#
#	set text weights
#
function TextWeight(WEIGHT)	{
	WEIGHT=WEIGHT"";
	PEN["light"]=-3;
	PEN["medium"]=0;
	PEN["bold"]=3;
	PEN["black"]=5;
	USE_PEN=PEN[WEIGHT];
	printf "\033(s" USE_PEN "B\033)s" USE_PEN "B";
}

#
#	---------------------------------------------------
#
#	set text style
#
function FontStyle(STYLE)	{
	STYLE=STYLE"";
	TYPE["normal"]=0;
	TYPE["italic"]=1;
	TYPE["condensed"]=4;
	TYPE["expanded"]=24;
	TYPE["outline"]=32;
	USE_TYPE=TYPE[STYLE];
	printf "\033(s" USE_TYPE "S\033)s" USE_TYPE "S";
}

#
#	---------------------------------------------------
#
#	Select typeface
#
function Font(FONT)	{
	FONT=FONT"";
	TYPEFACE["courier"]=3;
	TYPEFACE["helvetica"]=4;
     TYPEFACE["gothic"]=5;
     TYPEFACE["times"]=4101;
     TYPEFACE["univers"]=4148;
     TYPEFACE["arial"]=16602;
	USE_FACE=TYPEFACE[FONT];
     printf "\033(s" USE_FACE "T\033)s" USE_FACE "T";
}

#
#	---------------------------------------------------
#
#	Save cursor position
#
function SaveCursor()	{
	printf "\033&f0S";
}

#
#	---------------------------------------------------
#
#	Restore Cursor position
#
function RestoreCursor()	{
	printf "\033&f1S";
}

#
#	---------------------------------------------------
#
#	Set Underline on
#
function Underline() {
	printf "\033&d0D";
}

#
#	---------------------------------------------------
#
#	Setr underline off
#
function UnderlineOff() {
	printf "\033&d@";
}

#
#	---------------------------------------------------
#
#	Print Both sides of paper
#
function DoubleSide()	{
	printf "\033&l1S";
}

#
#	---------------------------------------------------
#                               
#	Print single side of paper
#
function SingleSide() {
	printf "\033&l0S";
}

#
#	---------------------------------------------------
#
#	Reset printer (use at start and end of job)
#
function ResetPrinter()	{
	printf "\033E";
	printf "\033-12345X";
}

#
#	---------------------------------------------------
#
#	Force Formfeed
#
function FormFeed() {
	printf "\014";
}

#
#	---------------------------------------------------
#
#	Number of Copies
#
function Copies(COPIES) {
	printf "\033&l" COPIES "X";
}

#
#	---------------------------------------------------
#
#	Set to tray witth Letterhead paper
#
function Letterhead()	{
	printf "\033&5H";
}

#
#	---------------------------------------------------
#
#	Set to Tray with plain paper
#
function NoLetterhead() {
	printf "\033&lH";
}

#
#	---------------------------------------------------
#
#	Set Standard Margins
# 	careful - there may be some problems here
#
function StandardMargins()	{
	printf "\033&a10L";
	printf "\033&a8M";
	printf "\033&l6E";
#	printf "\033&l58F";
}
