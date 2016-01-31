//Unless what else is individually marked, all code in this file is
//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#ifndef _MYSTUFF_H
#define _MYSTUFF_H


#include <mem.h>
#include <c_types.h>
#include <user_interface.h>
#include <ets_sys.h>
#include <espconn.h>
#include <esp8266_rom.h>


//XXX WARNING As of 1.3.0, "cansend" doesn't work.
//the SDK seems to misbehave when trying to send without a full
//response packet.
#define SAFESEND


#define HTONS(x) ((((uint16_t)(x))>>8)|(((x)&0xff)<<8))

#define IP4_to_uint32(x) (((uint32_t)x[3]<<24)|((uint32_t)x[2]<<16)|((uint32_t)x[1]<<8)|x[0])
#define uint32_to_IP4(x,y) {y[0] = (uint8_t)(x); y[1] = (uint8_t)((x)>>8); y[2] = (uint8_t)((x)>>16); y[3] = (uint8_t)((x)>>24);}

extern char generic_print_buffer[384];

extern const char * enctypes[6];// = { "open", "wep", "wpa", "wpa2", "wpa_wpa2", 0 };

#define printf( ... ) { ets_sprintf( generic_print_buffer, __VA_ARGS__ );  uart0_sendStr( generic_print_buffer ); }

char tohex1( uint8_t i );
int8_t fromhex1( char c ); //returns -1 if not hex char.

int32  my_atoi( const char * in );
void  Uint32To10Str( char * out, uint32 dat );

void  NixNewline( char * str ); //If there's a newline at the end of this string, make it null.

//For holding TX packet buffers
extern char generic_buffer[1500];
extern char * generic_ptr;
int8_t ICACHE_FLASH_ATTR  TCPCanSend( struct espconn * conn, int size );
int8_t ICACHE_FLASH_ATTR  TCPDoneSend( struct espconn * conn );
void  ICACHE_FLASH_ATTR  EndTCPWrite( struct espconn * conn );


#define PushByte( c ) { *(generic_ptr++) = c; }

void PushString( const char * buffer );
void PushBlob( const uint8 * buffer, int len );
#define START_PACK {generic_ptr=generic_buffer;}
#define PACK_LENGTH (generic_ptr-&generic_buffer[0]}

int ICACHE_FLASH_ATTR  ColonsToInts( const char * str, int32_t * vals, int max_quantity );

//As much as it pains me, we shouldn't be using the esp8266's base64_encode() function
//as it does stuff with dynamic memory.
void ICACHE_FLASH_ATTR my_base64_encode(const unsigned char *data, size_t input_length, uint8_t * encoded_data );


void ICACHE_FLASH_ATTR SafeMD5Update( MD5_CTX * md5ctx, uint8_t*from, uint32_t size1 );

char * ICACHE_FLASH_ATTR strdup( const char * src );
char * ICACHE_FLASH_ATTR strdupcaselower( const char * src );


uint32_t ICACHE_FLASH_ATTR GetCurrentIP( );

#endif
