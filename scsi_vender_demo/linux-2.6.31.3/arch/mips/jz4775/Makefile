#
# Makefile for the Ingenic JZ4775.
#

# Object file lists.

obj-y += prom.o irq.o time.o reset.o setup.o dma.o \
	platform.o cpm.o #gpiolib.o

obj-$(CONFIG_PROC_FS)		+= proc.o

obj-$(CONFIG_DWC_LM_INTERFACE)  += lm.o lm-device.o
# board specific support
obj-$(CONFIG_JZ4775_F4775)	+= board-f4775.o
obj-$(CONFIG_JZ4775_ORION)	+= board-orion.o
obj-$(CONFIG_JZ4775_MENSA)	+= board-mensa.o
obj-$(CONFIG_SOC_JZ4775)        += fpu.o

# PM support

obj-$(CONFIG_PM)         += pm.o sleep.o

# CPU Frequency scaling support

obj-$(CONFIG_CPU_FREQ_JZ)       += cpufreq.o

