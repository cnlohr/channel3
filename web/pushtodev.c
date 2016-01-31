//Unless what else is individually marked, all code in this file is
//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define sector_SIZE 4096
int sockfd;
char recvline[10000];


int PushMatch( const char * match )
{
	struct timeval tva, tvb;
	gettimeofday( &tva, 0 );
	gettimeofday( &tvb, 0 );
	int difftime = (tvb.tv_sec-tva.tv_sec)*1000000 + tvb.tv_usec - tva.tv_usec;
	while( difftime < 500000 ) //3 second timeout.
	{
		struct pollfd ufds;
		ufds.fd = sockfd;
		ufds.events = POLLIN;
		int rv = poll(&ufds, 1, 50);
		if( rv > 0 )
		{
//			tbuf = recvline;
			int n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
//			printf( "!!%d ->%s\n", n,recvline );
//			printf( "%s === %s\n", recvline, match );
			if( strncmp( recvline, match, strlen( match ) ) == 0 )
			{
//				printf( "Ok\n" ); fflush( stdout );
				return 0;
			}
		}
		gettimeofday( &tvb, 0 );
		difftime = (tvb.tv_sec-tva.tv_sec)*1000000 + tvb.tv_usec - tva.tv_usec;
	}
	return 1;
}


int main(int argc, char**argv)
{
	int n;
	struct sockaddr_in servaddr,cliaddr;
	char sendline[1000];
//	char recvline[1000];

	if (argc != 4)
	{
		printf("usage: pushtodev [IP address] [address offset] [file]\n");
		exit(-1);
	}

	int offset = atoi( argv[2] );
	const char * file = argv[3];

	if( offset <= 0 )
	{
		fprintf( stderr, "Error: Cannot write to address 0 or before.\n" );
		exit(-2);
	}

	FILE * f = fopen( file, "rb" );
	if( !f || feof( f ) )
	{
		fprintf( stderr, "Error: cannot open file.\n" );
		exit(-3);
	}



	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(7878);

	int devo = 0;
	int lastsector_block = -1;
	int resend_times = 0;
	int r;
	while( !feof( f ) )
	{
		int tries;
		int thissuccess;
		char buffer[1024];
		char bufferout[1600];
		int reads = fread( buffer, 1, 1024, f );
		int sendplace = offset + devo;
		int sendsize = reads;

#ifdef SECTOR
		int sector = sendplace / sector_SIZE;
		if( sector != lastsector_block )
		{
			char se[64];
			int sel = sprintf( se, "FE%d\r\n", sector );
	
			thissuccess = 0;
			for( tries = 0; tries < 10; tries++ )
			{
				char match[75];
				printf( "Erase: %d\n", sector );
				sendto( sockfd, se, sel, MSG_NOSIGNAL, (struct sockaddr *)&servaddr,sizeof(servaddr));
				sprintf( match, "FE%d", sector );

				if( PushMatch(match) == 0 ) { thissuccess = 1; break; }
				printf( "Retry.\n" );
			}
			if( !thissuccess )
			{
				fprintf( stderr, "Error: Timeout in communications.\n" );
				exit( -6 );
			}

			lastsector_block = sector;
		}
#else //block

		int block = sendplace / 65536;
		if( block != lastsector_block )
		{
			char se[64];
			int sel = sprintf( se, "FB%d\r\n", block );
	
			thissuccess = 0;
			for( tries = 0; tries < 10; tries++ )
			{
				char match[75];
				printf( "Erase: %d\n", block );
				sendto( sockfd, se, sel, MSG_NOSIGNAL, (struct sockaddr *)&servaddr,sizeof(servaddr));
				sprintf( match, "FB%d", block );

				if( PushMatch(match) == 0 ) { thissuccess = 1; break; }
				printf( "Retry.\n" );
			}
			if( !thissuccess )
			{
				fprintf( stderr, "Error: Timeout in communications.\n" );
				exit( -6 );
			}

			lastsector_block = block;
		}
#endif
		resend_times = 0;
resend:
		r = sprintf( bufferout, "FW%d\t%d\t", sendplace, sendsize );
		printf( "bufferout: %d %d %s\n", sendplace, sendsize, bufferout );
		memcpy( bufferout + r, buffer, sendsize );


		thissuccess = 0;
		for( tries = 0; tries < 10; tries++ )
		{
			char match[75];
			sendto( sockfd, bufferout, reads + r, MSG_NOSIGNAL, (struct sockaddr *)&servaddr,sizeof(servaddr));

			sprintf( match, "FW%d", sendplace );

			if( PushMatch(match) == 0 ) { thissuccess = 1; break; }
			printf( "Retry.\n" );
		}
		if( !thissuccess )
		{
			fprintf( stderr, "Error: Timeout in communications.\n" );
			exit( -6 );
		}

/*
		printf( "Verifying..." );
		fflush( stdout );

		int r = sprintf( bufferout, "FR%d:%d", sendplace, sendsize );
		bufferout[r] = 0;

		thissuccess = 0;
		for( tries = 0; tries < 10; tries++ )
		{
			char match[1600];
			sendto( sockfd, bufferout, r, MSG_NOSIGNAL, (struct sockaddr *)&servaddr,sizeof(servaddr));
			devo += reads;
			sprintf( match, "FR%08d", sendplace );

			if( PushMatch(match) == 0 ) {

				//Check data...
//printf( "RR:%s\n", recvline );
				char * colon1 = strchr( recvline, ':' );
				char * colon2 = (colon1)?strchr( colon1+1, ':' ):0;
//printf( "::%p %p \"%s\"\n", colon1, colon2,recvline );
				if( colon2 )
				{
					if( memcmp( colon2+1, buffer, sendsize ) == 0 )
						thissuccess = 1;
				}

				if( !thissuccess )
				{
					if( resend_times > 2 )
					{
						break;
					}
					resend_times++;
					goto resend;
				}
				break;
			}
			printf( "Retry.\n" );
		}
		if( !thissuccess )
		{
			fprintf( stderr, "Error: Fault verifying.\n" );
			exit( -6 );
		}
*/
			devo += reads;

	}

	return 0;
}
