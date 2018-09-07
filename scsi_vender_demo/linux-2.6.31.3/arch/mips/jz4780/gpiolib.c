
/* arch/mips/jz4780/gpiolib.c
 *
 *	hlguo <hlguo@ingenic.cn>
 *
 * jz4780 GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/jzsoc.h>
#include <asm/gpio.h>

//#include <linux/gpio.h>

struct jz4780_gpio_chip {
	struct gpio_chip	chip;
	void __iomem		*base;
};

static inline struct jz4780_gpio_chip *to_jz4780_chip(struct gpio_chip *gpc)
{
	return container_of(gpc, struct jz4780_gpio_chip, chip);
}

static void jz4780_gpiolib_set(struct gpio_chip *chip,
				unsigned offset, int value)
{
       	unsigned long flags;
	unsigned int n = 0;

	n = chip->base +offset;
	local_irq_save(flags);
	if (value) {
		__gpio_as_output1(n);
	} else {
		__gpio_as_output0(n);
	}
	local_irq_restore(flags);
}

static int jz4780_gpiolib_get(struct gpio_chip *chip, unsigned offset)
{
       	unsigned long flags;
	unsigned int n = 0;
	int state = -1;

	n = chip->base +offset;
	local_irq_save(flags);
	state = __gpio_get_pin(n);
	local_irq_restore(flags);
	return state;

}

static int jz4780_gpiolib_input(struct gpio_chip *chip, unsigned offset)
{
       	unsigned long flags;
	unsigned int n = 0;

	n = chip->base +offset;
	local_irq_save(flags);
	__gpio_as_input(n);
	local_irq_restore(flags);
	return 0;
}

static int jz4780_gpiolib_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	unsigned long flags;
	unsigned int n = 0;

	n = chip->base +offset;
	local_irq_save(flags);
	if (value) {
		__gpio_as_output1(n);
	} else {
		__gpio_as_output0(n);
	}
	local_irq_restore(flags);
	return 0;
}

static int jz4780_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static void jz4780_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	return;
}

static struct jz4780_gpio_chip jz4780_gpios[] = {
	[0] = {
		.base	= (unsigned *)GPIO_BASEA,
		.chip	= {
			.base			= 0*32,
			.owner			= THIS_MODULE,
			.label			= "GPIOA",
			.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
	[1] = {
		.base	= (unsigned *)GPIO_BASEB,
		.chip	= {
			.base			= 1*32,
			.owner			= THIS_MODULE,
			.label			= "GPIOB",
		       	.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
	[2] = {
		.base	= (unsigned *)GPIO_BASEC,
		.chip	= {
			.base			= 2*32,
			.owner			= THIS_MODULE,
			.label			= "GPIOC",
		       	.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
	[3] = {
		.base	= (unsigned *)GPIO_BASED,
		.chip	= {
			.base			= 3*32,
			.owner			= THIS_MODULE,
			.label			= "GPIOD",
		       	.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
	[4] = {
		.base	= (unsigned *)GPIO_BASEE,
		.chip	= {
			.base			= 4*32,
			.label			= "GPIOE",
			.owner			= THIS_MODULE,
		       	.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
	[5] = {
		.base	= (unsigned *)GPIO_BASEF,
		.chip	= {
			.base			= 5*32,
			.owner			= THIS_MODULE,
			.label			= "GPIOF",
		       	.ngpio			= 32,
			.direction_input	= jz4780_gpiolib_input,
			.direction_output	= jz4780_gpiolib_output,
			.set			= jz4780_gpiolib_set,
			.get			= jz4780_gpiolib_get,
			.request		= jz4780_gpio_request,
			.free			= jz4780_gpio_free,
		},
	},
};

static __init int jz4780_gpiolib_init(void)
{
	struct jz4780_gpio_chip *chip = jz4780_gpios;
	int gpn;

	for (gpn = 0; gpn < ARRAY_SIZE(jz4780_gpios); gpn++, chip++)
		gpiochip_add(&chip->chip);

	return 0;
}

int gpio_to_irq(unsigned gpio) {
	return gpio + IRQ_GPIO_0;
}
//EXPORT_SYMBOL(gpio_to_irq);

arch_initcall(jz4780_gpiolib_init);
