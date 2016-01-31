# channel3

ESP8266 Analog Broadcast Television Interface

Hook an antenna up to GPIO3/RX, tune your analog TV to Channel 3.  Power the ESP on!

## Background

This uses the I2S Bus in the same way the esp8266ws2812i2s project works.  Difference is it cranks the output baud to 80 MHz and codes bit patterns into that create effects at around 60 MHz.  Please not that this project does not actually output video up at the 60 MHz purposefully.  Instead it relies on secondary effects creating enough noise at the VHF Channel 3 mark to make it understandable by a TV.

Honestly, I spent days trying to find the optimal bit patterns to create overtones in the Channel 3 frequency space and nothing I planned for worked.  I just tried some random patterns and everything worked perfectly.  Those are the default bit patterns.

The way this works is by setting up DMA operations for all sync and control lines, as well as a bunch of lines in the middle for the actual display.  When running, if we are on a text line, the DMA interrupt fills the data in for the line.  I use the 'unused' bits in the DMA structure to signal to the interrupt handler what it should do.

You should read the user/ntsc_broadcast.c file for more information about how the DMA engine updates the RF output on the pin.

I actually update the frames in the main thread, so significant wifi events will cause frame glitches. 

## Rawdraw and 3D

For all the 3D and text, I'm using a new modified version of my "rawdraw" library ( http://github.com/cnlohr/rawdraw ) for 3D I'm using fixed point numbers, with 256 as the unit value, and the bottom 8 bits are the fractional component.

## Youtube video

I made a youtube video about this project, available here:

[[NOT YET UPDATED]]
