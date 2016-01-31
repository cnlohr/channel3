
//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>

#define SPI_FLASH_SEC_SIZE 4096
#define MFS_STARTFLASHSECTOR  0x100
#define MFS_START	(MFS_STARTFLASHSECTOR*SPI_FLASH_SEC_SIZE)
#define MFS_SECTOR	256
#define MFS_FILENAMELEN 32-8
#define ENTRIES 8192

#define ENDIAN(x) x//htonl

struct MFSFileEntry
{
	char name[MFS_FILENAMELEN];
	uint32_t start;  //From beginning of mfs thing.
	uint32_t len;
} mfsfat[ENTRIES];

unsigned char mfsdata[131072*8];

unsigned long fatpointer = 0;
unsigned long datapointer = 0;

int main( int argc, char ** argv )
{
	int i;
	DIR           *d;
	struct dirent *dir;

	if( argc != 3 )
	{
		fprintf( stderr, "Error: [tool] [directory to pack] [output packed .dat file]\n" );
		return -1;
	}

	d = opendir( argv[1] );

	if (!d)
	{
		fprintf( stderr, "Error: cannot open folder for packing.\n" );
		return -2;
	}


	memcpy( mfsfat[fatpointer].name, "MPFSMPFS", 8 );
	mfsfat[fatpointer].start = 0;
	mfsfat[fatpointer].len   = 0;
	fatpointer++;


	while ((dir = readdir(d)) != NULL)
    {
		if( dir->d_type & DT_REG )
		{
			char thisfile[1024];
			struct stat buf;
			int dlen = strlen( dir->d_name );
			int sprret = snprintf( thisfile, 1023, "%s/%s", argv[1], dir->d_name );

			if( sprret > 1023 || sprret < 1 )
			{
				fprintf( stderr, "Error processing \"%s\" (snprintf)\n", dir->d_name );
				continue;
			}

			int statret = stat( thisfile, &buf );

			if( statret )
			{
				fprintf( stderr, "Error processing \"%s\" (stat)\n", dir->d_name );
				continue;
			}

			if( dlen >= MFS_FILENAMELEN )
			{
				fprintf( stderr, "Warning: Fle \"%s\" too long.\n", dir->d_name );
				continue;
			}
			if( fatpointer > ENTRIES )
			{
				fprintf( stderr, "Warning: Not enough entries to fit \"%s\".\n", dir->d_name );
				continue;
			}

			if( buf.st_size + datapointer > sizeof( mfsdata ) )
			{
				fprintf( stderr, "Error: no space left.\n" );
				return -1;
			}

			memcpy( mfsfat[fatpointer].name, dir->d_name, dlen );
			mfsfat[fatpointer].start = datapointer;
			mfsfat[fatpointer].len   = ENDIAN( buf.st_size );
			fatpointer++;

			if( buf.st_size )
			{
				FILE * f = fopen( thisfile, "rb" );
				if( !f )
				{
					fprintf( stderr, "Error: cannot open \"%s\" for reading.\n", dir->d_name );
					return -9;
				}
				fread( &mfsdata[datapointer], 1, buf.st_size, f );
				fclose( f );
				int rs = buf.st_size;
				rs = (rs+1+MFS_SECTOR)&(~(MFS_SECTOR-1));
				datapointer += rs;
				printf( "%s: %d (%ld)\n", thisfile, rs, datapointer );
			}
		}
    }

    closedir(d);

	int rs = (fatpointer+1)*sizeof(struct MFSFileEntry);
	rs = (rs+1+MFS_SECTOR)&(~(MFS_SECTOR-1));
	for( i = 0; i < fatpointer; i++ )
	{
		mfsfat[i].start = ENDIAN(mfsfat[i].start + rs );
	}

	printf( "%d %ld\n", rs, datapointer );

	FILE * f = fopen( argv[2], "w" );

	if( !f || ferror( f ) )
	{
		fprintf( stderr, "Error: cannot open \"%s\" for writing.\n", argv[2] );
	}
	fwrite( mfsfat, rs, 1, f );
	fwrite( mfsdata, datapointer, 1, f );
	fclose( f );

	return 0;
}


