//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include <ntsc_broadcast.h>
#include <mystuff.h>

//Defaults
const uint32_t tRFmaps[] = {
		BLACK_LEVEL_d, WTB_LEVEL_d, BTW_LEVEL_d, WHITE_LEVEL_d, SYNC_LEVEL_d };

void ICACHE_FLASH_ATTR ReinitSettings()
{
	printf( "Restoring to factory %d\n", sizeof( tRFmaps ) );
	//Load from factory default.
	ets_memcpy( SETTINGS.UserData, tRFmaps, sizeof( tRFmaps ) );
	ets_memcpy( RFmaps, tRFmaps, sizeof( tRFmaps ) );
	int i;
	for( i = 0; i < 5; i++ )
	{
		printf( " %08x\n", tRFmaps[i] );
	}
}

void ICACHE_FLASH_ATTR SettingsLoaded()
{
	printf( "Loading to from \"settings\"\n" );
	ets_memcpy( RFmaps, SETTINGS.UserData, sizeof( RFmaps ) );
}


int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )
	{
	case 'C': case 'c': //Custom command test
	{
		buffend += ets_sprintf( buffend, "CC" );
		return buffend-buffer;
	}

	case 'g': case 'G': //get NTSC bits
	{
		int i, it = 0;
		buffend += ets_sprintf( buffend, "CG:%08x:%08x:%08x:%08x:%08x\n", RFmaps[0], RFmaps[1], RFmaps[2], RFmaps[3], RFmaps[4] );
		return buffend-buffer;
	}

	case 'd': case 'D': //get default NTSC bits
	{
		int i, it = 0;
		buffend += ets_sprintf( buffend, "CG:%08x:%08x:%08x:%08x:%08x\n", tRFmaps[0], tRFmaps[1], tRFmaps[2], tRFmaps[3], tRFmaps[4] );
		return buffend-buffer;
	}


	case 's': case 'S': //set NTSC bits
	{
		printf( "Setting NTSC Bits: %s\n", buffer+3 );
		int i;
		char * bp = &buffer[3];
		for( i = 0; i < 5; i++ )
		{
			uint32_t rh = 0;
			int k;
			for( k = 0; k < 8; k++ )
			{
				rh = (rh << 4) | fromhex1( *(bp++) );
			}
			RFmaps[i] = rh;
		}
		buffend += ets_sprintf( buffend, "CS:%08x:%08x:%08x:%08x:%08x\n", RFmaps[0], RFmaps[1], RFmaps[2], RFmaps[3], RFmaps[4] );
		ets_memcpy( SETTINGS.UserData, RFmaps, sizeof( RFmaps ) );
		redo_maps();
		return buffend-buffer;
	}


	}
	return -1;
}
