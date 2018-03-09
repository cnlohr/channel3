all : burn

SUBMODULE:=YES
BUILD:=REGULAR
MAIN_MHZ:=80
#173
USE_I2S:=SUBMODULE
USE_PRINT:=YES
SRCS:=main.c i2s_engine.c
ESPTOOLOPTS?=-b 1000000
ADDITIONAL_DEPS:=xmit_tables.h

xmit_tables.h : tablemaker
	./tablemaker > xmit_tables.h

tablemaker : tablemaker.c
	gcc -o $@ $^ -lm

include nosdk8266/Makefile


