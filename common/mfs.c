//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "mystuff.h"
#include "mfs.h"
#include "spi_flash.h"
#include "ets_sys.h"

uint32 mfs_at = 0;

void ICACHE_FLASH_ATTR FindMPFS()
{
	uint32 mfs_check[2];
	EnterCritical();
	flashchip->chip_size = 0x01000000;

	spi_flash_read( MFS_START, mfs_check, sizeof( mfs_check ) );
	if( strncmp( "MPFSMPFS", mfs_check, 8 ) == 0 ) { mfs_at = MFS_START; goto done; }
	
	printf( "MFS Not found at regular address (%08x).\n", mfs_check[0], mfs_check[1] );

	spi_flash_read( MFS_ALTERNATIVE_START, mfs_check, sizeof( mfs_check ) );
	if( strncmp( "MPFSMPFS", mfs_check, 8 ) == 0 ) { mfs_at = MFS_ALTERNATIVE_START; goto done; }

	printf( "MFS Not found at alternative address (%08x%08x).\n", mfs_check[0], mfs_check[1] );

done:
	printf( "MFS Found at: %08x\n", mfs_at );
	flashchip->chip_size = 0x00080000;
	ExitCritical();
}

extern SpiFlashChip * flashchip;

//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t ICACHE_FLASH_ATTR MFSOpenFile( const char * fname, struct MFSFileInfo * mfi )
{
	if( mfs_at == 0 )
	{
		FindMPFS();
	}
	if( mfs_at == 0 )
	{
		return -1;
	}

	EnterCritical();
	flashchip->chip_size = 0x01000000;
	uint32 ptr = mfs_at;
	struct MFSFileEntry e;
	while(1)
	{
		spi_flash_read( ptr, (uint32*)&e, sizeof( e ) );		
		ptr += sizeof(e);
		if( e.name[0] == 0xff || ets_strlen( e.name ) == 0 ) break;

		if( ets_strcmp( e.name, fname ) == 0 )
		{
			mfi->offset = e.start;
			mfi->filelen = e.len;
			flashchip->chip_size = 0x00080000;
			ExitCritical();
			return 0;
		}
	}
	flashchip->chip_size = 0x00080000;
	ExitCritical();
	return -1;
}

int32_t ICACHE_FLASH_ATTR MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi )
{
	 //returns # of bytes left tin file.
	if( !mfi->filelen )
	{
		return 0;
	}

	int toread = mfi->filelen;
	if( toread > MFS_SECTOR ) toread = MFS_SECTOR;

	EnterCritical();
	flashchip->chip_size = 0x01000000;
	spi_flash_read( mfs_at+mfi->offset, (uint32*)data, MFS_SECTOR );
	flashchip->chip_size = 0x00080000;
	ExitCritical();

	mfi->offset += toread;
	mfi->filelen -= toread;
	return mfi->filelen;
}



