#include "../usb_wrapper.h"


unsigned int last_remote_key;
unsigned int last_remote_keydir;
unsigned int current_remote_key;
unsigned int current_remote_keydir;
unsigned int Last_remote_keystroke;

struct usb_kbd_info {
	struct urb *urb;
	unsigned char kbd_pkt[8];
	unsigned char old[8];

	/*
	struct input_dev dev;
	struct usb_device *usbdev;
	struct urb irq, led;
	struct usb_ctrlrequest dr;
	unsigned char leds, newleds;
	char name[128];
	int open;
	*/
};

static void usb_kbd_irq(struct urb *urb, struct pt_regs *regs)
{
	struct usb_kbd_info *kbd = urb->context;
	int i;

	if (urb->status) return;
	
	memcpy(kbd->kbd_pkt, urb->transfer_buffer, 8);
	
	usb_submit_urb(urb,GFP_ATOMIC);
		
}

static int usb_kbd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct urb *urb;
	struct usb_device *udev = interface_to_usbdev (intf);
	struct usb_endpoint_descriptor *ep_irq_in;
	struct usb_endpoint_descriptor *ep_irq_out;
	struct usb_kbd_info *usbk;

	int i, pipe, maxp;
	char *buf;

	usbk=(struct usb_kbd_info *)kmalloc(sizeof(struct usb_kbd_info),0);
	if (!usbk) return -1;

	urb=usb_alloc_urb(0,0);
	if (!urb) return -1;

	usbk->urb=urb;

	ep_irq_in = &intf->altsetting[0].endpoint[0].desc;
	usb_fill_int_urb(urb, udev,
                         usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
                         usbk->kbd_pkt, 8, usb_kbd_irq,
                         usbk, 8);

	usb_submit_urb(urb,GFP_ATOMIC);
	usb_set_intfdata(intf,usbk);

	usbprintk("USB Keyboard Connected\n");	
}


static void usb_kbd_disconnect(struct usb_interface *intf)
{

}

static struct usb_device_id usb_kbd_id_table [] = {
	{ USB_INTERFACE_INFO(3, 1, 1) },
	{ }						/* Terminating entry */
};


static struct usb_driver usb_kbd_driver = {
	.owner =		THIS_MODULE,
	.name =			"keyboard",
	.probe =		usb_kbd_probe,
	.disconnect =		usb_kbd_disconnect,
	.id_table =		usb_kbd_id_table,
};

void UsbKeyBoardInit(void)
{

	//current_remote_key=0;
	//sbprintk("Keyboard probe %p ",xremote_probe);
	if (usb_register(&usb_kbd_driver) < 0) {
		err("Unable to register Keyboard driver");
		return;
	}       
}

void UsbKeyBoardRemove(void) {
	usb_deregister(&usb_kbd_driver);
}
