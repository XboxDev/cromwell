#include "../usb_wrapper.h"


unsigned int last_remote_key;
unsigned int last_remote_keydir;
unsigned int current_remote_key;
unsigned int current_remote_keydir;
unsigned int Last_remote_keystroke;

struct usb_kbd {
	struct urb *urb;
	/*
	struct input_dev dev;
	struct usb_device *usbdev;
	unsigned char new[8];
	unsigned char old[8];
	struct urb irq, led;
	struct usb_ctrlrequest dr;
	unsigned char leds, newleds;
	char name[128];
	int open;
	*/
};

static void usb_kbd__irq(struct urb *urb, struct pt_regs *regs)
{
	
}

static int usb_kbd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	
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
