//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include "uart.h"
#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "video_broadcast.h"
#include "commonservices.h"
#include <mdns.h>
#include "3d.h"

#define PORT 7777

#define procTaskPrio        0
#define procTaskQueueLen    1

static os_timer_t some_timer;
static struct espconn *pUdpServer;

//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];

void ICACHE_FLASH_ATTR SetupMatrix( )
{
	int16_t lmatrix[16];
	tdIdentity( ProjectionMatrix );
	tdIdentity( ModelviewMatrix );

	Perspective( 600, 250, 50, 8192, ProjectionMatrix );
}

void user_pre_init(void)
{
	//You must load the partition table so the NONOS SDK can find stuff.
	LoadDefaultPartitionMap();
}
 

//0 is the normal flow
//11 is the multi-panel scene.
#define INITIAL_SHOW_STATE 0

extern int gframe;
char lastct[256];
uint8_t showstate = INITIAL_SHOW_STATE;
uint8_t showallowadvance = 1;
int framessostate = 0;
int showtemp = 0;

int16_t Height( int x, int y, int l )
{
	return tdCOS( (x*x + y*y) + l );
}

void ICACHE_FLASH_ATTR DrawFrame(  )
{
	char * ctx = &lastct[0];
	int x = 0;
	int y = 0;
	int i;
	int sqsiz = gframe&0x7f;
	int newstate = showstate;
	CNFGPenX = 14;
	CNFGPenY = 20;
	ets_memset( frontframe, 0x00, ((FBW/4)*FBH) );
	int16_t rt[16];
	tdIdentity( ModelviewMatrix );
	tdIdentity( ProjectionMatrix );
	CNFGColor( 17 );

/*

*/

	switch( showstate )
	{
	case 11:  // State that's not in the normal set.  Just displays boxes.
	{
		for( i = 0; i < 16; i++ )
		{
			int x = i%4;
			int y = i/4;
			x *= (FBW/4);
			y *= (FBH/4);
			CNFGColor( i );
			CNFGTackRectangle( x, y, x+(FBW/4)-1, y+(FBH/4)-1);
		}
		break;
	}
	case 10:
	{
		int i;
		for( i = 0; i < 16; i++ )
		{
			CNFGPenX = 14;
			CNFGPenY = (i+1) * 12;
			CNFGColor( i );
			CNFGDrawText( "Hello", 3 );
			CNFGTackRectangle( 120, (i+1)*12, 180, (i+1)*12+12);
		}

		SetupMatrix();
		tdRotateEA( ProjectionMatrix, -20, 0, 0 );
		tdRotateEA( ModelviewMatrix, framessostate, 0, 0 );

		for( y = 3; y >= 0; y-- )
		for( x = 0; x < 4; x++ )
		{
			CNFGColor( x+y*4 );
			ModelviewMatrix[11] = 1000 + tdSIN( (x + y)*40 + framessostate*2 );
			ModelviewMatrix[3] = 600*x-850;
			ModelviewMatrix[7] = 600*y+800 - 850;
			DrawGeoSphere();
		}


		if( framessostate > 500 ) newstate = 9;
	}
		break;
	case 9:
	{
		const char * s = "Direct modulation.\nDMA through the I2S Bus!\nTry it yourself!\n\nhttp://github.com/cnlohr/\nchannel3\n";

		i = ets_strlen( s );
		if( i > framessostate ) i = framessostate;
		ets_memcpy( lastct, s, i );
		lastct[i] = 0;
		CNFGDrawText( lastct, 3 );
		if( framessostate > 500 ) newstate = 0;
		break;
	}
	case 8:
	{
		int16_t lmatrix[16];
		CNFGDrawText( "Dynamic 3D Meshes", 3 );
		SetupMatrix();
		tdRotateEA( ProjectionMatrix, -20, 0, 0 );
		tdRotateEA( ModelviewMatrix, 0, 0, framessostate );

		for( y = -18; y < 18; y++ )
		for( x = -18; x < 18; x++ )
		{
			int o = -framessostate*2;
			int t = Height( x, y, o )* 2 + 2000;
			CNFGColor( ((t/100)%15) + 1 );
			int nx = Height( x+1, y, o ) *2 + 2000;
			int ny = Height( x, y+1, o ) * 2 + 2000;
			//printf( "%d\n", t );
			int16_t p0[3] = { x*140, y*140, t };
			int16_t p1[3] = { (x+1)*140, y*140, nx };
			int16_t p2[3] = { x*140, (y+1)*140, ny };
			Draw3DSegment( p0, p1 );
			Draw3DSegment( p0, p2 );
		}

		if( framessostate > 400 ) newstate = 10;
		break;
	}
	case 7:
	{
		int16_t lmatrix[16];
		CNFGDrawText( "Matrix-based 3D engine.", 3 );
		SetupMatrix();
		tdRotateEA( ProjectionMatrix, -20, 0, 0 );


		tdRotateEA( ModelviewMatrix, framessostate, 0, 0 );
		int sphereset = (framessostate / 120);
		if( sphereset > 2 ) sphereset = 2;
		if( framessostate > 400 )
		{
			newstate = 8;
		}
		for( y = -sphereset; y <= sphereset; y++ )
		for( x = -sphereset; x <= sphereset; x++ )
		{
			if( y == 2 ) continue;
			ModelviewMatrix[11] = 1000 + tdSIN( (x + y)*40 + framessostate*2 );
			ModelviewMatrix[3] = 500*x;
			ModelviewMatrix[7] = 500*y+800;
			DrawGeoSphere();
		}

		break;
	}
	case 6:
		CNFGDrawText( "Lines on double-buffered 232x220.", 2 );
		if( framessostate > 60 )
		{
			for( i = 0; i < 350; i++ )
			{
				CNFGColor( rand()%16 );
				CNFGTackSegment( rand()%FBW2, rand()%(FBH-30)+30, rand()%FBW2, rand()%(FBH-30)+30 );
			}
		}
		if( framessostate > 240 )
		{
			newstate = 7;
		}
		break;
	case 5:
		ets_memcpy( frontframe, (uint8_t*)(framessostate*(FBW/8)+0x3FFF8000), ((FBW/4)*FBH) );
		CNFGColor( 17 );
		CNFGTackRectangle( 70, 110, 180+200, 150 );		
		CNFGColor( 16 );
		if( framessostate > 160 ) newstate = 6;
	case 4:
		CNFGPenY += 14*7;
		CNFGPenX += 60;
		CNFGDrawText( "38x14 TEXT MODE", 2 );

		CNFGPenY += 14;
		CNFGPenX -= 5;
		CNFGDrawText( "...on 232x220 gfx", 2 );

		if( framessostate > 60 && showstate == 4 )
		{
			newstate = 5;
		}
		break;
	case 3:
		for( y = 0; y < 14; y++ )
		{
			for( x = 0; x < 38; x++ )
			{
				i = x + y + 1;
				if( i < framessostate && i > framessostate - 60 )
					lastct[x] = ( i!=10 && i!=9 )?i:' ';
				else
					lastct[x] = ' ';
			}
			if( y == 7 )
			{
				ets_memcpy( lastct + 10, "36x12 TEXT MODE", 15 );
			}
			lastct[x] = 0;
			CNFGDrawText( lastct, 2 );
			CNFGPenY += 14;
			if( framessostate > 120 ) newstate = 4;
		}
		break;
	case 2:
		ctx += ets_sprintf( ctx, "ESP8266 Features:\n 802.11 Stack\n Xtensa Core @80 or 160 MHz\n 64kB IRAM\n 96kB DRAM\n 16 GPIO\n\
 SPI\n UART\n PWM\n ADC\n I2S with DMA\n                                                   \n Analog Broadcast Television\n" );
		int il = ctx - lastct;
		if( framessostate/2 < il )
			lastct[framessostate/2] = 0;
		else 
			showtemp++;
		CNFGDrawText( lastct, 2 );
		if( showtemp == 60 ) newstate = 3;
		break;
	case 1:
		i = ets_strlen( lastct );
		lastct[i-framessostate] = 0;
		if( i-framessostate == 1 ) newstate = 2;
	case 0:
	{
		int stat = wifi_station_get_connect_status();

		CNFGDrawText( lastct, 2 );

		int rssi = wifi_station_get_rssi();
		uint16 sysadc = system_adc_read();
		ctx += ets_sprintf( ctx, "Channel 3 Broadcasting.\nframe: %d\nrssi: %d\nadc:  %d\nsstat:%d\n", gframe, rssi,sysadc, stat );
		struct station_config wcfg;
		struct ip_info ipi;
		wifi_get_ip_info(0, &ipi);
		if( ipi.ip.addr || stat == 255 )
		{
			ctx += ets_sprintf( ctx, "IP: %d.%d.%d.%d\n", (ipi.ip.addr>>0)&0xff,(ipi.ip.addr>>8)&0xff,(ipi.ip.addr>>16)&0xff,(ipi.ip.addr>>24)&0xff );
			ctx += ets_sprintf( ctx, "NM: %d.%d.%d.%d\n", (ipi.netmask.addr>>0)&0xff,(ipi.netmask.addr>>8)&0xff,(ipi.netmask.addr>>16)&0xff,(ipi.netmask.addr>>24)&0xff );
			ctx += ets_sprintf( ctx, "GW: %d.%d.%d.%d\nESP Online\n", (ipi.gw.addr>>0)&0xff,(ipi.gw.addr>>8)&0xff,(ipi.gw.addr>>16)&0xff,(ipi.gw.addr>>24)&0xff );
			showtemp++;
			if( showtemp == 30 ) newstate = 1;
		}
		break;
	}

	}

	if( showstate != newstate && showallowadvance )
	{
		showstate = newstate;
		framessostate = 0;
		showtemp = 0;
	}
	else
	{
		framessostate++;
	}

}

static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{
	static uint8_t lastframe = 0;
	uint8_t tbuffer = !(gframe&1);

	CSTick( 0 );

	if( lastframe != tbuffer )
	{
		//printf( "FT: %d - ", last_internal_frametime );
		uint32_t tft = system_get_time();
		frontframe = (uint8_t*)&framebuffer[((FBW2/4)*FBH)*tbuffer];
		DrawFrame( frontframe );
		//ets_memset( frontframe, 0xaa, ((FBW/4)*FBH) );
		lastframe = tbuffer;
		//printf( "%d\n", system_get_time() - tft );
	}

	system_os_post(procTaskPrio, 0, 0 );
}

//Timer event.
static void ICACHE_FLASH_ATTR myTimer(void *arg)
{
	CSTick( 1 );
}


//Called when new packet comes in.
static void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;

	uart0_sendStr("X");
/*
	ws2812_push( pusrdata+3, len-3 );

	len -= 3;
	if( len > sizeof(last_leds) + 3 )
	{
		len = sizeof(last_leds) + 3;
	}
	ets_memcpy( last_leds, pusrdata+3, len );
	last_led_count = len / 3;*/
}

void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart0_sendStr("\r\nesp8266 ws2812 driver\r\n");
//	int opm = wifi_get_opmode();
//	if( opm == 1 ) need_to_switch_opmode = 120;
//	wifi_set_opmode_current(2);
//Uncomment this to force a system restore.
//	system_restore();

#ifdef FORCE_SSID
#define SSID ""
#define PSWD ""
#endif

	//Override wifi.
#if FORCE_SSID
	{
		struct station_config stationConf;
		wifi_station_get_config(&stationConf);
		os_strcpy((char*)&stationConf.ssid, SSID );
		os_strcpy((char*)&stationConf.password, PSWD );
		stationConf.bssid_set = 0;
		wifi_station_set_config(&stationConf);
		wifi_set_opmode(1);
	}
#endif

	CSSettingsLoad( 0 );
	CSPreInit();

	//Override wifi.
#if FORCE_SSID
	{
		struct station_config stationConf;
		wifi_station_get_config(&stationConf);
		os_strcpy((char*)&stationConf.ssid, SSID );
		os_strcpy((char*)&stationConf.password, PSWD );
		stationConf.bssid_set = 0;
		wifi_station_set_config(&stationConf);
		wifi_set_opmode(1);
	}
#else
		wifi_set_opmode(2);
#endif


    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7777;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	CSInit(1);

	SetServiceName( "ws2812" );
	AddMDNSName( "cn8266" );
	AddMDNSName( "ws2812" );
	AddMDNSService( "_http._tcp", "An ESP8266 Webserver", 80 );
	AddMDNSService( "_ws2812._udp", "WS2812 Driver", 7777 );
	AddMDNSService( "_cn8266._udp", "ESP8266 Backend", 7878 );

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 100, 1);


	testi2s_init();

	system_update_cpu_freq( SYS_CPU_160MHZ );

	system_os_post(procTaskPrio, 0, 0 );
}


//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical()
{
}

void ExitCritical()
{
}


