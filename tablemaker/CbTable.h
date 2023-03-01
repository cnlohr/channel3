#ifndef _CBTABLE_H
#define _CBTABLE_H

#include <c_types.h>

#define FT_STA_d 0
#define FT_STB_d 1
#define FT_B_d 2
#define FT_SRA_d 3
#define FT_SRB_d 4
#define FT_LIN_d 5
#define FT_CLOSE 6
#define FT_MAX_d 7

uint8_t CbLookupPAL[313];
uint8_t CbLookupNTSC[263];

#ifdef PAL
#define VIDEO_LINES 625
#define CbLookup CbLookupPAL
#else
#define VIDEO_LINES 525
#define CbLookup CbLookupNTSC
#endif

#endif

