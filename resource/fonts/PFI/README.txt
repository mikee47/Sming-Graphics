=====================================
PORTABLE FONT INDEX (PFI) FILE FORMAT
=====================================


F1 (ASCII-formatted) index files are defined as:

	F1						<-- Font index file signature (F1=ASCII, F4=Binary)
	Basic					<-- Font name (may contain spaces).
	7x11					<-- Font dimensions WxH. For proportional fonts, use ?xH (ie "?x11").
	FFFD 98 44				<-- Character definitions start here. The very first character is the 'default character' (drawn for characters not defined in this index file).
	20						
	! 0 0
	" 7 0					  First character represents the character.  If there are 2, 3, or 4 digits,
	# 14 0					  then the number represents the character code (in HEX). For example, "20" represents the Space character (20h),
	$ 21 0					  and FFFD represents the Unicode Replacement character.
	% 28 0
	& 35 0					  The two parameters that follow, represent the X,Y offset (in DEC)
	' 42 0					  into the font image file of where the character is defined.
	( 49 0
	) 56 0					  For proportional fonts, the first parameter represents the width (in DEC)
	* 63 0					  of the character, followed by X,Y.
	+ 70 0
	, 77 0					  If the X,Y parameters are missing, then no pixels will be shown
	- 84 0					  (ie. the Space character).
	. 91 0
	/ 98 0


F4 (binary-formatted) index non-porportional file:

	46 34 0A				<-- F4			ASCII  The bytes of the first three lines are identical to F1 format.
	42 61 73 69 63 0A		<-- Basic		ASCII
	37 78 31 31 0A			<-- 7x11		ASCII
	FF										<<<<<< This byte is a (FFh) marker that signifies the data beyond this point is binary.
	FF FD 62 2C				<-- FFFD 98 44	BINARY First two bytes (FFFDh) represent the character code in UInt16 (FFh=high byte, FDh=low byte).
	00 20 00				<-- 20			BINARY The third byte for each record represents number of parameters.
	00 21 02 00 00			<-- ! 0 0		BINARY
	00 22 02 07 00			<-- " 7 0		BINARY
	00 23 02 0E 00			<-- # 14 0		BINARY
	00 24 02 15 00			<-- $ 21 0		...
	00 25 02 1C 00			<-- % 28 0
	00 26 02 23 00			<-- & 35 0
	00 27 02 2A 00			<-- ' 42 0
	00 28 02 31 00			<-- ( 49 0
	00 29 02 38 00			<-- ) 56 0
	00 2A 02 3F 00			<-- * 63 0
	00 2B 02 46 00			<-- + 70 0
	00 2C 02 00 0B			<-- , 0 11
	00 2D 02 07 0B			<-- - 7 11
	00 2E 02 0E 0B			<-- . 14 11
	00 2F 02 15 0B			<-- / 21 11


F4 (binary-formatted) index porportional file:

	46 34 0A				<-- F4			ASCII  The bytes of the first three lines are identical to F1 format.
	42 61 73 69 63 0A		<-- Basic		ASCII
	3F 78 31 31 0A			<-- ?x11		ASCII  The question mark (?=3Fh) makes it porportional.
	FF										<<<<<< This byte is a (FFh) marker that signifies the data beyond this point is binary.
	FF FD 03 07 62 2C		<-- FFFD 98 44	BINARY First two bytes (FFFDh) represent the character code in UInt16 (FFh=high byte, FDh=low byte).
	00 20 01 3				<-- 20 3		BINARY The third byte for each record represents number of parameters.
	00 21 03 02 00 00		<-- ! 2 0 0		BINARY The fourth byte represents the width of the character
	00 22 03 04 02 00		<-- " 4 2 0		BINARY The fifth and sixth bytes represet the X and Y offset into the font image file.
	00 23 03 06 06 00		<-- # 6 6 0		...
	00 24 03 06 0C 00		<-- $ 6 12 0	
	00 25 03 07 12 00		<-- % 7 18 0
	00 26 03 07 19 00		<-- & 7 25 0
	00 27 03 02 20 00		<-- ' 2 32 0
	00 28 03 03 22 00		<-- ( 3 34 0
	00 29 03 03 25 00		<-- ) 3 37 0
	00 2A 03 06 28 00		<-- * 6 40 0
	00 2B 03 06 2E 00		<-- + 6 46 0
	00 2C 03 03 00 0B		<-- , 3 0 11
	00 2D 03 06 07 0B		<-- - 6 7 11
	00 2E 03 03 0E 0B		<-- . 3 14 11
	00 2F 03 05 15 0B		<-- / 5 21 11


NOTE:
1. ASCII-formatted lines can be terminated with a CR or LF or any combination of the two.
2. The very first character defined will be used as the 'default character' (drawn for characters not defined in this index file).



