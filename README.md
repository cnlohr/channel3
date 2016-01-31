# channel3

ESP8266 Analog Broadcast Television Interface

Hook an antenna up to GPIO3/RX, tune your analog TV to Channel 3.  Power the ESP on!

This uses the I2S Bus in the same way the esp8266ws2812i2s project works.  Difference is it cranks the output baud to 80 MHz and codes bit patterns into that create effects at around 60 MHz.
