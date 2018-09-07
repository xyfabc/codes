INCLUDE = -I../producer_fha/include
CC = arm-none-linux-gnueabi-gcc
STRIP = arm-none-linux-gnueabi-strip
PLATFORM=CONFIG_AK98
SPI ?= NO

CONFIG_YAFFS_SUPPORT_BIGFILE ?= Y
ifeq ($(CONFIG_YAFFS_SUPPORT_BIGFILE), Y)
CFLAGS 		= $(INCLUDE) -D$(PLATFORM) -DCONFIG_YAFFS_SUPPORT_BIGFILE -O2
else
CFLAGS 		= $(INCLUDE) -D$(PLATFORM) -O2
endif

ifeq ($(PLATFORM), CONFIG_AK37XX_SPI)
	SPI = YES
endif

ifeq ($(PLATFORM), CONFIG_AK39XX)
	SPI = YES
endif

ifeq ($(SPI), YES)
  MC = burnimage.o file.o spi_char_interface.o usbburn.o
else
  MC = burnimage.o file.o nand_char_interface.o usbburn.o 
endif

producer: $(MC)
	$(CC) -o $@ $(MC) ../producer_fha/fha.a -lpthread
	$(STRIP) -s $@

%.o: %.c
	$(CC) -c $(CFLAGS)  -o $@ $<

clean:
	rm -rf *.o producer

