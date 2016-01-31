//Unless what else is individually marked, all code in this file is
//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "mystuff.h"
#include <c_types.h>
#include <mem.h>

const char * enctypes[6] = { "open", "wep", "wpa", "wpa2", "wpa_wpa2", 0 };

char generic_print_buffer[384];
char generic_buffer[1500];
char * generic_ptr;

int32 my_atoi( const char * in )
{
	int positive = 1; //1 if negative.
	int hit = 0;
	int val = 0;
	while( *in && hit < 11 	)
	{
		if( *in == '-' )
		{
			if( positive == -1 ) return val*positive;
			positive = -1;
		} else if( *in >= '0' && *in <= '9' )
		{
			val *= 10;
			val += *in - '0';
			hit++;
		} else if (!hit && ( *in == ' ' || *in == '\t' ) )
		{
			//okay
		} else
		{
			//bad.
			return val*positive;
		}
		in++;
	}
	return val*positive;
}

void Uint32To10Str( char * out, uint32 dat )
{
	int tens = 1000000000;
	int val;
	int place = 0;

	while( tens > 1 )
	{
		if( dat/tens ) break;
		tens/=10;
	}

	while( tens )
	{
		val = dat/tens;
		dat -= val*tens;
		tens /= 10;
		out[place++] = val + '0';
	}

	out[place] = 0;
}

char tohex1( uint8_t i )
{
	i = i&0x0f;
	return (i<10)?('0'+i):('a'-10+i);
}

int8_t fromhex1( char c )
{
	if( c >= '0' && c <= '9' )
		return c - '0';
	else if( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	else if( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
	else
		return -1;
}

void  NixNewline( char * str )
{
	if( !str ) return;
	int sl = ets_strlen( str );
	if( sl > 1 && str[sl-1] == '\n' ) str[sl-1] = 0;
	if( sl > 2 && str[sl-2] == '\r' ) str[sl-2] = 0;
}



void ICACHE_FLASH_ATTR  EndTCPWrite( struct 	espconn * conn )
{
	if(generic_ptr!=generic_buffer)
	{
		int r = espconn_sent(conn,generic_buffer,generic_ptr-generic_buffer);
	}
}


void  PushString( const char * buffer )
{
	char c;
	while( c = *(buffer++) )
		PushByte( c );
}

void PushBlob( const uint8 * buffer, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		PushByte( buffer[i] );
}


int8_t TCPCanSend( struct espconn * conn, int size )
{
#ifdef SAFESEND
	return TCPDoneSend( conn );
#else
	struct espconn_packet infoarg;
	sint8 r = espconn_get_packet_info(conn, &infoarg);

	if( infoarg.snd_buf_size >= size && infoarg.snd_queuelen > 0 )
		return 1;
	else
		return 0;
#endif
}

int8_t ICACHE_FLASH_ATTR TCPDoneSend( struct espconn * conn )
{
	return conn->state == ESPCONN_CONNECT;
}

const char * ICACHE_FLASH_ATTR  my_strchr( const char * st, char c )
{
	while( *st && *st != c ) st++;
	if( !*st ) return 0;
	return st;
}

int ICACHE_FLASH_ATTR  ColonsToInts( const char * str, int32_t * vals, int max_quantity )
{
	int i;
	for( i = 0; i < max_quantity; i++ )
	{
		const char * colon = my_strchr( str, ':' );
		vals[i] = my_atoi( str );
		if( !colon ) break;
		str = colon+1;
	}
	return i+1;
}







//from http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
static const char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                      'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                      '4', '5', '6', '7', '8', '9', '+', '/'};

static const int mod_table[] = {0, 2, 1};

void ICACHE_FLASH_ATTR my_base64_encode(const unsigned char *data, size_t input_length, uint8_t * encoded_data )
{

	int i, j;
    int output_length = 4 * ((input_length + 2) / 3);

    if( !encoded_data ) return;
	if( !data ) { encoded_data[0] = '='; encoded_data[1] = 0; return; }

    for (i = 0, j = 0; i < input_length; ) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

	encoded_data[j] = 0;
}



void ICACHE_FLASH_ATTR SafeMD5Update( MD5_CTX * md5ctx, uint8_t*from, uint32_t size1 )
{
	char  __attribute__ ((aligned (32))) buffer[32];

	while( size1 > 32 )
	{
		ets_memcpy( buffer, from, 32 );
		MD5Update( md5ctx, buffer, 32 );
		size1-=32;
		from+=32;
	}
	ets_memcpy( buffer, from, 32 );
	MD5Update( md5ctx, buffer, size1 );
}

char * ICACHE_FLASH_ATTR strdup( const char * src )
{
	int len = ets_strlen( src );
	char * ret = (char*)os_malloc( len+1 );
	ets_memcpy( ret, src, len+1 );
	return ret;
}

char * ICACHE_FLASH_ATTR strdupcaselower( const char * src )
{
	int i;
	int len = ets_strlen( src );
	char * ret = (char*)os_malloc( len+1 );
	for( i = 0; i < len+1; i++ )
	{
		if( src[i] >= 'A' && src[i] <= 'Z' )
			ret[i] = src[i] - 'A' + 'a';
		else
			ret[i] = src[i];
	}
	return ret;
}

uint32_t ICACHE_FLASH_ATTR GetCurrentIP( )
{
	struct ip_info sta_ip;
	wifi_get_ip_info(STATION_IF, &sta_ip);
	if( sta_ip.ip.addr == 0 )
	{
		wifi_get_ip_info(SOFTAP_IF, &sta_ip);
	}
	if( sta_ip.ip.addr != 0 )
		return sta_ip.ip.addr;
	else
		return 0;
}

