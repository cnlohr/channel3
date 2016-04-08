FW_FILE_1:=image.elf-0x00000.bin
FW_FILE_2:=image.elf-0x40000.bin
TARGET_OUT:=image.elf
all : $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


SRCS:=driver/uart.c \
	common/http.c \
	common/mystuff.c \
	common/flash_rewriter.c \
	common/commonservices.c \
	common/http_custom.c \
	common/mdns.c \
	common/mfs.c \
	user/custom_commands.c \
	user/ntsc_broadcast.c \
	user/user_main.c \
	user/3d.c \
	tablemaker/broadcast_tables.c \
	tablemaker/CbTable.c

GCC_FOLDER:=~/esp-open-sdk/xtensa-lx106-elf
ESPTOOL_PY:=~/esp-open-sdk/esptool/esptool.py
SDK:=/home/baitisj/esp-open-sdk/esp_iot_sdk_v1.5.2
PORT:=/dev/ttyUSB0
#PORT:=/dev/ttyACM0

XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Iuser -Os -I$(SDK)/include/ -Icommon -DICACHE_FLASH

#	-Wl,--gc-sections \
#	-Wl,--relax  \
#	-flto \

LDFLAGS_CORE:=\
	-nostdlib \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	-g \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libpp.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libcrypto.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

#image.elf : $(OBJS)
#	$(PREFIX)ld $^ $(LDFLAGS) -o $@

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^  $(LINKFLAGS) -o $@
	nm -S -n $(TARGET_OUT) > image.map
	$(PREFIX)objdump -S $@ > image.lst

$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(ESPTOOL_PY) elf2image $(TARGET_OUT)

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(ESPTOOL_PY) elf2image $(TARGET_OUT)

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)

#If you have space, MFS should live at 0x100000, if you don't it can also live at
#0x10000.  But, then it is limited to 180kB.  You might need to do this if you have a 512kB 
#ESP variant.

burnweb : web/page.mpfs
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x10000 web/page.mpfs)||(true)


IP?=192.168.4.1

netburn : image.elf $(FW_FILE_1) $(FW_FILE_2)
	web/execute_reflash $(IP) 0x00000.bin 0x40000.bin

clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


