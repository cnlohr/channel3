#include <stdio.h>
#include <math.h>
#include <stdint.h>

void OutputTable( int sampsx32, double * dfreqs, double * dvols, int nrsigs, double cutoff )
{
	int i, j;
	double pl[nrsigs];
	double adv[nrsigs];
	int sig;
	for( sig = 0; sig < nrsigs; sig++ )
	{
		pl[sig] = 0;
		adv[sig] = dfreqs[sig]*3.14159*2;
	}

	for( i = 0; i < sampsx32;i ++ )
	{
		uint32_t frame = 0;
		for( j = 0; j < 32; j++ )
		{
			double tf = 0;
			sig = (i) & 1;
			int bit = 0;
			for( sig = 0; sig < nrsigs; sig++ )
			{
				double thv = sin( pl[sig] ) * dvols[sig];
				tf += thv;
				pl[sig]+=adv[sig];
				bit |= (thv > cutoff)?1:0;
			}
			//bit = (tf > cutoff)?1:0;
			frame |= bit<<(31-j);
		}
		if( ( i & 7 ) == 0 ) printf( "\n\t" );
		printf( "0x%08xL, ", frame );
	}
}

int main()
{
	double lower_edge = 60;
	double base = lower_edge+1.25;
	double targ_freq_0 = lower_edge+5.75-0.01;
	double targ_freq_1 = lower_edge+5.75+0.01;
	double mhz = 80;//1040/6.0; 

	int sampsx32 = 1000;
	int samps = sampsx32*32;

	printf( "#ifndef xmit_tables_h_\n" );
	printf( "#define xmit_tables_h_\n" );

	printf( "#include <stdint.h>\n" );
	printf( "#define I2SDATASIZE %d\n", sampsx32 );

	double dfs[2];
	double amps[2] = {.53, 1.0 };

	printf( "uint32_t table0[%d] = {", sampsx32 );
	dfs[0] = targ_freq_0 / mhz ;
	dfs[1] = base/mhz;
	amps[1] = 1;
	OutputTable( sampsx32, dfs, amps, 2, .5 );
	printf( "};\n" );

	printf( "uint32_t table1[%d] = {", sampsx32 );
	dfs[0] = targ_freq_1 / mhz;
	dfs[1] = base/mhz;
	amps[1] = 1.0;
	OutputTable( sampsx32, dfs, amps, 2, .5 );
	printf( "};\n" );

	printf( "#endif\n" );
}

