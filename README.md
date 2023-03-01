# channel3

ESP8266 Analog Broadcast Television Interface

Hook an antenna up to GPIO3/RX, tune your analog TV to Channel 3.  Power the ESP on!

## Background and RF

This uses the I2S Bus in the same way the esp8266ws2812i2s project does.  Difference is it cranks the output baud to 80 MHz.  We set up DMA buffers and let the CPU fill them as they pass through one line at a time.  The DMA interrupt fills in the buffers one word at a time.  The I2S bus shifts those buffers out at 80 MHz!

You may say "But nyquist says you can't transmit or receive frequencies at more than 1/2 the sample rate (40 MHz in this case).  To a degree that is true.  Some people thought it may be overtones, but what happens in reality something stranger happens.  Everything you transmit is actually mirrored around 1/2 the sample rate (40 MHz).  So, transmitting 60 MHz on an 80 MHz bitclock creates a waveform both at 60 as well as 20.  This isn't perfect.  Some frequencies line up to the 80 MHz well, others do not.

We store a bit pattern in the "premodulated_table" array.  This contains bitstreams for various signals, such as the "sync" level or "colorbust" level, or any of the visual colors.  This table's length of 1408 bits per color was chosen so that when sent out one bit at a time at 80 MHz, it works out to an even multiplier of the NTCS chroma frequency of 315.0/88.0 MHz, or 3.579545455 MHz.  You can calculate this by taking 1408/80MHz = 17.6us * 3.579545 MHz = 63 cycles, exactly.  Conveniently, it also works out to an even multiplier of 61.25 MHz, Channel 3's luma center.  17.6us * 61.25 MHz = 1078 cycles, exactly! When you modulate arbitrary frequencies, sometimes the cycles come out very uneven. 

In order to generate luma (the black and white portion) we modulate 61.25 MHz.  If we generate a strong signal, it is viewed as a very "dark", and a weak signal is a very "bright."  This means when we want to send out a sync pulse, we modulate it as loud as we can... when we want to modulate white, we put out barely any signal at all.  One thing you will notice is dot pour.  This is because the signal we are sending is so terrible.  The chroma signal is very dirty and has a repeating intensity pattern.  While the chroma lines up to the 1408 bit-wide repeating patten, the total number of pixels on the screen does not.  This causes the patterns created to roll down the screen.

In order to generate color, we need to modulate in a chroma signal, 3.579MHz above the baseband.  The chroma is synchronized by a colorburst at the beginning of each line.  This also sets the level for the chroma.  Then, during the line, we can either choose a "color" that has a high coefficient at the chroma level, or a low one.  This determines how vivid the color is.  We can change phase to change the color's hue.

This is basically a 1-bit dithering DAC, operating at a frequency below the nyquist, trying to encode luma and color at the same time.  Don't be surprised that the quality's terrible.

## Code Layout

Tables for handling the line-buffer state machine are (generated/stored?) in MayCbTables.h/c, and similar tables for creating the on-wire signal encoding are in synthtables.c.

Functions to set up the DMA transfers, refill the buffers when they become empty, and change what kind of line should be sent based on the framebuffer contents are in video_broadcast.c. These functions handle all of the modulation.  This sets up the DMA, and an interrupt that is called when the DMA finishes a block (equal to one line).  Upon completion, it uses CbTable to decide what function to call to fill in the line.  The interrupt fills out the next line for DMA which keeps going.

The framebuffer is updated by various demo screens located in user_main.c.

custom_commands.c contain the custom commands used for the NTSC-specific aspects.  Using the common websockets interface there are two added commands.  These include "CO" and "CV" which set the operation mode (CO) and allow users to change the modulation table from a web interface (CV).

## Demo screens

The following demo screens are available.  They normally tick through one after another (except ones after 10), unless the user disables this in the web browser.

## Screen Modes

1. Basic intro screen, shows IP address if available.
2. ESP8266 Features
3. Intro to and completion of framebuffer copy test.  Beware, running this screen too long deliberately will cause a crash.
6. Draw a bunch of lines... IN COLOR!
7. Matrix-based 3D engine demo.
8. Dynamic 3D mesh demo.
9. Pitch for this project's github.
10. Color screen with 16 color balls.
11. 4x4 color swatches, useful for when you're messing with colors in the web GUI.

## Web interface

The web interface is borrowing the web interface from esp8266ws2812i2s.  Power on the ESP, connect to it, then, point your web browser to http://192.168.4.1.  It has a new button "NTSC."  This gives you the option to allow demo to continue from screen to screen, or freeze at a specific screen.  You can specify the screen.  Additionally, for RF testing, you can jam a color.  Whenever the color jam is set to something 0 or above, it turns off all line drawing logic, and simply outputs that color continuously.  This will prevent TV sets from seeing it, however, you can see it on other RF equipment.

It also has an interactive Javascript webworker system that lets you write code to make a new color!  You can create a new bitstream that will be transmitted when a specific color is hit.  You can edit the code and it is effective as you type.  It automatically re-starts the webworker every time you change it.

You should only output -1 or +1 as that is all the ESP can output.  It will then run a DFT with a randomized window over a frequency area you choose.  Increase the DFT window, and it will increase your q (or precision).  Decrease, it decreases your q.  This is to help see how receivers like the TV really understand the signal and help illustrate how wacky this really is.

You can try it in your own browser using this link: http://cnlohr.github.io/channel3/web/page/index.html  Click NTSC and go to town.

## Rawdraw and 3D

For all the 3D and text, I'm using a new modified version of my "rawdraw" library ( http://github.com/cnlohr/rawdraw ) for 3D I'm using fixed point numbers, with 256 as the unit value, and the bottom 8 bits are the fractional component.

## PAL Modification
To allow for PAL broadcasts, the timings in the video_broadcast-library (formerly ntsc_broadcast) were modified. Since I only wanted to use this with a black an white TV, and PAL colour is actually quite complicated to do digitally, I didn't modify the broadcast_tables (synthtables.c). So the library broadcasts a PAL compliant B/W-Signal with NTSC Colour information (kind of like NTSC50).

To enable PAL broadcasting you need to enable ```OPTS += -DPAL``` in user.cfg. 

## Youtube video

Here is the original youtube video on this project:

[![NTSC Video on the ESP8266](http://img.youtube.com/vi/SSiRkpgwVKY/0.jpg)](http://www.youtube.com/watch?v=SSiRkpgwVKY)

Here is the new video (with COLOR):

[![Broadcasting COLOR Channel 3 on an ESP](http://img.youtube.com/vi/bcez5pcp55w/0.jpg)](http://www.youtube.com/watch?v=bcez5pcp55w)


