/*
 * Linux driver for Led Fan by Dream Cheeky (version 0)
 */
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

MODULE_AUTHOR("Thomas Venries, EPITA");
MODULE_DESCRIPTION("CheekyFan module, v0.1");
MODULE_LICENSE("GPL");

#define VENDOR_ID	0x0c45
#define PRODUCT_ID	0x7701

#define MAX_CHAR_NB	26

struct usb_cfan {
	struct usb_device	*udev;
	u64	displays[8][39];
	u64     cfg[8];
	u8	views_nb;
};

static struct usb_driver cfan_driver;
static struct usb_cfan *cfan;

/* Add the device to this driver's supported device list */
static struct usb_device_id id_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

/* Set effect config for a fan view */
static u64 set_config(const unsigned char id,
		      const unsigned char open_op,
		      const unsigned char close_op,
		      const unsigned char turn_op)
{
	/* Configuration:
	 * A0 10 <A><B> <C><N> 55 00 00 00
	 * [A] : OpenOption	[0 ; 5]
	 * [B] : CloseOption	[0 ; 5]
	 * [C] : TurnOption	(6 | c)
	 * [D] : Display ID	[0 ; 7]
	 * Warning: Reverse bytes sequence (stack behind)
	 */
	return 0x00000055000010A0 | (id << 24)
			| (open_op << 16)
			| (close_op << 20)
			| (turn_op << 25);
}

/* Send a '8 bytes' packet followed by an INTERRUPT_IN msg */
static int send_data(struct usb_device *udev, u64 data)
{
	char buf[8];
	int l = 0;
	int ret;

	memset(buf, 0, 8);
	buf[7] = 0x02;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			      0x09, 0x21, 0x200, 0x00, &data, 0x008,
			      10 * HZ);

	if (ret <= 0)
		return ret;

	usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
			  buf, 8, &l,
			  10 * HZ);

	return 0;
}

static int cfan_open(struct inode *i, struct file *f)
{
	  return 0;
}

static int cfan_release(struct inode *i, struct file *f)
{
	  return 0;
}

static void init_hello(struct usb_cfan *cfan)
{
	int i;

	for (i = 0; i < 15; ++i)
		cfan->displays[0][i] = 0xFFFFFFFFFFFFFFFF;

	cfan->displays[0][15] = 0x000000000FFFFF83;
	cfan->displays[0][16] = 0xFF7DFF7DFF7DFF83;
	cfan->displays[0][17] = 0xFFFFFFFDFFFDFFFD;
	cfan->displays[0][18] = 0xFFFDFF01FFFFFFFD;
	cfan->displays[0][19] = 0xFFFDFFFDFFFDFF01;
	cfan->displays[0][20] = 0xFFFFFF7DFF6DFF6D;
	cfan->displays[0][21] = 0xFF6DFF01FFFFFF01;
	cfan->displays[0][22] = 0xFFEFFFEFFFEFFF00;

	for (i = 23; i < 39; ++i)
		cfan->displays[0][i] = 0xFFFFFFFFFFFFFFFF;
}

static ssize_t cfan_write(struct file *f,
			const char __user *buffer,
			size_t cnt,
			loff_t *off)
{
	if (!cfan) {
		dev_warn(&cfan->udev->dev,
			 "cfan: %s() : cfan is NULL !\n", __func__);
		return -1;
	}

	if (cnt > MAX_CHAR_NB)
		return -1;

	init_hello(cfan);

	send_data(cfan->udev, set_config(0, 0, 0, 0));

	int i;

	for (i = 0; i < 39; ++i)
		send_data(cfan->udev, cfan->displays[0][i]);

	return 1;
}

const struct file_operations cfan_fops = {
	.open    = cfan_open,
	.release = cfan_release,
	.write = cfan_write,
};

static struct usb_class_driver cfan_class_driver = {
	.name = "usb/cfan0",
	.fops = &cfan_fops,
	.minor_base = 0
};

static int cfan_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	int ret;

	cfan = kmalloc(sizeof(*cfan), GFP_KERNEL);

	if (!cfan)
		return -ENOMEM;

	memset(cfan, 0x00, sizeof(*cfan));

	/* Increment the reference count of the usb device structure */
	cfan->udev = usb_get_dev(udev);

	/* Increment the ref counter */
	usb_set_intfdata(interface, cfan);

	pr_info("cfan:%s: USB Cheeky Fan has been connected\n", __func__);
	pr_info("cfan:%s: [devnum=%d;bus_id=%d;devid=%d]\n",
		__func__,
		cfan->udev->devnum,
		cfan->udev->bus->busnum,
		cfan->udev->dev.id);

	ret = usb_register_dev(interface, &cfan_class_driver);

	if (ret < 0) {
		pr_info("cfan: %s(): unable to register the device.\n",
		       __func__);
		return ret;
	}


	return 0;
}

static void cfan_disconnect(struct usb_interface *interface)
{
	struct usb_cfan *dev;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	usb_deregister_dev(interface, &cfan_class_driver);
	kfree(dev);

	pr_info("cfan: %s(): USB Cheeky Fan has been disconnected.\n",
		__func__);
}

static struct usb_driver cfan_driver = {
	.name       =	"cfan",
	.probe      =	cfan_probe,
	.disconnect =	cfan_disconnect,
	.id_table   =	id_table,
};

static int __init usb_cfan_init(void)
{
	int retval = 0;
	/* Register the USB driver. */
	retval = usb_register(&cfan_driver);
	if (retval < 0) {
		pr_info("cfan: %s(): unable to register USB driver.\n",
		       __func__);
		return retval;
	}
	return 0;
}

static void __exit usb_cfan_exit(void)
{
	/* Un-register this USB driver. */
	usb_deregister(&cfan_driver);
}

module_init(usb_cfan_init);
module_exit(usb_cfan_exit);
