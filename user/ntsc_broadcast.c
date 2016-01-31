/******************************************************************************
 * Copyright 2013-2015 Espressif Systems
 *           2015 <>< Charles Lohr
 *
 * FileName: i2s_freertos.c
 *
 * Description: I2S output routines for a FreeRTOS system. Uses DMA and a queue
 * to abstract away the nitty-gritty details.
 *
 * Modification history:
 *     2015/06/01, v1.0 File created.
 *     2015/07/23, Switch to making it a WS2812 output device.
 *     2016/01/28, Modified to re-include TX_ stuff.
*******************************************************************************

Notes:

 This is pretty badly hacked together from the MP3 example.
 I spent some time trying to strip it down to avoid a lot of the TX_ stuff. 
 That seems to work.

 Major suggestions that I couldn't figure out:
	* Use interrupts to disable DMA, so it isn't running nonstop.
    * Use interrupts to flag when new data can be sent.

 When I try using interrupts, it seems to work for a bit but things fall apart
 rather quickly and the engine just refuses to send anymore until reboot.

 The way it works right now is to keep the DMA running forever and just update
 the data in the buffer so it continues sending the frame.

Extra copyright info:
  Actually not much of this file is Copyright Espressif, comparativly little
  mostly just the stuff to make the I2S bus go.

*******************************************************************************/


#include "slc_register.h"
#include "mystuff.h"
#include <c_types.h>
#include "ntsc_broadcast.h"
#include "user_interface.h"
#include "pin_mux_register.h"
#include <dmastuff.h>

#define WS_I2S_BCK 1  //Can't be less than 1.
#define WS_I2S_DIV 2

#define I2SDMABUFLEN (159)		//Length of one buffer, in 32-bit words.

//Bit clock @ 80MHz = 12.5ns
//Word clock = 400ns
//Each NTSC line = 15,734.264 Hz.  63556 ns
//Each group of 4 bytes = 

#define NTSC_LINES 525
#define LINETYPES 6

//#define PATTERN 0b10010010010010010010010010010010 //Why does this work well?  (11 repeats)
//#define PATTERN 0b10010010100100101001001010010010 //12 repeats


uint32_t RFmaps[] = {
	BLACK_LEVEL_d, WTB_LEVEL_d, BTW_LEVEL_d, WHITE_LEVEL_d, SYNC_LEVEL_d };

//WS_I2S_DIV - if 1 will actually be 2.  Can't be less than 2.

//		CLK_I2S = 160MHz / I2S_CLKM_DIV_NUM
//		BCLK = CLK_I2S / I2S_BCK_DIV_NUM
//		WS = BCLK/ 2 / (16 + I2S_BITS_MOD)
//		Note that I2S_CLKM_DIV_NUM must be >5 for I2S data
//		I2S_CLKM_DIV_NUM - 5-127
//		I2S_BCK_DIV_NUM - 2-127

#define FT_STA 0
#define FT_STB 1
#define FT_B 2
#define FT_SRA 3
#define FT_SRB 4
#define FT_LIN 5
uint32_t i2sBD[I2SDMABUFLEN*LINETYPES];

//I2S DMA buffer descriptors
static struct sdio_queue i2sBufDesc[NTSC_LINES];
//Queue which contains empty DMA buffers

//This routine is called as soon as the DMA routine has something to tell us. All we
//handle here is the RX_EOF_INT status, which indicate the DMA has sent a buffer whose
//descriptor has the 'EOF' field set to 1.
int gi;
int gline;
int gframe;


uint8_t framebuffer[((FBW/8)*(FBH))*2];


LOCAL void slc_isr(void) {
	//portBASE_TYPE HPTaskAwoken=0;
	struct sdio_queue *finishedDesc;
	uint32 slc_intr_status;
	int dummy, x;

	//Grab int status
	slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);//slc_intr_status);
	if (slc_intr_status & SLC_RX_EOF_INT_ST) {
		//The DMA subsystem is done with this block: Push it on the queue so it can be re-used.
		finishedDesc=(struct sdio_queue*)READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
		struct sdio_queue * next = (struct sdio_queue *)finishedDesc->next_link_ptr;
		uint32_t * b = (uint32_t*)next->buf_ptr;
		if( next->unused == 2 )
		{
			gline = 0;
		}
		else if( next->unused == 3 )
		{
			gline = 0; gframe++;
		}
		else if( next->unused )
		{
			uint8_t pxm = 0;
			uint8_t idx = 25;
			uint8_t xt = 0;
			uint8_t * f = &framebuffer[(FBW/8)*gline + ((FBW/8)*FBH)*(gframe&1)];
			for( x = 0; x < 128/4; x++ )
			{
				uint8_t fx = f[x];
				b[idx++] = RFmaps[ (fx>>0)&3 ];
				b[idx++] = RFmaps[ (fx>>2)&3 ];
				b[idx++] = RFmaps[ (fx>>4)&3 ];
				b[idx++] = RFmaps[ (fx>>6)&3 ];
			}
			gline++;
		}
	}
}

//Initialize I2S subsystem for DMA circular buffer use
void ICACHE_FLASH_ATTR testi2s_init() {
	int x, y;
		
//	uint32_t i2sBD[I2SDMABUFLEN*LINETYPES];

	redo_maps();

	//Initialize DMA buffer descriptors in such a way that they will form a circular
	//buffer.
	for (x=0; x<NTSC_LINES; x++) {
		i2sBufDesc[x].owner=1;
		i2sBufDesc[x].eof=1;
		i2sBufDesc[x].sub_sof=0;
		i2sBufDesc[x].datalen=I2SDMABUFLEN*4;
		i2sBufDesc[x].blocksize=I2SDMABUFLEN*4;
//		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(x%2)*I2SDMABUFLEN];
		i2sBufDesc[x].unused=0;
		i2sBufDesc[x].next_link_ptr=(int)((x<(NTSC_LINES-1))?(&i2sBufDesc[x+1]):(&i2sBufDesc[0]));
	}

	for( x = 0; x < 3; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STA)*I2SDMABUFLEN];
	for( ; x < 6; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STB)*I2SDMABUFLEN];
	for( ; x < 9; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STA)*I2SDMABUFLEN];
	for( ; x < 19; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_B)*I2SDMABUFLEN];
	i2sBufDesc[x-1].unused = 2;
	for( ; x < 260; x++ )
	{
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_LIN)*I2SDMABUFLEN];
		i2sBufDesc[x].unused = 1;//x&1; //every other line
	}
	for( ; x < 263; x++ )
	{
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_B)*I2SDMABUFLEN];
	}

	//263rd frame, begin odd sync.
	for( ; x < 266; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STA)*I2SDMABUFLEN];
	i2sBufDesc[x++].buf_ptr=(uint32_t)&i2sBD[(FT_SRA)*I2SDMABUFLEN];
	for( ; x < 269; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STB)*I2SDMABUFLEN];
	i2sBufDesc[x++].buf_ptr=(uint32_t)&i2sBD[(FT_SRB)*I2SDMABUFLEN];
	for( ; x < 272; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_STA)*I2SDMABUFLEN];
	for( ; x < 283; x++ )
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_B)*I2SDMABUFLEN];
	i2sBufDesc[x-1].unused = 3;
	for( ; x < 520; x++ )
	{
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_LIN)*I2SDMABUFLEN];
		i2sBufDesc[x].unused = 1;//x&1; //every other line
	}
	for( ; x < NTSC_LINES; x++ )
	{
		i2sBufDesc[x].buf_ptr=(uint32_t)&i2sBD[(FT_B)*I2SDMABUFLEN];
	}


	//Reset DMA
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);

	//Clear DMA int flags
	SET_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);
	CLEAR_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);

	//Enable and configure DMA
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_CONF0,(1<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF,SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE);
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);
	
	//Feed dma the 1st buffer desc addr
	//To send data to the I2S subsystem, counter-intuitively we use the RXLINK part, not the TXLINK as you might
	//expect. The TXLINK part still needs a valid DMA descriptor, even if it's unused: the DMA engine will throw
	//an error at us otherwise. Just feed it any random descriptor.
	CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&i2sBufDesc[1]) & SLC_TXLINK_DESCADDR_MASK); //any random desc is OK, we don't use TX but it needs something valid
	CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&i2sBufDesc[0]) & SLC_RXLINK_DESCADDR_MASK);

	//Attach the DMA interrupt
	ets_isr_attach(ETS_SLC_INUM, slc_isr);
	//Enable DMA operation intr
	WRITE_PERI_REG(SLC_INT_ENA,  SLC_RX_EOF_INT_ENA);
	//clear any interrupt flags that are set
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
	///enable DMA intr in cpu
	ets_isr_unmask(1<<ETS_SLC_INUM);

	//We use a queue to keep track of the DMA buffers that are empty. The ISR will push buffers to the back of the queue,
	//the mp3 decode will pull them from the front and fill them. For ease, the queue will contain *pointers* to the DMA
	//buffers, not the data itself. The queue depth is one smaller than the amount of buffers we have, because there's
	//always a buffer that is being used by the DMA subsystem *right now* and we don't want to be able to write to that
	//simultaneously.
	//dmaQueue=xQueueCreate(I2SDMABUFCNT-1, sizeof(int*));

	//Start transmission
	SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
	SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

//----

	//Init pins to i2s functions
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);

	//Enable clock to i2s subsystem
	i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

	//Reset I2S subsystem
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);

	//Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
	CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN|(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
	//Enable DMA in i2s subsystem
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);

	//tx/rx binaureal
	CLEAR_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

	//Clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);

	//trans master&rece slave,MSB shift,right_first,msb right
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						((WS_I2S_BCK&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						((WS_I2S_DIV&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));

	//No idea if ints are needed...
	//clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|
	I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

	//Start transmission
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START);
}



void ICACHE_FLASH_ATTR redo_maps() {
	int x, y;
	//Create line types
	uint32_t * l;

	//Each X is in units of 0.4us.  Each line is 158-159t or H=63.5555 long.
	for( x = 0; x < sizeof( framebuffer ); x++ )
	{
		framebuffer[x] = rand();
	}

	//Line type 0: Even field synctype A.
	y = FT_STA;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 6; x++)				l[x]= SYNC_LEVEL;
	for (; x < 79; x++)				l[x]= BLACK_LEVEL;
	for (; x < 6+79; x++)			l[x]= SYNC_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;

	//Line type 1: Even field Synchtype B.
	y = FT_STB;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 68; x++)				l[x]= SYNC_LEVEL;
	for (; x < 79; x++)				l[x]= BLACK_LEVEL;
	for (; x < 68+79; x++)			l[x]= SYNC_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;

	//Line type 2: Black.
	y = FT_B;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 12; x++)				l[x]= SYNC_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;

	//Odd fields 

	//Line type 3: Flippy odd field type
	y = FT_SRA;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 6; x++)				l[x]= SYNC_LEVEL;
	for (; x < 79; x++)				l[x]= BLACK_LEVEL;
	for (; x < 68+79; x++)			l[x]= SYNC_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;

	//Line type 4: Flippy odd field type B
	y = FT_SRB;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 68; x++)				l[x]= SYNC_LEVEL;
	for (; x < 79; x++)				l[x]= BLACK_LEVEL;
	for (; x < 6+79; x++)			l[x]= SYNC_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;

	//Line type 5: Line
	y = FT_LIN;	l = &i2sBD[y*I2SDMABUFLEN];	x = 0; 
	for (; x < 11; x++)				l[x]= SYNC_LEVEL;
	for (; x < 79; x++)				l[x]= BLACK_LEVEL;
	for (; x < 1+79; x++)			l[x]= WHITE_LEVEL;
	for (; x < I2SDMABUFLEN; x++)	l[x]= BLACK_LEVEL;
}
