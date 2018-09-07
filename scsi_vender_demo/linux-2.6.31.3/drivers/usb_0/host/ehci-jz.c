/*
 * EHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for AMD Alchemy Au1xxx
 *
 * Based on "ehci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 *
 * Modified for AMD Alchemy Au1200 EHC
 *  by K.Boge <karsten.boge@amd.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/platform_device.h>

#include <asm/jzsoc.h>

extern int usb_disabled(void);

static void jz_start_ehc(void)
{
	printk(KERN_ERR "+++++++++++%s() Line:%d \n", __func__, __LINE__);	//twxie_debug
#if defined(CONFIG_SOC_JZ4780) || defined(CONFIG_SOC_JZ4775)
	/* enable clock gate */
	REG_CPM_CLKGR0 &= ~(1 << 24);

	/* UHC clock source is OTG_PHY */
	REG_CPM_UHCCDR |= UHCCDR_UHCS_MASK;

	REG_CPM_USBPCR &= ~USBPCR_OTG_DISABLE;	//twxie
	
	/* The PLL uses CLKCORE as reference */
	REG_CPM_USBPCR1 |= USBPCR1_REFCLKSEL_MASK;
	
	/* 12M clock frequency */
	REG_CPM_USBPCR1 &= ~USBPCR1_REFCLKDIV_MASK;

	/* port1(uhc) hasn't forced to entered SUSPEND mode */
	__cpm_enable_uhc_phy();
	
	/* The pull-down resistance on D-/D+ of port1 */
	REG_CPM_USBPCR1 |= USBPCR1_DMPD1 | USBPCR1_DPPD1;

	/* select utmi data bus width of port1 to 16bit/30M */
	REG_CPM_USBPCR1 |= USBPCR1_WORD_IF1;
	//REG_CPM_USBPCR1 &= ~USBPCR1_WORD_IF1;
	printk("%s:%d  REG_CPM_USBPCR1 = %#08x\n", __func__, __LINE__, REG_CPM_USBPCR1);

	/* select utmi data bus width of controller to 16bit */
	REG32(0xb34900b0) |= (1 << 6);
	//REG32(0xb34900b0) &= ~(1 << 6);
	printk("%s:%d  REG32(0xb34900b0) = %#08x\n", __func__, __LINE__, REG32(0xb34900b0));

#if 0
	/* Port1 reset */
	REG_CPM_USBPCR1 |= USBPCR1_PORT1_RST;
        udelay(300);
	REG_CPM_USBPCR1 &= USBPCR1_PORT1_RST;
        udelay(300);
#endif

	/* phy reset */
        REG_CPM_USBPCR |= USBPCR_POR;
        udelay(30);
        REG_CPM_USBPCR &= ~USBPCR_POR;
        udelay(300);

#if 1
	/* UHC soft reset */
	REG_CPM_SRBC |= 1 << 14;
        udelay(300);
	REG_CPM_SRBC &= ~(1 << 14);
        udelay(300);
	printk("/* UHC soft reset */\n");
#endif

#endif
}

static void jz_stop_ehc(void)
{
#ifdef CONFIG_SOC_JZ4780
	REG_CPM_OPCR &= ~OPCR_OTGPHY1_ENABLE;
#endif
}

static const struct hc_driver ehci_jz_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "JZ EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static int ehci_hcd_jz_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	int ret;

	if (usb_disabled())
		return -ENODEV;

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}
	hcd = usb_create_hcd(&ehci_jz_hc_driver, &pdev->dev, "JZ_EHCI");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		ret = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		ret = -ENOMEM;
		goto err2;
	}

	jz_start_ehc();

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = usb_add_hcd(hcd, pdev->resource[1].start,
			  IRQF_DISABLED | IRQF_SHARED);


	if (ret == 0) {
		platform_set_drvdata(pdev, hcd);
		return ret;
	}

	jz_stop_ehc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return ret;
}

static int ehci_hcd_jz_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	jz_stop_ehc();
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ehci_hcd_jz_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc;

	return 0;
	rc = 0;

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW) {
		ehci_halt(ehci);
		ehci_reset(ehci);
	}

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	jz_stop_ehc();

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	// could save FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	return rc;
}


static int ehci_hcd_jz_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	jz_start_ehc();

	// maybe restore FLADJ

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	/* If CF is still set, we maintained PCI Vaux power.
	 * Just undo the effect of ehci_pci_suspend().
	 */
	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}

#else
#define ehci_hcd_jz_drv_suspend NULL
#define ehci_hcd_jz_drv_resume NULL
#endif

static struct platform_driver ehci_hcd_jz_driver = {
	.probe		= ehci_hcd_jz_drv_probe,
	.remove		= ehci_hcd_jz_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_jz_drv_suspend,
	.resume		= ehci_hcd_jz_drv_resume,
	.driver = {
		.name	= "jz-ehci",
		.owner	= THIS_MODULE,
	}
};

MODULE_ALIAS("platform:jz-ehci");
