#
# Makefile for the Ingenic JZ4780.
#

# Object file lists.

obj-y += prom.o irq.o time.o reset.o setup.o dma.o \
	platform.o cpm.o #gpiolib.o

obj-$(CONFIG_PROC_FS)		+= proc.o

obj-$(CONFIG_DWC_LM_INTERFACE)  += lm.o lm-device.o
# board specific support
obj-$(CONFIG_JZ4780_F4780)	+= board-f4780.o
obj-$(CONFIG_JZ4780_NORMA)	+= board-norma.o
obj-$(CONFIG_JZ4780_NP_T706)	+= board-np-t706.o
obj-$(CONFIG_SOC_JZ4780)        += fpu.o

# PM support

obj-$(CONFIG_PM)         += pm.o sleep.o

# CPU Frequency scaling support

obj-$(CONFIG_CPU_FREQ_JZ)       += cpufreq.o

