/*
 * Simple XPAD driver for XBOX
 *
 * (c) 2003-07-04, Georg Acher (georg@acher.org)
 *
 * Inspired by linux/drivers/usb/input/xpad.c
 * by Marko Friedemann <mfr@bmx-chemnitz.de>
 *
 */



#include "../usb_wrapper.h"

/* Stores time and XPAD state */
struct xpad_data XPAD_current[4];
struct xpad_data XPAD_last[4];

struct xpad_info
{
	struct urb *urb;
	int num;
	unsigned char data[32];
};

int xpad_num=0;
/*------------------------------------------------------------------------*/ 
static void xpad_irq(struct urb *urb, struct pt_regs *regs)
{
	struct xpad_info *xpi = urb->context;
	unsigned char* data= urb->transfer_buffer;
	struct xpad_data *xp=&XPAD_current[xpi->num];
	struct xpad_data *xpo=&XPAD_last[xpi->num];
	
	
	if (xpi->num<0 || xpi->num>3)
		return;
	memcpy(xpo,xp,sizeof(struct xpad_data));
	
	xp->stick_left_x=(short) (((short)data[13] << 8) | data[12]);
	xp->stick_left_y=(short) (((short)data[15] << 8) | data[14]);
	xp->stick_right_x=(short) (((short)data[17] << 8) | data[16]);
	xp->stick_right_y=(short) (((short)data[19] << 8) | data[18]);
	xp->trig_left= data[10];
	xp->trig_right= data[11];
	xp->pad = data[2]&0xf;
	xp->state = (data[2]>>4)&0xf;
	xp->keys[0] = data[4]; // a
	xp->keys[1] = data[5]; // b
	xp->keys[2] = data[6]; // x
	xp->keys[3] = data[7]; // y
	xp->keys[4] = data[8]; // black
	xp->keys[5] = data[9]; // white
	xp->timestamp=jiffies; // FIXME: A more uniform flowing time would be better... 	
	usb_submit_urb(urb,GFP_ATOMIC);

}
/*------------------------------------------------------------------------*/ 
static int xpad_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct urb *urb;
	struct usb_device *udev = interface_to_usbdev (intf);
	struct usb_endpoint_descriptor *ep_irq_in;
	struct usb_endpoint_descriptor *ep_irq_out;
	struct xpad_info *xpi;
	

	xpi=kmalloc(sizeof(struct xpad_data),0);
	if (!xpi) return -1;

	urb=usb_alloc_urb(0,0);
	if (!urb) return -1;

	xpi->urb=urb;
	xpi->num=xpad_num;
	ep_irq_in = &intf->altsetting[0].endpoint[0].desc;
	usb_fill_int_urb(urb, udev,
                         usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
                         xpi->data, 32, xpad_irq,
                         xpi, 32);

	usb_submit_urb(urb,GFP_ATOMIC);

	usb_set_intfdata(intf,xpi);
	usbprintk("XPAD #%i connected\n",xpad_num);
	{
		// Brum Brum
		char data1[6]={0,6,0,120,0,120};
		char data2[6]={0,6,0,0,0,0};
		int dummy;			

		usb_bulk_msg(udev, usb_sndbulkpipe(udev,2),
			data1, 6, &dummy, 500);
		wait_ms(500);
		usb_bulk_msg(udev, usb_sndbulkpipe(udev,2),
			data2, 6, &dummy, 500);		
	}
	xpad_num++;
	return 0;
}
/*------------------------------------------------------------------------*/ 
static void xpad_disconnect(struct usb_interface *intf)
{
	struct xpad_info *xpi=usb_get_intfdata (intf);
	usbprintk("XPAD disconnected\n ");
	usb_unlink_urb(xpi->urb);
	usb_free_urb(xpi->urb);
	kfree(xpi);
	xpad_num--;
}
/*------------------------------------------------------------------------*/ 
static struct usb_device_id xpad_ids [] = {
	{ USB_DEVICE(0x045e, 0x202) },
	{ USB_DEVICE(0x045e, 0x285) },
	{ USB_DEVICE(0x05fd, 0x107a) },
        { USB_DEVICE(0x045e, 0x289) }, /* Controller S */
        { }                            /* Terminating entry */
};


static struct usb_driver xpad_driver = {
	.owner =	THIS_MODULE,
	.name =		"XPAD",
	.probe =	xpad_probe,
	.disconnect =	xpad_disconnect,
	.id_table =	xpad_ids,
};
/*------------------------------------------------------------------------*/ 
void XPADInit(void)
{
	int n;
	for(n=0;n<4;n++)
	{
		memset(XPAD_current,0, sizeof(struct xpad_data));
		memset(XPAD_last,0, sizeof(struct xpad_data));
	}
	usbprintk("XPAD probe %p ",xpad_probe);
	if (usb_register(&xpad_driver) < 0) {
		err("Unable to register XPAD driver");
		return;
	}       
}

