
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-02-13 andy@warmcat.com  Structural improvements as I learn more about the OHCI architecture

*/

#include "boot.h"
#include "config.h"
#include "BootUsbOhci.h"
#include <linux/types.h>

#define OHCI_CONTROL_INIT (OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_IE | OHCI_CTRL_PLE

/* AMD-756 (D2 rev) reports corrupt register contents in some cases.
 * The erratum (#4) description is incorrect.  AMD's workaround waits
 * till some bits (mostly reserved) are clear; ok for all revs.
 */
#define read_roothub(hc, register, mask) ({ \
        __u32 temp = readl (&hc->regs->roothub.register); \
        if (hc->flags & OHCI_QUIRK_AMD756) \
                while (temp & mask) \
		temp = readl (&hc->regs->roothub.register); \
        temp; })

static __u32 roothub_a (struct ohci *hc)
        { return read_roothub (hc, a, 0xfc0fe000); }
/*
static inline __u32 roothub_b (struct ohci *hc)
        { return readl (&hc->regs->roothub.b); }
static inline __u32 roothub_status (struct ohci *hc)
        { return readl (&hc->regs->roothub.status); }
static __u32 roothub_portstatus (struct ohci *hc, int i)
	{ return read_roothub (hc, portstatus [i], 0xffe0fce0); }
*/		

const LIST_ENTRY listentryEmpty = { NULL, NULL };

/*
struct usb_device *usb_alloc_dev(struct usb_device *parent, struct usb_bus *bus)
{
	
}
*/

void BootUsbInit(ohci_t * ohci, const char *szName, void *regbase) {

	__u32 mask;
	unsigned int fminterval;
//        struct usb_device  * usb_dev;
//        struct ohci_device * dev;
	void *hcca;
	
	memset(ohci,0,sizeof(ohci_t));
	
	_strncpy(ohci->szName, szName, sizeof(ohci->szName)-1);

	(void *) ohci->regs = (volatile void *) regbase;
	
	// init the hardware

	// what state is the hardware in at the moment?  (usually reset state)
	hcca = malloc(0x10000+0x100);
	memset(hcca,0,0x10000+0x100);	
	ohci->hcca = (volatile struct ohci_hcca *) ((DWORD)(hcca + 0x100) & 0xffffff00);
	
	
	memset((struct ohci_hcca *)ohci->hcca,0,sizeof(struct ohci_hcca));

	ohci->hcca_dma = (__u32)ohci->hcca;

        ohci->disabled = 1;

        /* Tell the controller where the control and bulk lists are
         * The lists are empty now. */
	writel (0, &ohci->regs->ed_controlhead);
        writel (0, &ohci->regs->ed_bulkhead);
        writel (ohci->hcca_dma, &ohci->regs->hcca); /* a reset clears this */

        fminterval = 0x2edf;
        writel ((fminterval * 9) / 10, &ohci->regs->periodicstart);
        fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
        writel (fminterval, &ohci->regs->fminterval);
        writel (0x628, &ohci->regs->lsthresh);

	/* start controller operations */
        ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
        ohci->disabled = 0;
        writel (ohci->hc_control, &ohci->regs->control);

        /* Choose the interrupts we care about now, others later on demand */
        mask = OHCI_INTR_MIE | OHCI_INTR_UE | OHCI_INTR_WDH | OHCI_INTR_SO;
        writel (mask, &ohci->regs->intrenable);
        writel (mask, &ohci->regs->intrstatus);

	/* required for AMD-756 and some Mac platforms */
	writel ((roothub_a (ohci) | RH_A_NPS) & ~RH_A_PSM,
		&ohci->regs->roothub.a);

	Sleep(50000);

}


void BootUsbTurnOff(ohci_t * ohci)
{
	/*
	pusbcontroller->m_pusboperationalregisters->m_dwHcControl=
		((pusbcontroller->m_pusboperationalregisters->m_dwHcControl)&(~0xc0))|0x00;
	pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptDisable=0x8000007f; // kill all interrupts
	pusbcontroller->m_pusboperationalregisters->m_dwHcHCCA=(DWORD)0; // kill HCCA is in memory
	*/
}

void BootUsbDebugOut(ohci_t * ohci) {
	VIDEO_ATTR=0xffc8c800;
	VIDEO_CURSOR_POSX = 0;	
	VIDEO_CURSOR_POSY = 0;	
	printk(" revision 0x%08x\n",ohci->regs->revision);
	printk(" control 0x%08x\n",ohci->regs->control);
	printk(" cmdstatus 0x%08x\n",ohci->regs->cmdstatus);
	printk(" intrstatus 0x%08x\n",ohci->regs->intrstatus);
	printk(" intrenable 0x%08x\n",ohci->regs->intrenable);
	printk(" intrdisable 0x%08x\n",ohci->regs->intrdisable);
	printk(" ed_periodcurrent 0x%08x\n",ohci->regs->ed_periodcurrent);
	printk(" ed_controlhead 0x%08x\n",ohci->regs->ed_controlhead);
	printk(" ed_controlcurrent 0x%08x\n",ohci->regs->ed_controlcurrent);
	printk(" ed_bulkhead 0x%08x\n",ohci->regs->ed_bulkhead);
	printk(" ed_bulkcurrent 0x%08x\n",ohci->regs->ed_bulkcurrent);
	printk(" donehead 0x%08x\n",ohci->regs->donehead);
	printk(" fminterval 0x%08x\n",ohci->regs->fminterval);
	printk(" fmremaining 0x%08x\n",ohci->regs->fmremaining);
	printk(" periodicstart 0x%08x\n",ohci->regs->periodicstart);
	printk(" lsthresh 0x%08x\n",ohci->regs->lsthresh);
	printk(" ohci_roothub_regs.a 0x%08x\n",ohci->regs->roothub.a);
	printk(" ohci_roothub_regs.b 0x%08x\n",ohci->regs->roothub.b);
	printk(" ohci_roothub_regs.status 0x%08x\n",ohci->regs->roothub.status);
	VideoDumpAddressAndData(0, (BYTE *)ohci->regs->roothub.portstatus , MAX_ROOT_PORTS);
}

	// ISR calls through to here

void BootUsbInterrupt(ohci_t * ohci)
{
	printk("%s Interrupt (%u)\n", ohci->szName,ohci->Interrupts);

	if(ohci->regs->intrstatus&1) {
		printk("%s Interrupt : USB Scheduling Overrun\n", ohci->szName);
		ohci->regs->intrstatus=1;
	}
	if(ohci->regs->intrstatus&2) {
		printk("%s Interrupt : USB Writeback Done Head\n", ohci->szName);
		ohci->regs->intrstatus=2;
	}
	if(ohci->regs->intrstatus&4) {
		printk("%s Interrupt : USB Start of Frame\n", ohci->szName);
		ohci->regs->intrstatus=4;
	}
	if(ohci->regs->intrstatus&8) {
		printk("%s Interrupt : USB Resume Detected\n", ohci->szName);
		ohci->regs->intrstatus=8;
	}
	if(ohci->regs->intrstatus&16) {
		printk("%s Interrupt : USB Unrecoverable Error\n", ohci->szName);
		ohci->regs->intrstatus=16;
	}
	if(ohci->regs->intrstatus&32) {
		printk("%s Interrupt : USB Frame Number Overflow\n", ohci->szName);
		ohci->regs->intrstatus=32;
	}
	if(ohci->regs->intrstatus&64) {
		printk("%s Interrupt : RootHub Status Change\n", ohci->szName);
		ohci->regs->intrstatus=64;
	}
	if(ohci->regs->intrstatus&0x40000000) {
		printk("%s Interrupt : USB Ownership Change\n", ohci->szName);
		ohci->regs->intrstatus=0x40000000;
	}
}

