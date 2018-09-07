#
# Makefile for MMC/SD host controller drivers
#

obj-$(CONFIG_MMC_JZSOC) += jz_mmc_main.o \
			jz_mmc_controller.o \
			jz_mmc_msc.o \
			jz_mmc_pio.o \
			jz_mmc_gpio.o

ifeq ($(CONFIG_JZ_MSC_USE_DMA), y)
obj-$(CONFIG_JZ_CDMA) += jz_mmc_dma.o
obj-$(CONFIG_JZ_SDMA) += jz_mmc_sdma.o
else
obj-$(CONFIG_MMC_JZSOC) += jz_mmc_dma.o
endif

