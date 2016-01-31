//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "flash_rewriter.h"
#include <c_types.h>
#include <esp8266_rom.h>
#include <stdio.h>

#define SRCSIZE 4096
#define BLKSIZE 65536

static const char * key = "";
static int keylen = 0;


static int MyRewriteFlash( char * command, int commandlen )
{
	MD5_CTX md5ctx;
	char  __attribute__ ((aligned (32))) buffer[512];
	char * colons[8];
	int i, ipl = 0;
	int p;
	//[from_address]\t[to_address]\t[size]\t[MD5(key+data)]\t[from_address]\t[to_address]\t[size]\t[MD5(key+data)]
	command[commandlen] = 0;

	flashchip->chip_size = 0x01000000;

	ets_wdt_disable();

	colons[ipl++] = &command[0];
	for( i = 0; i < commandlen; i++ )
	{
		if( command[i] == 0 ) break;
		if( command[i] == '\t' )
		{
			if( ipl >= 8 ) break;
			command[i] = 0;
			colons[ipl++] = &command[i+1];
		}
	}
	if( ipl != 8 )
	{
		return 1;
	}
	uint32_t from1 = my_atoi( colons[0] );
	uint32_t to1 =   my_atoi( colons[1] );
	int32_t  size1 = my_atoi( colons[2] );
	char *   md51  = colons[3];
	char     md5h1raw[48];
	char     md5h1[48];
	uint32_t from2 = my_atoi( colons[4] );
	uint32_t to2 =   my_atoi( colons[5] );
	int32_t  size2 = my_atoi( colons[6] );
	char *   md52  = colons[7];
	char     md5h2raw[48];
	char     md5h2[48];

	if( from1 == 0 || from2 == 0 || size1 == 0 )
	{
		return 2;
	}

	if( ( from1 & 0xfff ) || ( from2 & 0xfff ) || ( to1 & 0xfff ) || ( to2 & 0xfff ) )
	{
		return 3;
	}


	///////////////////////////////
	char st[400];
/*
	ets_sprintf( st, "!!%08x", (((uint32_t*)(0x40200000 + from1))) );
	uart0_sendStr( st );
	
	for( i = 0; i < 200; i++ )
	{
		ets_sprintf( st, "%08x", (uint32_t)(((uint32_t*)(0x40200000 + from1))[i]) );
		uart0_sendStr( st );
	}
	uart0_sendStr( "+\n" );

/*


	uint32_t __attribute__ ((aligned (32)))  readr = 0xAAAAAAAA;
	SPIRead( from1, &readr, 4 );
	ets_sprintf( st, ":%08x\n", readr );
	uart0_sendStr( st );
	readr = 0x12345678;

	SPIRead( from1+4, &readr, 4 );
	ets_sprintf( st, ":%08x\n", readr );
	uart0_sendStr( st );
	uart0_sendStr( "\n" );

	ets_sprintf( st, "TT: %08x ADDY: %08x\n", from1, &readr );
	uart0_sendStr( st );



	
	readr = 0xAAAAAAAA;
	spi_flash_read( from1, &readr, 4 );
	ets_sprintf( st, "+%08x\n", readr );
	uart0_sendStr( st );

	readr = 0xbbbbbbbb;
	spi_flash_read( from1+4, &readr, 4 );
	ets_sprintf( st, "+%08x\n", readr );
	uart0_sendStr( st );

*/

/*
	MD5Init( &md5ctx );
	MD5Update( &md5ctx, (uint8_t*)0x40200000 + from1, 4 );
	MD5Final( md5h1raw, &md5ctx );
	for( i = 0; i < 16; i++ )
	{
		ets_sprintf( md5h1+i*2, "%02x", md5h1raw[i] );
	}
	uart0_sendStr( "Hash 1:" );
	uart0_sendStr( md5h1 );
	uart0_sendStr( "!\n" );
*/

	//////////////////////////////

	ets_sprintf( st, "Computing Hash 1: %08x size %d\n", from1, size1 );
	uart0_sendStr( st );

	MD5Init( &md5ctx );
	if( keylen )
		MD5Update( &md5ctx, key, keylen );
//	MD5Update( &md5ctx, (uint8_t*)0x40200000 + from1, size1 );
	SafeMD5Update( &md5ctx, (uint8_t*)0x40200000 + from1, size1 );
	MD5Final( md5h1raw, &md5ctx );
	for( i = 0; i < 16; i++ )
	{
		ets_sprintf( md5h1+i*2, "%02x", md5h1raw[i] );
	}

	uart0_sendStr( "Hash 1:" );
	uart0_sendStr( md5h1 );
	uart0_sendStr( "\n" );

	for( i = 0; i < 32; i++ )
	{
		if( md5h1[i] != md51[i] )
		{
			//printf( "%s != %s", md5h1, md51 );
			uart0_sendStr( "File 1 MD5 mismatch\nActual:" );
			uart0_sendStr( md5h1 );
			uart0_sendStr( "\nExpected:" );
			uart0_sendStr( md51 );
			uart0_sendStr( "\n" );
			return 4;
		}
	}

	ets_sprintf( st, "Computing Hash 2: %08x size %d\n", from2, size2 );
	uart0_sendStr( st );

	MD5Init( &md5ctx );
	if( keylen )
		MD5Update( &md5ctx, key, keylen );
	SafeMD5Update( &md5ctx, (uint8_t*)0x40200000 + from2, size2 );
	MD5Final( md5h2raw, &md5ctx );
	for( i = 0; i < 16; i++ )
	{
		ets_sprintf( md5h2+i*2, "%02x", md5h2raw[i] );
	}

	uart0_sendStr( "Hash 2:" );
	uart0_sendStr( md5h2 );
	uart0_sendStr( "\n" );

/*	for( i = 0; i <= size2/4; i++ )
	{
		uint32_t j = ((uint32_t*)(0x40200000 + from2))[i];
		ets_sprintf( st, "%02x%02x%02x%02x\n", (uint8_t)(j>>0), (uint8_t)(j>>8), (uint8_t)(j>>16), (uint8_t)(j>>24) );
		uart0_sendStr( st );
	}*/

	for( i = 0; i < 32; i++ )
	{
		if( md5h2[i] != md52[i] )
		{
			uart0_sendStr( "File 2 MD5 mismatch\nActual:" );
			uart0_sendStr( md5h2 );
			uart0_sendStr( "\nExpected:" );
			uart0_sendStr( md52 );
			uart0_sendStr( "\n" );
			return 5;
		}
	}

	//Need to round sizes up.
	size1 = ((size1-1)&(~0xfff))+1;
	size2 = ((size2-1)&(~0xfff))+1;

	ets_sprintf( st, "Copy 1: %08x to %08x, size %d\n", from1, to1, size1 );
	uart0_sendStr( st );
	ets_sprintf( st, "Copy 2: %08x to %08x, size %d\n", from2, to2, size2 );
	uart0_sendStr( st );

	//Everything checked out... Need to move the flashes.

	ets_delay_us( 1000 );

	//Disable all interrupts.
	ets_intr_lock();

	uart_tx_one_char( 'A' );

	int j;
	ipl = (size1/BLKSIZE)+1;
	p = to1/BLKSIZE;
	for( i = 0; i < ipl; i++ )
	{
		SPIEraseBlock( p++ );

		for( j = 0; j < BLKSIZE/SRCSIZE; j++ )
		{
			SPIWrite( to1, (uint32_t*)(0x40200000 + from1), SRCSIZE );
			to1 += SRCSIZE;
			from1 += SRCSIZE;
		}
	}

	uart_tx_one_char( 'B' );

	ipl = (size2/BLKSIZE)+1;
	p = to2/BLKSIZE;
	for( i = 0; i < ipl; i++ )
	{
		SPIEraseBlock( p++ );
		for( j = 0; j < BLKSIZE/SRCSIZE; j++ )
		{
			SPIWrite( to2, (uint32_t*)(0x40200000 + from2), SRCSIZE );
			to2 += SRCSIZE;
			from2 += SRCSIZE;
		}
	}

	uart_tx_one_char( 'C' );


	void(*rebootme)() = (void(*)())0x40000080;
	rebootme();


//	 system_upgrade_reboot();
//	software_reset(); //Doesn't seem to boot back in.


	//Destinations are erased.  Copy over the other part.
//	for( i = 0; i < ; i++ )

	/*Things I know:
		flashchip->chip_size = 0x01000000;
		SPIEraseSector( 1000000/4096 );
        SPIWrite( 1000000, &t, 4 ); <<This looks right.
		SPIRead( 1000000, &t, 4 ); <<Will read if we just wrote, but not from cache.

		uint32_t * v = (uint32_t*)(0x40200000 + 1000000); //This will read, but from cache.

//Looks like you can copy this way...
//		SPIWrite( 1000004, 0x40200000 + 1000000, 4 );

  */

/*	
	MD5Init( &c );
	MD5Update( &c, "apple", 5 );
	MD5Final( hash, &c );*/


	//Once we hit this stage we cannot do too much, otherwise we'll cause major crashing.
	

}


int (*GlobalRewriteFlash)( char * command, int commandlen ) = MyRewriteFlash;
