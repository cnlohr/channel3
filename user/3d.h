#ifndef _3d_H
#define _3d_H

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "video_broadcast.h"

extern int gframe;
extern uint8_t * frontframe;
extern int16_t ProjectionMatrix[16];
extern int16_t ModelviewMatrix[16];
extern int CNFGPenX, CNFGPenY;
extern uint8_t CNFGBGColor;
extern uint8_t CNFGLastColor;
extern uint8_t CNFGDialogColor; //background for boxes

void CNFGTackSegment( int x0, int y0, int x1, int y1 );
int LABS( int x );
void (*CNFGTackPixel)( int x, int y ); //Unsafe plot pixel.
//#define CNFGTackPixelFAST( x, y ) { frontframe[(x+y*FBW)>>2] |= 2<<( (x&3)<<1 ); }  //Store in 4 bits per byte.
void LocalToScreenspace( int16_t * coords_3v, int16_t * o1, int16_t * o2 );
int16_t tdSIN( uint8_t iv );
int16_t tdCOS( uint8_t iv );

/* Colors:
    0 .. 15 = standard-density colors
	16: Black, Double-Density
	17: White, Double-Density
*/
void CNFGColor( uint8_t col ); 

void ICACHE_FLASH_ATTR tdTranslate( int16_t * f, int16_t x, int16_t y, int16_t z );		//Operates ON f
void ICACHE_FLASH_ATTR tdScale( int16_t * f, int16_t x, int16_t y, int16_t z );			//Operates ON f
void ICACHE_FLASH_ATTR tdRotateEA( int16_t * f, int16_t x, int16_t y, int16_t z );		//Operates ON f

void ICACHE_FLASH_ATTR CNFGDrawText( const char * text, int scale );
void ICACHE_FLASH_ATTR CNFGDrawBox(  int x1, int y1, int x2, int y2 );
void ICACHE_FLASH_ATTR CNFGTackRectangle( short x1, short y1, short x2, short y2 );
void ICACHE_FLASH_ATTR tdMultiply( int16_t * fin1, int16_t * fin2, int16_t * fout );
void ICACHE_FLASH_ATTR tdPTransform( int16_t * pin, int16_t * f, int16_t * pout );
void  td4Transform( int16_t * pin, int16_t * f, int16_t * pout );
void ICACHE_FLASH_ATTR MakeTranslate( int x, int y, int z, int16_t * out );
void ICACHE_FLASH_ATTR Perspective( int fovx, int aspect, int zNear, int zFar, int16_t * out );
void ICACHE_FLASH_ATTR tdIdentity( int16_t * matrix );
void ICACHE_FLASH_ATTR MakeYRotationMatrix( uint8_t angle, int16_t * f );
void ICACHE_FLASH_ATTR MakeXRotationMatrix( uint8_t angle, int16_t * f );
void ICACHE_FLASH_ATTR DrawGeoSphere();
void ICACHE_FLASH_ATTR Draw3DSegment( int16_t * c1, int16_t * c2 );

int16_t ICACHE_FLASH_ATTR tdPerlin2D( int16_t x, int16_t y );
int16_t ICACHE_FLASH_ATTR tdFLerp( int16_t a, int16_t b, int16_t t );
int16_t ICACHE_FLASH_ATTR tdNoiseAt( int16_t x, int16_t y );

#endif

