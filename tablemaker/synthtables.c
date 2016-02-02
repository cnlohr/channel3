/*
	Table synthesizer:  this tool creates bit tables for differen bit stream outputs.
	though the frequency we are synthesizing is above what we can actually create (f/2 and all)
	this will let you try and will alias just fine.  Just be sure to check its output against
	an FFT.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define PI2 (3.14159265359*2)

double eps = 0.01; //USed to help resolve and prevent issues at 0.
double NTSC_Frequency = 315.0/88.0;   //3.579545455 MHz
double MODULATION_Frequency = 61.25;
double BIT_Frequency = 80.0;
int samples = 1408;//+2; //+2 if you want to see   WARNING: This MUST be divisible by 16!

//ChromaValue, LumaValue = 0...??? 
//, ChromaShift, Luma are all 0..1
//Set boundary to a value to normalize Luma + Chroma.  Setting this to 0 will maximize both.
//Note that if ChromaValue == LumaValue, Chroma will not effect the signal.
//
//For all use of this function, I highly recommend checking the output at 160MHz (doubling)
//in an FFT, like http://sooeet.com/math/online-fft-calculator.php
//
//WARNING: These values are sort of inverted, as abscence of signal indicates value!
void WriteSignal( double LumaValue, double ChromaValue, double Boundary, double ChromaShift, int debug_output, uint16_t * raw_output )
{
	double t = 0;
	int i;
	int bitplace = 0;
	int byteplace = 0;

	for( i = 0; i < samples; i++ )
	{
		double ChromaV = sin( (NTSC_Frequency * t ) * PI2 / 1000.0 + eps  + ChromaShift * PI2  );
		double ModV = sin( ( MODULATION_Frequency * t ) * PI2 / 1000.0 + eps );
		double Signal = ModV * (ChromaV*(ChromaValue)+LumaValue) + Boundary;

		if( Signal > 0 )
			raw_output[byteplace] |= 1<<(15-bitplace);
		bitplace++;
		if( bitplace == 16 ) { bitplace = 0; byteplace++; }
		
		if( debug_output )
		{
			fprintf( stderr, "%d\n", ((Signal)>0)?1:-1 );
			fprintf( stderr, "%d\n", ((Signal)>0)?1:-1 );
		}
		t += 1000.0 / BIT_Frequency;
	}
}

int main()
{
	#define TABLESIZE 16

	int stride = samples/16;
	uint16_t databuffer[stride*TABLESIZE];
	memset( databuffer, 0, sizeof( databuffer ) );
	int i;
	double t = 0;	//In ns.
	int debug = 0;

	WriteSignal( 1.0, 0.0, 0.6, 0.0, debug, &databuffer[stride*0] ); //Black.  //-18.1 db
	WriteSignal( 1.0, 0.0, 1.0, 0.0, debug, &databuffer[stride*1] ); //White.  //-24.6 db
	WriteSignal( 1.0, 1.2, 0.8, 0.0, debug, &databuffer[stride*2] ); //Color.  //-24.6 db
	WriteSignal( 1.0, 1.1, 0.8, 0.5, debug, &databuffer[stride*3] ); 
	WriteSignal( 1.0, 1.0, 0.8, 0.4, debug, &databuffer[stride*4] ); 
	WriteSignal( 1.0, 0.9, 0.8, 0.8, debug, &databuffer[stride*5] ); 
	WriteSignal( 1.0, 0.8, 0.8, 0.0, debug, &databuffer[stride*6] ); 
	WriteSignal( 0.5, 1.0, 0.8, 0.0, debug, &databuffer[stride*7] ); 
	WriteSignal( 1.0, 0.0, 0.8, 0.0, debug, &databuffer[stride*8] ); 
	WriteSignal( 1.0, 0.0, 0.9, 0.0, debug, &databuffer[stride*9] ); 
	WriteSignal( 1.0, 0.0, 0.9, 0.0, debug, &databuffer[stride*10] );
	WriteSignal( 1.0, 0.0, 0.9, 0.0, debug, &databuffer[stride*11] ); //White.  //-24.6 db
	WriteSignal( 1.0, 0.0, 0.9, 0.0, debug, &databuffer[stride*12] ); //White.  //-24.6 db
	WriteSignal( 1.0, 0.0, 0.9, 0.0, debug, &databuffer[stride*13] ); //White.  //-24.6 db
	WriteSignal( 0.8, 0.1, 0.6, 0.0, debug, &databuffer[stride*14] ); //Chroma.
	WriteSignal( 1.0, 0.0, 0.0, 0.0, debug, &databuffer[stride*15] ); //Sync Tip.  //-16.3 db

	FILE * f = fopen( "broadcast_tables.c", "w" );
	fprintf( f, "#include \"broadcast_tables.h\"\n\n" );
	fprintf( f, "uint16_t premodulated_table[%d] = {", stride * TABLESIZE );
	for( i = 0; i < TABLESIZE*stride; i++ )
	{
		int imod = i % TABLESIZE;
		int idiv = i / TABLESIZE;
		idiv ^= 1; //Invert rows... So we flip our bit orders.  This looks weird, but it fixes our order-of-text.
		uint16_t val = databuffer[imod * stride + idiv];
		if( imod == 0 ) { fprintf( f, "\n\t" ); }
		fprintf( f, "0x%02x, ", (val) );
	}
	fprintf( f, "\n};\n" );
	fclose( f );

	f = fopen( "broadcast_tables.h", "w" );
	fprintf( f, "#include <c_types.h>\n\n" );
	fprintf( f, "#define PREMOD_ENTRIES %d\n", stride );
	fprintf( f, "#define PREMOD_SIZE %d\n", TABLESIZE );
	fprintf( f, "#define SYNC_LEVEL 15\n" );
	fprintf( f, "#define COLORBURST_LEVEL 14\n" );
	fprintf( f, "#define BLACK_LEVEL 0\n" );
	fprintf( f, "extern uint16_t premodulated_table[%d];\n\n", stride * TABLESIZE );
	fclose( f );
	
	return 0;
}

