//Copyright 2016 <>< Charles Lohr, See LICENSE file.

#ifndef _NTSC_BROADCAST_TEST
#define _NTSC_BROADCAST_TEST

/*
	This is the NTSC Broadcast code.  To set it up, call testi2s_init.
	This will set up the DMA engine and all the chains for outputting 
	broadcast.

	This is tightly based off of SpriteTM's ESP8266 MP3 Decoder.

	If you change the RF Maps, please call redo_maps, this will make
	the system update all the non-frame data to use the right bit patterns.
*/


//Stuff that should be for the header:

#include <c_types.h>

//Framebuffer width/height
#define FBW 232 //Must be divisible by 8.  These are actually "double-pixels" used for double-resolution monochrome width.
#define FBW2 (FBW/2) //Actual width in true pixels.
#define FBH 220

#define DMABUFFERDEPTH 3

extern int gframe; //Current frame #
extern uint16_t framebuffer[((FBW2/4)*(FBH))*2]; // /4 = 4 pixels per word (*2 = double buffer)
extern uint32_t last_internal_frametime;
extern int8_t jam_color; //Used to test frequency out


void ICACHE_FLASH_ATTR testi2s_init();

#endif

