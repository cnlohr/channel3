#ifndef _ESP8266_ROM_FUNCTS
#define _ESP8266_ROM_FUNCTS

//This is my best guess at some of the ROM functions for the ESP8266.
//I have no idea if this stuff is correct!

#include <spi_flash.h>
#include <c_types.h>

void Cache_Read_Disable(); //Can't seem to operate...
void Cache_Read_Enable();

//PROVIDE ( Cache_Read_Disable = 0x400047f0 );
//PROVIDE ( Cache_Read_Enable = 0x40004678 );

typedef struct {
  uint32_t i[2];
  uint32_t buf[4];
  unsigned char in[64];
  unsigned char digest[16];
} MD5_CTX;

void MD5Init  ( MD5_CTX *mdContext);
void MD5Update( MD5_CTX *mdContext, const unsigned char *inBuf, unsigned int inLen);
void MD5Final ( unsigned char hash[], MD5_CTX *mdContext);


//SHA Stuff from: https://github.com/pvvx/esp8266web/blob/master/app/include/bios/cha1.h
#define	SHA1_HASH_LEN	20
typedef struct {
    uint32 state[5];
    uint32 count[2];
    uint8 buffer[64];
	uint8 extra[40];
} SHA1_CTX;

void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context,
                const uint8 *data,
                size_t len);
void SHA1Final(uint8 digest[SHA1_HASH_LEN], SHA1_CTX* context);
void SHA1Transform(uint32 state[5], const uint8 buffer[64]);



//SPI_FLASH_SEC_SIZE      4096

void SPIEraseSector(uint16 sec);
void SPIEraseArea(uint32 start,uint32 len); //Doesn't work?
void SPIEraseBlock(uint16 blk);
void SPIWrite(uint32 des_addr, uint32_t *src_addr, uint32_t size);
void SPIRead(uint32 src_addr, uint32_t *des_addr, uint16_t size);
void SPILock( uint16_t sec ); //??? I don't use this?
void SPIUnlock( ); //??? I don't use this? -> Seems to crash.

extern SpiFlashChip * flashchip; //don't forget: flashchip->chip_size = 0x01000000;


/*
		flashchip->chip_size = 0x01000000;

	{
		uint32_t __attribute__ ((aligned (16)))  t[1024];
		t[0] = 0xAABBCCDD;
		t[1] = 0xEEFF0011;
		for( i = 0; i < 10000; i++ ) uart0_sendStr("A\n");
		SPIEraseSector( 1000000/4096 );
		for( i = 0; i < 10000; i++ ) uart0_sendStr("B\n");
		SPIWrite( 1000000, t, 8 );
	}

		for( i = 0; i < 10000; i++ ) uart0_sendStr("C\n");

	while(1)
	{
		char ct[32];
		uint32_t __attribute__ ((aligned (16)))  ret = 0x12345678;
//		SPIRead( 1000000, &ret, 4 );
		ret = *(uint32_t*)(0x40200000+1000004);
		ets_sprintf( ct, "%08x\n", ret );
		printf( ct );
	}
*/


void software_reset();



#endif


