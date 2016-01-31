//Copyright 2016 <>< Charles Lohr, See LICENSE file.

#ifndef _NTSC_BROADCAST_TEST
#define _NTSC_BROADCAST_TEST

//Stuff that should be for the header:

#include <c_types.h>


//These are just initial levels.

//Initial guess, manually found
#if 0
#define SYNC_LEVEL_d        0b10010000100000000010001000010000
#define WHITE_LEVEL_d       0b10010010010010010010010010010010 //Why does this work well?  (11 repeats)
#define BLACK_LEVEL_d       0b10000000100000000010000000010000
#define BTW_LEVEL_d         0b10000000100000000010010010010010 //Why does this work well?  (11 repeats)
#define WTB_LEVEL_d         0b10010010010010010010000000010000 //Why does this work well?  (11 repeats)
#else
#define SYNC_LEVEL_d        0x98c63333
#define WHITE_LEVEL_d       0xffffffff //Why does this work well?  (11 repeats)
#define BTW_LEVEL_d         0x80807fff
#define WTB_LEVEL_d         0xffff2010 //Why does this work well?  (11 repeats)
#define BLACK_LEVEL_d       0x88862210 //Why does this work well?  (11 repeats)
#endif



#define SYNC_LEVEL RFmaps[4]
#define WHITE_LEVEL RFmaps[3]
#define BTW_LEVEL RFmaps[2]
#define WTB_LEVEL RFmaps[1]
#define BLACK_LEVEL RFmaps[0]

extern uint32_t RFmaps[5];



#define FBW 256
#define FBH 240

extern int gframe;
extern uint8_t framebuffer[((FBW/8)*(FBH))*2]; //prevent overscan a bit.


void ICACHE_FLASH_ATTR testi2s_init();
void ICACHE_FLASH_ATTR redo_maps();

#endif

