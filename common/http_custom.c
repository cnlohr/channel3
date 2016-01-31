//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "http.h"
#include "mystuff.h"
#include "commonservices.h"

static ICACHE_FLASH_ATTR void huge()
{
	uint8_t i = 0;

	START_PACK;
	do
	{
		PushByte( 0 );
		PushByte( i );
	} while( ++i ); //Tricky:  this will roll-over to 0, and thus only execute 256 times.

	EndTCPWrite( curhttp->socket );
}


static ICACHE_FLASH_ATTR void echo()
{
	char mydat[128];
	int len = URLDecode( mydat, 128, curhttp->pathbuffer+8 );

	START_PACK;
	PushBlob( mydat, len );
	EndTCPWrite( curhttp->socket );

	curhttp->state = HTTP_WAIT_CLOSE;
}

static ICACHE_FLASH_ATTR void issue()
{
	uint8_t  __attribute__ ((aligned (32))) buf[1300];
	int len = URLDecode( buf, 1300, curhttp->pathbuffer+9 );

	int r = issue_command(buf, 1300, buf, len );
	if( r > 0 )
	{
		START_PACK;
		PushBlob( buf, r );
		EndTCPWrite( curhttp->socket );
	}
	curhttp->state = HTTP_WAIT_CLOSE;
}


void ICACHE_FLASH_ATTR HTTPCustomStart( )
{
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/huge", 7 ) == 0 )
	{
		curhttp->rcb = (void(*)())&huge;
		curhttp->bytesleft = 0xffffffff;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/echo?", 8 ) == 0 )
	{
		curhttp->rcb = (void(*)())&echo;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/issue?", 9 ) == 0 )
	{
		curhttp->rcb = (void(*)())&issue;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	{
		curhttp->rcb = 0;
		curhttp->bytesleft = 0;
	}
	curhttp->isfirst = 1;
	HTTPHandleInternalCallback();
}



void ICACHE_FLASH_ATTR HTTPCustomCallback( )
{
	if( curhttp->rcb )
		((void(*)())curhttp->rcb)();
	else
		curhttp->isdone = 1;
}




static void ICACHE_FLASH_ATTR WSEchoData(  int len )
{
	char cbo[len];
	int i;
	for( i = 0; i < len; i++ )
	{
		cbo[i] = WSPOPMASK();
	}
	WebSocketSend( cbo, len );
}


static void ICACHE_FLASH_ATTR WSEvalData( int len )
{
	char cbo[128];
	int l = ets_sprintf( cbo, "output.innerHTML = %d; doSend('x' );", curhttp->bytessofar++ );

	WebSocketSend( cbo, l );
}


static void ICACHE_FLASH_ATTR WSCommandData(  int len )
{
	uint8_t  __attribute__ ((aligned (32))) buf[1300];
	int i;

	for( i = 0; i < len; i++ )
	{
		buf[i] = WSPOPMASK();
	}

	i = issue_command(buf, 1300, buf, len );

	if( i < 0 ) i = 0;

	WebSocketSend( buf, i );
}


//	output.innerHTML = msg++ + " " + lasthz;
//	doSend('x' );



void ICACHE_FLASH_ATTR NewWebSocket()
{
	if( strcmp( (const char*)curhttp->pathbuffer, "/d/ws/echo" ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSEchoData;
	}
	else if( strcmp( (const char*)curhttp->pathbuffer, "/d/ws/evaltest" ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSEvalData;
	}
	else if( strncmp( (const char*)curhttp->pathbuffer, "/d/ws/issue", 11 ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSCommandData;
	}
	else
	{
		curhttp->is404 = 1;
	}
}




void ICACHE_FLASH_ATTR WebSocketTick()
{
	if( curhttp->rcb )
	{
		((void(*)())curhttp->rcb)();
	}
}

void ICACHE_FLASH_ATTR WebSocketData( int len )
{	
	if( curhttp->rcbDat )
	{
		((void(*)( int ))curhttp->rcbDat)(  len ); 
	}
}
