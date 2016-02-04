//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include <ntsc_broadcast.h>
#include <mystuff.h>

extern uint8_t showstate;
extern uint8_t showallowadvance;
extern int8_t jam_color;

void ICACHE_FLASH_ATTR ReinitSettings()
{
}

void ICACHE_FLASH_ATTR SettingsLoaded()
{
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

	case 'o': case 'O':  //co xxyy   (where xx = current show state, yy = allow advancing)
	{
		//Show control
		char * bp = &buffer[3];
		uint8_t rh = 0;
		rh = fromhex1( *(bp++) );
		showstate = (rh << 4) | fromhex1( *(bp++) );
		rh = fromhex1( *(bp++) );
		showallowadvance = (rh << 4) | fromhex1( *(bp++) );
		rh = fromhex1( *(bp++) );
		jam_color = (rh << 4) | fromhex1( *(bp++) );
		break;
	}

	case 'v': case 'V': //cv xxyynnnnnnnnnnnnn (where xx is the color #, yy is the offset in that color number, nnnnnnnn is the color data)
		
		break;
	}
	return -1;
}
