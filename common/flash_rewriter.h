//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#ifndef _FLASH_REWRITER_H
#define _FLASH_REWRITER_H

//Unusual, but guarentees that the code resides in IRAM.
/*
Usage:
	[from_address]:[to_address]:[size]:[MD5(key+data)]:[from_address]:[to_address]:[size]:[MD5(key+data)][extra byte required]

	NOTE: YOU MUST USE 4096-byte-aligned boundaries.

	Note: this will modify the text in "command"
*/


extern int (*GlobalRewriteFlash)( char * command, int commandlen );

#endif
