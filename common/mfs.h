//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

//Not to be confused with MFS for the AVR.

#ifndef _MFS_H
#define _MFS_H

#include "mem.h"
#include "c_types.h"
#include "mystuff.h"

//SPI_FLASH_SEC_SIZE      4096

//If you are on a chip with limited space, MFS can alternatively live here, with a max size of 180kB.
#define MFS_ALTERNATIVE_START 0x10000

#define MFS_STARTFLASHSECTOR  0x100
#define MFS_START	(MFS_STARTFLASHSECTOR*SPI_FLASH_SEC_SIZE)
#define MFS_SECTOR	256


#define MFS_FILENAMELEN 32-8

//Format:
//  [FILE NAME (24)] [Start (4)] [Len (4)]
//  NOTE: Filename must be null-terminated within the 24.
struct MFSFileEntry
{
	char name[MFS_FILENAMELEN];
	uint32 start;  //From beginning of mfs thing.
	uint32 len;
};


struct MFSFileInfo
{
	uint32 offset;
	uint32 filelen;
};



//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t ICACHE_FLASH_ATTR MFSOpenFile( const char * fname, struct MFSFileInfo * mfi );
int32_t MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi ); //returns # of bytes left in file.



#endif


