#include <stdio.h>
#include <stdint.h>

#define NTSC_LINES 525

#define FT_STA_d 0
#define FT_STB_d 1
#define FT_B_d 2
#define FT_SRA_d 3
#define FT_SRB_d 4
#define FT_LIN_d 5
#define FT_CLOSE_d 6
#define FT_MAX_d 7

int main()
{

	uint8_t CbLookup[NTSC_LINES];
	int x;
	//Setup the callback table.
	for( x = 0; x < 3; x++ )
		CbLookup[x] = FT_STA_d;
	for( ; x < 6; x++ )
		CbLookup[x] = FT_STB_d;
	for( ; x < 9; x++ )
		CbLookup[x] = FT_STA_d;
	for( ; x < 24+6; x++ )
		CbLookup[x] = FT_B_d;
	for( ; x < 256-15; x++ )
		CbLookup[x] = FT_LIN_d;
	for( ; x < 263; x++ )
		CbLookup[x] = FT_B_d;

	//263rd frame, begin odd sync.
	for( ; x < 266; x++ )
		CbLookup[x] = FT_STA_d;

	CbLookup[x++] = FT_SRA_d;

	for( ; x < 269; x++ )
		CbLookup[x] = FT_STB_d;

	CbLookup[x++] = FT_SRB_d;

	for( ; x < 272; x++ )
		CbLookup[x] = FT_STA_d;
	for( ; x < 288+6; x++ )
		CbLookup[x] = FT_B_d;
	for( ; x < 519-15; x++ )
		CbLookup[x] = FT_LIN_d;
	for( ; x < NTSC_LINES-1; x++ )
		CbLookup[x] = FT_B_d;
	CbLookup[x] = FT_CLOSE_d;


	FILE * f = fopen( "CbTable.h", "w" );
	fprintf( f, "#ifndef _CBTABLE_H\n\
#define _CBTABLE_H\n\
\n\
#include <c_types.h>\n\
\n\
#define FT_STA_d 0\n\
#define FT_STB_d 1\n\
#define FT_B_d 2\n\
#define FT_SRA_d 3\n\
#define FT_SRB_d 4\n\
#define FT_LIN_d 5\n\
#define FT_CLOSE 6\n\
#define FT_MAX_d 7\n\
\n\
#define NTSC_LINES %d\n\
\n\
uint8_t CbLookup[%d];\n\
\n\
#endif\n\n", NTSC_LINES, (NTSC_LINES+1)/2 );
	fclose( f );

	f = fopen( "CbTable.c", "w" );
	fprintf( f, "#include \"CbTable.h\"\n\n" );
	fprintf( f, "uint8_t CbLookup[%d] = {", (NTSC_LINES+1)/2 );
	for( x = 0; x < 263; x++ )
	{
		if( (x & 0x0f) == 0 )
		{
			fprintf( f, "\n\t" );
		}
		fprintf( f, "0x%02x, ", CbLookup[x*2+0] | ( CbLookup[x*2+1]<<4 ) );
	}
	fprintf( f, "};\n" );

	return 0;
}

