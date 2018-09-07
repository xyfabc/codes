#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <asm/atomic.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/poll.h>

#include <asm/jzsoc.h>

#include "api/api.h"
#include "util/log.h"

/* ioctl command */

#define JZ_HDMI_MAJOR 0	      /* 0: dyna alloc */
static int jz_hdmi_major = JZ_HDMI_MAJOR;

static struct class *jz_hdmi_class;
struct jz_hdmi_dev {
	struct cdev dev;
	atomic_t opened;
};

static struct jz_hdmi_dev hdmi_dev;

enum {
	API_INITIALIZE = 0,
	API_CONFIGURE,
	API_STANDBY,
	API_COREREAD,
	API_COREWRITE,
	API_EVENTENABLE,
	API_EVENTCLEAR,
	API_EVENTDISABLE,
	API_EDIDREAD,
	API_EDIDDTDCOUNT,
	API_EDIDDTD,
	API_EDIDHDMIVSDB,
	API_EDIDMONITORNAME,
	API_EDIDMONITORRANGELIMITS,
	API_EDIDSVDCOUNT,
	API_EDIDSVD,
	API_EDIDSADCOUNT,
	API_EDIDSAD,
	API_EDIDVIDEOCAPABILITYDATABLOCK,
	API_EDIDSPEAKERALLOCATIONDATABLOCK,
	API_EDIDCOLORIMETRYDATABLOCK,
	API_EDIDSUPPORTSBASICAUDIO,
	API_EDIDSUPPORTSUNDERSCAN,
	API_EDIDSUPPORTSYCC422,
	API_EDIDSUPPORTSYCC444,
	API_PACKETSAUDIOCONTENTPROTECTION,
	API_PACKETSISRC,
	API_PACKETSISRCSTATUS,
	API_PACKETSGAMUTMETADATA,
	API_PACKETSSTOPSENDACP,
	API_PACKETSSTOPSENDISRC,
	API_PACKETSSTOPSENDSPD,
	API_PACKETSSTOPSENDVSD,
	API_AVMUTE,
	API_AUDIOMUTE,
	API_AUDIODMAREQUESTADDRESS,
	API_AUDIODMAGETCURRENTBURSTLENGTH,
	API_AUDIODMAGETCURRENTOPERATIONADDRESS,
	API_AUDIODMASTARTREAD,
	API_AUDIODMASTOPREAD,
	API_HDCPENGAGED,
	API_HDCPAUTHENTICATIONSTATE,
	API_HDCPCIPHERSTATE,
	API_HDCPREVOCATIONSTATE,
	API_HDCPBYPASSENCRYPTION,
	API_HDCPDISABLEENCRYPTION,
	API_HDCPSRMUPDATE,
	API_HOTPLUGDETECTED,
	API_READCONFIG,
	API_GET_EDIDDONE,
	API_GET_HPDCONNECTED,
	API_GET_RECONFIG,
	API_SET_RECONFIG,
	API_PHY_I2CWRITE,
	API_ACCESS_INITIALIZE,
};

/***********************************************
 * globals
 **********************************************/
int volatile hpdConnected = FALSE;
int volatile reconfig = FALSE;
int volatile edidDone = FALSE;
/***********************************************
 * callbacks
 **********************************************/
static void edidCallback(void *param)
{
	LOG_NOTICE("EDID reading done");
	edidDone = TRUE;
}

static void hdcpKsvCallback(void *param)
{
	unsigned i = 0;
	u8 ksv_list[128] = {0};
	unsigned size = 0;
	buffer_t *temp_buffer = (buffer_t*)(param);
	size = temp_buffer->size;
	LOG_NOTICE("KSV list ready");
	for (i = 0; i < size; i++)
	{
		ksv_list[i] = temp_buffer->buffer[i];
		LOG_NOTICE3("ksv", i, ksv_list[i]);
	}
}

static void hpdCallback(void *param)
{
	u8 hpd = *((u8*)(param));
	hpdConnected = hpd;
	if (hpd != TRUE)
	{
		LOG_NOTICE("HPD LOST");
		if (!api_Standby())
		{
			LOG_ERROR2("API standby", error_Get());
		}
		if (!api_Initialize(0, 1, 2500, FALSE))
		{
			LOG_ERROR2("API initialize", error_Get());
		}
		reconfig = TRUE;
	}
	else
	{
		LOG_NOTICE("HPD ASSERTED");
		edidDone = FALSE;
		if (!api_EdidRead(edidCallback))
		{
			LOG_ERROR2("cannot read E-EDID", error_Get());
		}
	}
}

int hdmi_Initialize(u32 args[])
{
	u16 address = (u16)args[0];
	u8 dataEnablePolarity = (u8)args[1];
	u16 sfrClock = (u16)args[2];
	u8 force = (u8)args[3];

	return api_Initialize(address, dataEnablePolarity, sfrClock, force);
}

int hdmi_Configure(u32 args[])
{
	videoParams_t video;
	videoParams_t *pv;

	audioParams_t audio;
	audioParams_t *pa;

	productParams_t product;
	productParams_t *pp;

	hdcpParams_t hdcp;
	hdcpParams_t *ph;

	pv = (videoParams_t *)args[0];
	if (copy_from_user(&video, pv, sizeof(videoParams_t)))
		return -EFAULT;

	pa = (audioParams_t *)args[1];
	if (copy_from_user(&audio, pa, sizeof(audioParams_t)))
		return -EFAULT;

	pp = (productParams_t *)args[2];
	if (copy_from_user(&product, pp, sizeof(productParams_t)))
		return -EFAULT;

	ph = (hdcpParams_t *)args[3];
	if (copy_from_user(&hdcp, ph, sizeof(hdcpParams_t)))
		return -EFAULT;

	return api_Configure(&video, &audio, &product, &hdcp);
}

int hdmi_Standby(u32 args[])
{
	return api_Standby();
}

int hdmi_CoreRead(u32 args[])
{
	u16 addr = (u16)args[0];
	return api_CoreRead(addr);
}

int hdmi_CoreWrite(u32 args[])
{
	u8 data = (u8)args[0];
	u16 addr = (u16)args[1];

	api_CoreWrite(data, addr);

	return 0;
}

int hdmi_EventEnable(u32 args[])
{
	event_t idEvent = (event_t)args[0];
	//handler_t handler = (handler_t)args[1];
	int handler_id = (int)args[1];
	u8 oneshot = (u8)args[2];

	handler_t handler;
	if (handler_id == ('h' + 'p' + 'd'))
		handler = hpdCallback;
	else if (handler_id == ('h' + 'd' + 'c' + 'p' + 'K' + 's' + 'v'))
		handler = hdcpKsvCallback;
	else
		printk("=====>unknown EventEnable Handler\n");

	return api_EventEnable(idEvent, handler, oneshot);
}

int hdmi_EventClear(u32 args[])
{
	event_t idEvent = (event_t)args[0];

	return api_EventClear(idEvent);
}

int hdmi_EventDisable(u32 args[])
{
	event_t idEvent = (event_t)args[0];
	return api_EventDisable(idEvent);
}

int hdmi_EdidRead(u32 args[])
{
	//handler_t handler = (handler_t)args[0];

	return api_EdidRead(edidCallback);
}

int hdmi_EdidDtdCount(u32 args[])
{
	return api_EdidDtdCount();
}

int hdmi_EdidDtd(u32 args[])
{
	unsigned int n = (unsigned int)args[0];
	int ret;

	dtd_t dtd;
	dtd_t *pd;

	pd = (dtd_t *)args[1];

	ret = api_EdidDtd(n, &dtd);
	if (ret == TRUE)
		if (copy_to_user(pd, &dtd, sizeof(dtd_t)))
			return -EFAULT;

	return ret;
}

int hdmi_EdidHdmivsdb(u32 args[])
{
	hdmivsdb_t vsdb;
	hdmivsdb_t *pv;
	int ret;

	pv = (hdmivsdb_t *)args[0];

	ret = api_EdidHdmivsdb(&vsdb);
	if (ret == TRUE)
		if (copy_to_user(pv, &vsdb, sizeof(hdmivsdb_t)))
			return -EFAULT;
	return ret;
}

int hdmi_EdidMonitorName(u32 args[])
{
	char *name = (char *)args[0];
	unsigned int length = args[1];

	char *m_name = kmalloc(length+1, GFP_KERNEL);
	int ret;

	ret = api_EdidMonitorName(m_name, length);
	if (ret == TRUE)
		if (copy_to_user(name, &m_name, length)) {
			kfree(m_name);
			return -EFAULT;
		}

	kfree(m_name);
	return ret;
}

int hdmi_EdidMonitorRangeLimits(u32 args[])
{
	monitorRangeLimits_t limits;
	monitorRangeLimits_t *pl = (monitorRangeLimits_t *)args[0];

	int ret = api_EdidMonitorRangeLimits(&limits);
	if (ret == TRUE) {
		if (copy_to_user(pl, &limits, sizeof(monitorRangeLimits_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidSvdCount(u32 args[])
{
	return api_EdidSvdCount();
}

int hdmi_EdidSvd(u32 args[])
{
	unsigned int n = args[0];
	shortVideoDesc_t svd;
	shortVideoDesc_t *ps = (shortVideoDesc_t *)args[1];

	int ret = api_EdidSvd(n, &svd);
	if (ret == TRUE) {
		if (copy_to_user(ps, &svd, sizeof(shortVideoDesc_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidSadCount(u32 args[])
{
	return api_EdidSadCount();
}

int hdmi_EdidSad(u32 args[])
{
	unsigned int n = args[0];
	shortAudioDesc_t sad;
	shortAudioDesc_t *ps = (shortAudioDesc_t *)args[1];

	int ret = api_EdidSad(n, &sad);
	if (ret == TRUE) {
		if (copy_to_user(ps, &sad, sizeof(shortAudioDesc_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidVideoCapabilityDataBlock(u32 args[])
{
	videoCapabilityDataBlock_t capability;
	videoCapabilityDataBlock_t *pc = (videoCapabilityDataBlock_t *)args[0];

	int ret = api_EdidVideoCapabilityDataBlock(&capability);
	if (ret == TRUE) {
		if (copy_to_user(pc, &capability, sizeof(videoCapabilityDataBlock_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidSpeakerAllocationDataBlock(u32 args[])
{
	speakerAllocationDataBlock_t allocation;
	speakerAllocationDataBlock_t *pa = (speakerAllocationDataBlock_t *)args[0];

	int ret = api_EdidSpeakerAllocationDataBlock(&allocation);;
	if (ret == TRUE) {
		if (copy_to_user(pa, &allocation, sizeof(speakerAllocationDataBlock_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidColorimetryDataBlock(u32 args[])
{
	colorimetryDataBlock_t colorimetry;
	colorimetryDataBlock_t *pc = (colorimetryDataBlock_t *)args[0];

	int ret = api_EdidColorimetryDataBlock(&colorimetry);
	if (ret == TRUE) {
		if (copy_to_user(pc, &colorimetry, sizeof(colorimetryDataBlock_t)))
			return -EFAULT;
	}

	return ret;
}

int hdmi_EdidSupportsBasicAudio(u32 args[])
{
	return api_EdidSupportsBasicAudio();
}

int hdmi_EdidSupportsUnderscan(u32 args[])
{
	return api_EdidSupportsUnderscan();
}

int hdmi_EdidSupportsYcc422(u32 args[])
{
	return api_EdidSupportsYcc422();
}

int hdmi_EdidSupportsYcc444(u32 args[])
{
	return api_EdidSupportsYcc444();
}

int hdmi_PacketsAudioContentProtection(u32 args[])
{
	u8 type = (u8)args[0];
	u8 *fields = (u8 *)args[1];
	u8 length = (u8)args[2];
	u8 autoSend = (u8)args[3];

	u8 *m_fields = kmalloc(length + 1, GFP_KERNEL);
	if (copy_from_user(m_fields, fields, length))
		goto out;

	api_PacketsAudioContentProtection(type, m_fields, length, autoSend);

out:
	kfree(m_fields);
	return 0;
}

int hdmi_PacketsIsrc(u32 args[])
{
	u8 initStatus = (u8)args[0];
	u8 *codes = (u8 *)args[1];
	u8 length = (u8)args[2];
	u8 autoSend = (u8)args[3];

	u8 *m_codes = kmalloc(length + 1, GFP_KERNEL);
	if (copy_from_user(m_codes, codes, length))
		goto out;

	api_PacketsIsrc(initStatus, m_codes, length, autoSend);

out:
	kfree(m_codes);
	return 0;
}

int hdmi_PacketsIsrcStatus(u32 args[])
{
	u8 status = (u8)args[0];

	api_PacketsIsrcStatus(status);

	return 0;
}

int hdmi_PacketsGamutMetadata(u32 args[])
{
	u8 *gdbContent = (u8 *)args[0];
	u8 length = (u8)args[1];

	u8 *m_content = kmalloc(length + 1, GFP_KERNEL);
	if (copy_from_user(m_content, gdbContent, length))
		goto out;

	api_PacketsGamutMetadata(m_content, length);

out:
	kfree(m_content);

	return 0;
}

int hdmi_PacketsStopSendAcp(u32 args[])
{
	api_PacketsStopSendAcp();

	return 0;
}

int hdmi_PacketsStopSendIsrc(u32 args[])
{
	api_PacketsStopSendIsrc();

	return 0;
}

int hdmi_PacketsStopSendSpd(u32 args[])
{
	api_PacketsStopSendSpd();

	return 0;
}

int hdmi_PacketsStopSendVsd(u32 args[])
{
	api_PacketsStopSendVsd();

	return 0;
}

int hdmi_AvMute(u32 args[])
{
	int enable = (int)args[0];
	api_AvMute(enable);

	return 0;
}

int hdmi_AudioMute(u32 args[])
{
	int enable = (int)args[0];
	api_AudioMute(enable);

	return 0;
}

int hdmi_AudioDmaRequestAddress(u32 args[])
{
	u32 startAddress = args[0];
	u32 stopAddress = args[1];

	api_AudioDmaRequestAddress(startAddress, stopAddress);

	return 0;
}
int hdmi_AudioDmaGetCurrentBurstLength(u32 args[])
{
	return api_AudioDmaGetCurrentBurstLength();
}

int hdmi_AudioDmaGetCurrentOperationAddress(u32 args[])
{
	return (int)api_AudioDmaGetCurrentOperationAddress();
}

int hdmi_AudioDmaStartRead(u32 args[])
{
	api_AudioDmaStartRead();

	return 0;
}
int hdmi_AudioDmaStopRead(u32 args[])
{
	api_AudioDmaStopRead();

	return 0;
}

int hdmi_HdcpEngaged(u32 args[])
{
	return api_HdcpEngaged();
}

int hdmi_HdcpAuthenticationState(u32 args[])
{
	return api_HdcpAuthenticationState();
}

int hdmi_HdcpCipherState(u32 args[])
{
	return api_HdcpCipherState();
}

int hdmi_HdcpRevocationState(u32 args[])
{
	return api_HdcpRevocationState();
}

int hdmi_HdcpBypassEncryption(u32 args[])
{
	int bypass = (int)args[0];

	return api_HdcpBypassEncryption(bypass);
}

int hdmi_HdcpDisableEncryption(u32 args[])
{
	int disable = (int)args[0];

	return api_HdcpDisableEncryption(disable);
}

int hdmi_HdcpSrmUpdate(u32 args[])
{
	/* args: const u8 * data */
	/* TODO: what is the lengh of the data */
	return 0;
}

int hdmi_HotPlugDetected(u32 args[])
{
	return api_HotPlugDetected();
}

int hdmi_ReadConfig(u32 args[])
{
	return api_ReadConfig();
}

int hdmi_get_edidDone(u32 args[]) {
	return edidDone;
}

int hdmi_get_hpdConnected(u32 args[]) {
	return hpdConnected;
}

int hdmi_get_reconfig(u32 args[]) {
	return reconfig;
}

int hdmi_set_reconfig(u32 args[]) {
	reconfig = (int)args[0];

	return 0;
}

int hdmi_phy_I2cWrite(u32 args[]) {
	u16 baseAddr = (u16)args[0];
	u16 data = (u16)args[1];
	u8 addr = (u8)args[2];

	return phy_I2cWrite(baseAddr, data, addr);
}

int hdmi_access_Initialize(u32 args[]) {
	u8 *baseAddr = (u8 *)args[0];

	return access_Initialize(baseAddr);
}

typedef int (*hdmi_api_func)(u32 args[]);

hdmi_api_func api_funcs[] = {
	[API_INITIALIZE] =   hdmi_Initialize ,
	[API_CONFIGURE] =   hdmi_Configure ,
	[API_STANDBY] =   hdmi_Standby ,
	[API_COREREAD] =   hdmi_CoreRead ,
	[API_COREWRITE] =   hdmi_CoreWrite ,
	[API_EVENTENABLE] =   hdmi_EventEnable ,
	[API_EVENTCLEAR] =   hdmi_EventClear ,
	[API_EVENTDISABLE] =   hdmi_EventDisable ,
	[API_EDIDREAD] =   hdmi_EdidRead ,
	[API_EDIDDTDCOUNT] =   hdmi_EdidDtdCount ,
	[API_EDIDDTD] =   hdmi_EdidDtd ,
	[API_EDIDHDMIVSDB] =   hdmi_EdidHdmivsdb ,
	[API_EDIDMONITORNAME] =   hdmi_EdidMonitorName ,
	[API_EDIDMONITORRANGELIMITS] =   hdmi_EdidMonitorRangeLimits ,
	[API_EDIDSVDCOUNT] =   hdmi_EdidSvdCount ,
	[API_EDIDSVD] =   hdmi_EdidSvd ,
	[API_EDIDSADCOUNT] =   hdmi_EdidSadCount ,
	[API_EDIDSAD] =   hdmi_EdidSad ,
	[API_EDIDVIDEOCAPABILITYDATABLOCK] =   hdmi_EdidVideoCapabilityDataBlock ,
	[API_EDIDSPEAKERALLOCATIONDATABLOCK] =   hdmi_EdidSpeakerAllocationDataBlock ,
	[API_EDIDCOLORIMETRYDATABLOCK] =   hdmi_EdidColorimetryDataBlock ,
	[API_EDIDSUPPORTSBASICAUDIO] =   hdmi_EdidSupportsBasicAudio ,
	[API_EDIDSUPPORTSUNDERSCAN] =   hdmi_EdidSupportsUnderscan ,
	[API_EDIDSUPPORTSYCC422] =   hdmi_EdidSupportsYcc422 ,
	[API_EDIDSUPPORTSYCC444] =   hdmi_EdidSupportsYcc444 ,
	[API_PACKETSAUDIOCONTENTPROTECTION] =   hdmi_PacketsAudioContentProtection ,
	[API_PACKETSISRC] =   hdmi_PacketsIsrc ,
	[API_PACKETSISRCSTATUS] =   hdmi_PacketsIsrcStatus ,
	[API_PACKETSGAMUTMETADATA] =   hdmi_PacketsGamutMetadata ,
	[API_PACKETSSTOPSENDACP] =   hdmi_PacketsStopSendAcp ,
	[API_PACKETSSTOPSENDISRC] =   hdmi_PacketsStopSendIsrc ,
	[API_PACKETSSTOPSENDSPD] =   hdmi_PacketsStopSendSpd ,
	[API_PACKETSSTOPSENDVSD] =   hdmi_PacketsStopSendVsd ,
	[API_AVMUTE] =   hdmi_AvMute ,
	[API_AUDIOMUTE] =   hdmi_AudioMute ,
	[API_AUDIODMAREQUESTADDRESS] =   hdmi_AudioDmaRequestAddress ,
	[API_AUDIODMAGETCURRENTBURSTLENGTH] =   hdmi_AudioDmaGetCurrentBurstLength ,
	[API_AUDIODMAGETCURRENTOPERATIONADDRESS] =   hdmi_AudioDmaGetCurrentOperationAddress ,
	[API_AUDIODMASTARTREAD] =   hdmi_AudioDmaStartRead ,
	[API_AUDIODMASTOPREAD] =   hdmi_AudioDmaStopRead ,
	[API_HDCPENGAGED] =   hdmi_HdcpEngaged ,
	[API_HDCPAUTHENTICATIONSTATE] =   hdmi_HdcpAuthenticationState ,
	[API_HDCPCIPHERSTATE] =   hdmi_HdcpCipherState ,
	[API_HDCPREVOCATIONSTATE] =   hdmi_HdcpRevocationState ,
	[API_HDCPBYPASSENCRYPTION] =   hdmi_HdcpBypassEncryption ,
	[API_HDCPDISABLEENCRYPTION] =   hdmi_HdcpDisableEncryption ,
	[API_HDCPSRMUPDATE] =   hdmi_HdcpSrmUpdate ,
	[API_HOTPLUGDETECTED] =   hdmi_HotPlugDetected ,
	[API_READCONFIG] =   hdmi_ReadConfig ,
	[API_GET_EDIDDONE] =   hdmi_get_edidDone ,
	[API_GET_HPDCONNECTED] =   hdmi_get_hpdConnected ,
	[API_GET_RECONFIG] =   hdmi_get_reconfig ,
	[API_SET_RECONFIG] =   hdmi_set_reconfig ,
	[API_PHY_I2CWRITE] = hdmi_phy_I2cWrite,
	[API_ACCESS_INITIALIZE] = hdmi_access_Initialize,
};

static int hdmi_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	return api_funcs[cmd]( (u32 *)arg);
}

static int hdmi_open(struct inode * inode, struct file * filp)
{
	if (! atomic_dec_and_test (&hdmi_dev.opened)) {
		atomic_inc(&hdmi_dev.opened);
		return -EBUSY; /* already open */
	}

	//filp->private_data = &hdmi_dev;
	printk("Ingenic Onchip HDMI opened\n");
	return 0;
}

static int hdmi_close(struct inode * inode, struct file * filp)
{
	atomic_inc(&hdmi_dev.opened); /* release the device */
	return 0;
}

static struct file_operations jz_hdmi_fops = {
	.owner = THIS_MODULE,
	.open = hdmi_open,
	.release = hdmi_close,
	.ioctl = hdmi_ioctl,
};

static int __init hdmi_init(void)
{
	int result;
	dev_t devno = 0;

	if (jz_hdmi_major) {
		devno = MKDEV(jz_hdmi_major, 0);
		result = register_chrdev_region(devno, 1, "hdmi");
	} else {
		result = alloc_chrdev_region(&devno, 0, 1, "hdmi");
		jz_hdmi_major = MAJOR(devno);
	}

	if (result < 0)
		return result;

	memset(&hdmi_dev, 0, sizeof(struct jz_hdmi_dev));
	atomic_set(&hdmi_dev.opened, 1);
	cdev_init(&hdmi_dev.dev, &jz_hdmi_fops);
	hdmi_dev.dev.owner = THIS_MODULE;

	result = cdev_add(&hdmi_dev.dev, devno, 1);
	if (result)
		goto fail_cdev;

	jz_hdmi_class = class_create(THIS_MODULE, "hdmi");
	if (IS_ERR(jz_hdmi_class)) {
		result =  PTR_ERR(jz_hdmi_class);
		goto fail_class;
	}

	device_create(jz_hdmi_class, NULL, devno, NULL, "hdmi");
	return 0;

fail_class:
	cdev_del(&hdmi_dev.dev);
fail_cdev:
	unregister_chrdev_region(devno, 1);
	return result;
}

static void __exit hdmi_exit(void)
{
	dev_t devno = MKDEV(jz_hdmi_major, 0);

	cdev_del(&hdmi_dev.dev);

	device_destroy(jz_hdmi_class, devno);
	class_destroy(jz_hdmi_class);

	unregister_chrdev_region(MKDEV(jz_hdmi_major, 0), 1);
}

module_init(hdmi_init);
module_exit(hdmi_exit);

MODULE_AUTHOR("Lutts Wolf<slcao@ingenic.cn>");
MODULE_LICENSE("Dual BSD/GPL");
