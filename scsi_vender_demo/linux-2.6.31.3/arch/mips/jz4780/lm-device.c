#include <linux/device.h>
#include <linux/lm.h>
#include <linux/resource.h>
#include <asm/jzsoc.h>

#define IRQ_DWC_OTG	IRQ_OTG
#define DWC_OTG_BASE	UDC_BASE

static u64 dwc_otg_dmamask = ~(u32)0;
static struct lm_device dwc_otg_dev = {
	.dev = {
		.dma_mask               = &dwc_otg_dmamask,
                .coherent_dma_mask      = 0xffffffff,
	},
	.resource = {
		.start			= CPHYSADDR(DWC_OTG_BASE),
		.end			= CPHYSADDR(DWC_OTG_BASE) + 0x40000 - 1,
		.flags			= IORESOURCE_MEM,
	},

	.irq	= IRQ_DWC_OTG, 		
	.id	= 0,
};

static int __init  lm_dev_init(void)
{
	int ret;	

	ret = lm_device_register(&dwc_otg_dev);
	if (ret) {
		printk("register dwc otg device failed.\n");
		return ret;
	}

	printk("=================================>\n");
	printk("dwc otg device init suscessed.\n");
	printk("=================================>\n");

	return ret;
		
}

arch_initcall(lm_dev_init);


