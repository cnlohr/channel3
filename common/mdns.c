#include "mdns.h"
#include "commonservices.h"
#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"
#include "ip_addr.h"

#define MDNS_BRD 0xfb0000e0

static char * MDNSNames[MAX_MDNS_NAMES];
static struct espconn *pMDNSServer;
static uint8_t igmp_bound[4];
static char MyLocalName[MAX_MDNS_PATH+7];

static int nr_services = 0;
static char * MDNSServices[MAX_MDNS_SERVICES];
static char * MDNSServiceTexts[MAX_MDNS_SERVICES];
static uint16_t MDNSServicePorts[MAX_MDNS_SERVICES];

static char MDNSSearchName[MAX_MDNS_PATH];


void ICACHE_FLASH_ATTR AddMDNSService( const char * ServiceName, const char * Text, int port )
{
	int i;
	for( i = 0; i < MAX_MDNS_SERVICES; i++ )
	{
		if( !MDNSServices[i] )
		{
			MDNSServices[i] = strdupcaselower( ServiceName );
			MDNSServiceTexts[i] = strdup( Text );
			MDNSServicePorts[i] = port;
			nr_services++;
			break;
		}
	}
}

void ICACHE_FLASH_ATTR AddMDNSName( const char * ToDup )
{
	int i;
	for( i = 1; i < MAX_MDNS_NAMES; i++ )
	{
		if( !MDNSNames[i] )
		{
			if( i == 0 )
			{
				int sl = ets_strlen( ToDup );
				ets_memcpy( MyLocalName, ToDup, sl );
				ets_memcpy( MyLocalName + sl, ".local", 7 );
			}
			MDNSNames[i] = strdupcaselower( ToDup );
			break;
		}
	}
}

void ICACHE_FLASH_ATTR ClearMDNSNames()
{
	int i;
	for( i = 0; i < MAX_MDNS_NAMES; i++ )
	{
		if( MDNSNames[i] )
		{
			os_zfree( MDNSNames[i] );
			MDNSNames[i] = 0;
		}
	}
	for( i = 0; i < MAX_MDNS_SERVICES; i++ )
	{
		if( MDNSServices[i] )
		{
			os_zfree( MDNSServices[i] );			MDNSServices[i] = 0;
			os_zfree( MDNSServiceTexts[i] );		MDNSServiceTexts[i] = 0;
			MDNSServicePorts[i] = 0;
		}
	}
	nr_services = 0;
	ets_memset( MyLocalName, 0, sizeof( MyLocalName ) );
	MDNSNames[0] = strdupcaselower( &SETTINGS.DeviceName[0] );
}

uint8_t * ICACHE_FLASH_ATTR ParseMDNSPath( uint8_t * dat, char * topop, int * len )
{
	int l;
	int j;
	*len = 0;
	do
	{
		l = *(dat++);
		if( l == 0 )
		{
			*topop = 0;
			return dat;
		}
		if( *len + l >= MAX_MDNS_PATH ) return 0;

		//If not our first time through, add a '.'
		if( *len != 0 )
		{
			*(topop++) = '.';
			(*len)++;
		}

		for( j = 0; j < l; j++ )
		{
			if( dat[j] >= 'A' && dat[j] <= 'Z' )
				topop[j] = dat[j] - 'A' + 'a';
			else
				topop[j] = dat[j];
		}
		topop += l;
		dat += l;
		*topop = 0; //Null terminate.
		*len += l;
	} while( 1 );
}

uint8_t * ICACHE_FLASH_ATTR SendPathSegment( uint8_t * dat, const char * path )
{
	char c;
	int i;
	do
	{
		const char * pathstart = path;
		int this_seg_length = 0;
		while( c = *(path++) )
		{
			if( c == '.' ) break;
			this_seg_length++;
		}
		if( !this_seg_length ) return dat;

		*(dat++) = this_seg_length;
		for( i = 0; i < this_seg_length; i++ )
		{
			*(dat++) = *(pathstart++);
		}
	} while( c != 0 );
	return dat;
}

uint8_t * ICACHE_FLASH_ATTR TackTemp( uint8_t * obptr )
{
	*(obptr++) = ets_strlen( MDNSNames[0] );
	ets_memcpy( obptr, MDNSNames[0], ets_strlen( MDNSNames[0] ) );
	obptr+=ets_strlen( MDNSNames[0] );
	*(obptr++) = 0xc0; *(obptr++) = 0x0c; //continue the name.
	return obptr;
}

static void ICACHE_FLASH_ATTR SendOurARecord( uint8_t * namestartptr, int xactionid, int stlen )
{
	//Found match with us.
	//Send a response.

	uint8_t outbuff[MAX_MDNS_PATH+32];
	uint8_t * obptr = outbuff;
	uint16_t * obb = (uint16_t*)outbuff;
	*(obb++) = xactionid;
	*(obb++) = HTONS(0x8400); //Authortative response.
	*(obb++) = 0;
	*(obb++) = HTONS(1); //1 answer.
	*(obb++) = 0;
	*(obb++) = 0;

	obptr = (uint8_t*)obb;
	ets_memcpy( obptr, namestartptr, stlen+1 ); //Hack: Copy the name in.
	obptr += stlen+1;
	*(obptr++) = 0;
	*(obptr++) = 0x00; *(obptr++) = 0x01; //A record
	*(obptr++) = 0x80; *(obptr++) = 0x01; //Flush cache + in ptr.
	*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
	*(obptr++) = 0x00; *(obptr++) = 120;   //very short. (120 seconds)
	*(obptr++) = 0x00; *(obptr++) = 0x04; //Size 4 (IP)
	ets_memcpy( obptr, pMDNSServer->proto.udp->local_ip, 4 );
	obptr+=4;
	uint32_t md = MDNS_BRD;
	ets_memcpy( pMDNSServer->proto.udp->remote_ip, &md, 4 );
	espconn_sent( pMDNSServer, outbuff, obptr - outbuff );
}

static void ICACHE_FLASH_ATTR SendAvailableServices( uint8_t * namestartptr, int xactionid, int stlen )
{
	int i;
	if( nr_services == 0 ) return;
	uint8_t outbuff[(MAX_MDNS_PATH+14)*MAX_MDNS_SERVICES+32];
	uint8_t * obptr = outbuff;
	uint16_t * obb = (uint16_t*)outbuff;
	*(obb++) = xactionid;
	*(obb++) = HTONS(0x8400); //Authortative response.
	*(obb++) = 0;
	*(obb++) = HTONS(nr_services); //Answers
	*(obb++) = 0;
	*(obb++) = 0;

	obptr = (uint8_t*)obb;

	for( i = 0; i < nr_services; i++ )
	{
		char localservicename[MAX_MDNS_PATH+8];

		ets_memcpy( obptr, namestartptr, stlen+1 ); //Hack: Copy the name in.
		obptr += stlen+1;
		*(obptr++) = 0;
		*(obptr++) = 0x00; *(obptr++) = 0x0c; //PTR record
		*(obptr++) = 0x00; *(obptr++) = 0x01; //Don't flush cache + ptr
		*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
		*(obptr++) = 0x00; *(obptr++) = 0xf0; //4 minute TTL.

		int sl = ets_strlen( MDNSServices[i] );
		ets_memcpy( localservicename, MDNSServices[i], sl );
		ets_memcpy( localservicename+sl, ".local", 7 );
		*(obptr++) = 0x00; *(obptr++) = sl+8;
		obptr = SendPathSegment( obptr, localservicename );
		*(obptr++) = 0x00;
	}

	//Send to broadcast.
	uint32_t md = MDNS_BRD;
	ets_memcpy( pMDNSServer->proto.udp->remote_ip, &md, 4 );
	espconn_sent( pMDNSServer, outbuff, obptr - outbuff );
}



static void ICACHE_FLASH_ATTR SendSpecificService( int sid, uint8_t*namestartptr, int xactionid, int stlen, int is_prefix )
{
	uint8_t outbuff[256];
	uint8_t * obptr = outbuff;
	uint16_t * obb = (uint16_t*)outbuff;

	const char *sn = MDNSServices[sid];
	const char *st = MDNSServiceTexts[sid]; int stl = ets_strlen( st );
	int sp = MDNSServicePorts[sid];

	*(obb++) = xactionid;
	*(obb++) = HTONS(0x8400); //We are authoratative. And this is a response.
	*(obb++) = 0;
	*(obb++) = HTONS(sp?4:3); //4 answers. (or 3 if we don't have a port)
	*(obb++) = 0;
	*(obb++) = 0;


	if( !sn || !st ) return;

	obptr = (uint8_t*)obb;
//	ets_memcpy( obptr, namestartptr, stlen+1 ); //Hack: Copy the name in.
//	obptr += stlen+1;
	obptr = SendPathSegment( obptr,  sn );
	obptr = SendPathSegment( obptr,  "local" );
	*(obptr++) = 0;

	*(obptr++) = 0x00; *(obptr++) = 0x0c; //PTR record
	*(obptr++) = 0x00; *(obptr++) = 0x01; //No Flush cache + in ptr.
	*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
	*(obptr++) = 0x00; *(obptr++) = 100; //very short. (100 seconds)

	if( !is_prefix )
	{
		*(obptr++) = 0x00; *(obptr++) = ets_strlen( MDNSNames[0] ) + 3; //Service...
		obptr = TackTemp( obptr );
	}
	else
	{
		*(obptr++) = 0x00; *(obptr++) = 2;
		*(obptr++) = 0xc0; *(obptr++) = 0x0c; //continue the name.
	}


	if( !is_prefix )
	{
		obptr = TackTemp( obptr );
	}
	else
	{
		*(obptr++) = 0xc0; *(obptr++) = 0x0c; //continue the name.
	}

	*(obptr++) = 0x00; *(obptr++) = 0x10; //TXT record
	*(obptr++) = 0x80; *(obptr++) = 0x01; //Flush cache + in ptr.
	*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
	*(obptr++) = 0x00; *(obptr++) = 100; //very short. (100 seconds)
	*(obptr++) = 0x00; *(obptr++) = stl + 1; //Service...
	*(obptr++) = stl; //Service length
	ets_memcpy( obptr, st, stl );
	obptr += stl;

	//Service record
	if( sp )
	{
		int localnamelen = ets_strlen( MyLocalName );


		if( !is_prefix )
		{
			obptr = TackTemp( obptr );
		}
		else
		{
			*(obptr++) = 0xc0; *(obptr++) = 0x0c; //continue the name.
		}

		//obptr = TackTemp( obptr );
		//*(obptr++) = 0xc0; *(obptr++) = 0x2f; //continue the name.
		*(obptr++) = 0x00; *(obptr++) = 0x21; //SRV record
		*(obptr++) = 0x80; *(obptr++) = 0x01; //Don't Flush cache + in ptr.
		*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
		*(obptr++) = 0x00; *(obptr++) = 100; //very short. (100 seconds)
		*(obptr++) = 0x00; *(obptr++) = localnamelen + 7 + 1; //Service?
		*(obptr++) = 0x00; *(obptr++) = 0x00; //Priority
		*(obptr++) = 0x00; *(obptr++) = 0x00; //Weight
		*(obptr++) = sp>>8; *(obptr++) = sp&0xff; //Port#
		obptr = SendPathSegment( obptr, MyLocalName );
		*(obptr++) = 0;
	}

	//A Record
	obptr = SendPathSegment( obptr, MyLocalName );
	*(obptr++) = 0;
	*(obptr++) = 0x00; *(obptr++) = 0x01; //A record
	*(obptr++) = 0x80; *(obptr++) = 0x01; //Flush cache + in ptr.
	*(obptr++) = 0x00; *(obptr++) = 0x00; //TTL
	*(obptr++) = 0x00; *(obptr++) = 30; //very short. (30 seconds)
	*(obptr++) = 0x00; *(obptr++) = 0x04; //Size 4 (IP)
	ets_memcpy( obptr, pMDNSServer->proto.udp->local_ip, 4 );
	obptr+=4;

	//Send to broadcast.
	uint32_t md = MDNS_BRD;
	ets_memcpy( pMDNSServer->proto.udp->remote_ip, &md, 4 );
	espconn_sent( pMDNSServer, outbuff, obptr - outbuff );
}


static void ICACHE_FLASH_ATTR got_mdns_packet(void *arg, char *pusrdata, unsigned short len)
{
	int i, j, stlen;
	char path[MAX_MDNS_PATH];

	remot_info * ri = 0;
	espconn_get_connection_info( pMDNSServer, &ri, 0);
	ets_memcpy( pMDNSServer->proto.udp->remote_ip, ri->remote_ip, 4 );
	pMDNSServer->proto.udp->remote_port = ri->remote_port;

	uint16_t * psr = (uint16_t*)pusrdata;
	uint16_t xactionid = HTONS( psr[0] );
	uint16_t flags = HTONS( psr[1] );
	uint16_t questions = HTONS( psr[2] );
	uint16_t answers = HTONS( psr[3] );

	uint8_t * dataptr = (uint8_t*)pusrdata + 12;
	uint8_t * dataend = dataptr + len;

	if( flags & 0x8000 )
	{
		//Response

		//Unused; MDNS does not fit the browse model we want to use.
	}
	else
	{
		//Query
		for( i = 0; i < questions; i++ )
		{
			uint8_t * namestartptr = dataptr;
			//Work our way through.
			dataptr = ParseMDNSPath( dataptr, path, &stlen );

			if( dataend - dataptr < 10 ) return;

			if( !dataptr )
			{
				return;
			}

			int pathlen = ets_strlen( path );
			if( pathlen < 6 )
			{
				continue;
			}
			if( strcmp( path + pathlen - 6, ".local" ) != 0 )
			{
				continue;
			}
			uint16_t record_type = ( dataptr[0] << 8 ) | dataptr[1];
			uint16_t record_class = ( dataptr[2] << 8 ) | dataptr[3];

			const char * path_first_dot = path;
			const char * cpp = path;
			while( *cpp && *cpp != '.' ) cpp++;
			int dotlen = 0;
			if( *cpp == '.' )
			{
				path_first_dot = cpp+1;
				dotlen = path_first_dot - path - 1;
			}
			else 
				path_first_dot = 0;

			int found = 0;
			for( i = 0; i < MAX_MDNS_NAMES; i++ )
			{
				//Handle [hostname].local, or [hostname].[service].local
				if( MDNSNames[i] && dotlen && ets_strncmp( MDNSNames[i], path, dotlen ) == 0 && dotlen == ets_strlen( MDNSNames[i] )) 
				{
					found = 1;
					if( record_type == 0x0001 ) //A Name Lookup.
						SendOurARecord( namestartptr, xactionid, stlen );
					else
						SendSpecificService( i, namestartptr, xactionid, stlen, 1 );
				}
			}
	
			if( !found ) //Not a specific entry lookup...
			{
				//Is this a browse?
				if( ets_strcmp( path, "_services._dns-sd._udp.local" ) == 0 )
				{
					SendAvailableServices( namestartptr, xactionid, stlen );
				}
				else
				{
					//A specific service?
					for( i = 0; i < MAX_MDNS_SERVICES; i++ )
					{
						const char * srv = MDNSServices[i];
						if( !srv ) continue;
						int sl = ets_strlen( srv );
						if( strncmp( path, srv, sl ) == 0 )
						{
							SendSpecificService( i, namestartptr, xactionid, stlen, 0 );
						}
					}
				}
			}
		}
	}
}

void ICACHE_FLASH_ATTR SetupMDNS()
{
	MDNSNames[0] = strdupcaselower( &SETTINGS.DeviceName[0] );

    pMDNSServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pMDNSServer, 0, sizeof( struct espconn ) );
	pMDNSServer->type = ESPCONN_UDP;
	pMDNSServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pMDNSServer->proto.udp->local_port = 5353;
	espconn_regist_recvcb(pMDNSServer, got_mdns_packet);
	espconn_create( pMDNSServer );
}

int ICACHE_FLASH_ATTR JoinGropMDNS()
{
	uint32_t ip = GetCurrentIP();
	if( ip )
	{
		struct ip_addr grpip;
		grpip.addr = MDNS_BRD;
		printf( "IGMP Joining: %08x %08x\n", ip, grpip.addr );
		ets_memcpy( igmp_bound, &grpip.addr, 4 ); 
		espconn_igmp_join( (ip_addr_t *)&ip, &grpip);
		return 0;
	}
	return 1;
}


