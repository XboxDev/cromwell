#ifndef _BootOHCI_H_
#define _BootOHCI_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include "list.h"

#include <linux/types.h>

#define __io_virt(x) ((void *)(x))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))

#define SIZEOF_HCCA 0x100

/* roothub.status bits */
#define RH_HS_LPS            0x00000001         /* local power status */
#define RH_HS_OCI            0x00000002         /* over current indicator */
#define RH_HS_DRWE           0x00008000         /* device remote wakeup enable */
#define RH_HS_LPSC           0x00010000         /* local power status change */
#define RH_HS_OCIC           0x00020000         /* over current indicator change */
#define RH_HS_CRWE           0x80000000         /* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR         0x0000ffff              /* device removable flags */
#define RH_B_PPCM       0xffff0000              /* port power control mask */

/* roothub.b masks */
#define RH_B_DR         0x0000ffff              /* device removable flags */
#define RH_B_PPCM       0xffff0000              /* port power control mask */

/* roothub.a masks */
#define RH_A_NDP        (0xff << 0)             /* number of downstream ports */
#define RH_A_PSM        (1 << 8)                /* power switching mode */
#define RH_A_NPS        (1 << 9)                /* no power switching */
#define RH_A_DT         (1 << 10)               /* device type (mbz) */
#define RH_A_OCPM       (1 << 11)               /* over current protection mode */
#define RH_A_NOCP       (1 << 12)               /* no over current protection */
#define RH_A_POTPGT     (0xff << 24)            /* power on to power good time */

/*
 * HcControl (control) register masks
 */
#define OHCI_CTRL_CBSR  (3 << 0)        /* control/bulk service ratio */
#define OHCI_CTRL_PLE   (1 << 2)        /* periodic list enable */
#define OHCI_CTRL_IE    (1 << 3)        /* isochronous enable */
#define OHCI_CTRL_CLE   (1 << 4)        /* control list enable */
#define OHCI_CTRL_BLE   (1 << 5)        /* bulk list enable */
#define OHCI_CTRL_HCFS  (3 << 6)        /* host controller functional state */
#define OHCI_CTRL_IR    (1 << 8)        /* interrupt routing */
#define OHCI_CTRL_RWC   (1 << 9)        /* remote wakeup connected */
#define OHCI_CTRL_RWE   (1 << 10)       /* remote wakeup enable */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */

#define OHCI_INTR_SO    (1 << 0)        /* scheduling overrun */
#define OHCI_INTR_WDH   (1 << 1)        /* writeback of done_head */
#define OHCI_INTR_SF    (1 << 2)        /* start frame */
#define OHCI_INTR_RD    (1 << 3)        /* resume detect */
#define OHCI_INTR_UE    (1 << 4)        /* unrecoverable error */
#define OHCI_INTR_FNO   (1 << 5)        /* frame number overflow */
#define OHCI_INTR_RHSC  (1 << 6)        /* root hub status change */
#define OHCI_INTR_OC    (1 << 30)       /* ownership change */
#define OHCI_INTR_MIE   (1 << 31)       /* master interrupt enable */


/* pre-shifted values for HCFS */
#       define OHCI_USB_RESET   (0 << 6)
#       define OHCI_USB_RESUME  (1 << 6)
#       define OHCI_USB_OPER    (2 << 6)
#       define OHCI_USB_SUSPEND (3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR        (1 << 0)        /* host controller reset */
#define OHCI_CLF        (1 << 1)        /* control list filled */
#define OHCI_BLF        (1 << 2)        /* bulk list filled */
#define OHCI_OCR        (1 << 3)        /* ownership change request */
#define OHCI_SOC        (3 << 16)       /* scheduling overrun count */


/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined in the OHCI spec. that the host controller is
 * told the base address of.  It must be 256-byte aligned.
 */

#define NUM_INTS 32     /* part of the OHCI standard */
struct ohci_hcca {
        __u32   int_table[NUM_INTS];    /* Interrupt ED table */
        __u16   frame_no;               /* current frame number */
        __u16   pad1;                   /* set to 0 on each frame_no change */
        __u32   done_head;              /* info returned for an interrupt */
        __u8    reserved_for_hc[116];
} __attribute((aligned(256)));
typedef struct ohci_hcca ohci_hcca_t;

/*
 * Maximum number of root hub ports.
 */

#define MAX_ROOT_PORTS 15

/*
 * This is the structure of the OHCI controller's memory mapped I/O
 * region.  This is Memory Mapped I/O.  You must use the readl() and
 * writel() macros defined in asm/io.h to access these!!
 */

struct ohci_regs {
        /* control and status registers */
        __u32   revision;
        __u32   control;
        __u32   cmdstatus;
        __u32   intrstatus;
        __u32   intrenable;
        __u32   intrdisable;
        /* memory pointers */
        __u32   hcca;
        __u32   ed_periodcurrent;
        __u32   ed_controlhead;
        __u32   ed_controlcurrent;
        __u32   ed_bulkhead;
        __u32   ed_bulkcurrent;
        __u32   donehead;
        /* frame counters */
        __u32   fminterval;
	__u32   fmremaining;
	__u32   fmnumber;
        __u32   periodicstart;
        __u32   lsthresh;
        /* Root hub ports */
	struct  ohci_roothub_regs {
		__u32 a;
		__u32 b;
		__u32 status;
		__u32 portstatus[MAX_ROOT_PORTS];
	} roothub;
} __attribute((aligned(32)));
typedef struct ohci_regs ohci_regs_t;

/* Virtual Root HUB */
struct virt_root_hub {
        int devnum; /* Address of Root Hub endpoint */
        void * urb;
        void * int_addr;
        int send;
        int interval;
};

/* ED States */

#define ED_NEW          0x00
#define ED_UNLINK       0x01
#define ED_OPER         0x02
#define ED_DEL          0x04
#define ED_URB_DEL      0x08

/* usb_ohci_ed */
struct ed {
        __u32 hwINFO;
        __u32 hwTailP;
        __u32 hwHeadP;
        __u32 hwNextED;
        struct ed * ed_prev;
        __u8 int_period;
        __u8 int_branch;
        __u8 int_load;
        __u8 int_interval;
        __u8 state;
        __u8 type;
        __u16 last_iso;
        struct ed * ed_rm_list;
        __u32 dma;
        __u32 unused[3];
} __attribute((aligned(16)));
typedef struct ed ed_t;

/* TD info field */
#define TD_CC       0xf0000000
#define TD_CC_GET(td_p) ((td_p >>28) & 0x0f)
#define TD_CC_SET(td_p, cc) (td_p) = ((td_p) & 0x0fffffff) | (((cc) & 0x0f) << 28)
#define TD_EC       0x0C000000
#define TD_T        0x03000000
#define TD_T_DATA0  0x02000000
#define TD_T_DATA1  0x03000000
#define TD_T_TOGGLE 0x00000000
#define TD_R        0x00040000
#define TD_DI       0x00E00000
#define TD_DI_SET(X) (((X) & 0x07)<< 21)
#define TD_DP       0x00180000
#define TD_DP_SETUP 0x00000000
#define TD_DP_IN    0x00100000
#define TD_DP_OUT   0x00080000

#define TD_ISO      0x00010000
#define TD_DEL      0x00020000

/* CC Codes */
#define TD_CC_NOERROR      0x00
#define TD_CC_CRC          0x01
#define TD_CC_BITSTUFFING  0x02
#define TD_CC_DATATOGGLEM  0x03
#define TD_CC_STALL        0x04
#define TD_DEVNOTRESP      0x05
#define TD_PIDCHECKFAIL    0x06
#define TD_UNEXPECTEDPID   0x07
#define TD_DATAOVERRUN     0x08
#define TD_DATAUNDERRUN    0x09
#define TD_BUFFEROVERRUN   0x0C
#define TD_BUFFERUNDERRUN  0x0D
#define TD_NOTACCESSED     0x0F


#define MAXPSW 1

struct td {
        __u32 hwINFO;
        __u32 hwCBP;            /* Current Buffer Pointer */
        __u32 hwNextTD;         /* Next TD Pointer */
        __u32 hwBE;             /* Memory Buffer End Pointer */

	__u16 hwPSW[MAXPSW];
        __u8 unused;
        __u8 index;
        struct ed * ed;
        struct td * next_dl_td;
        struct urb * urb;
        __u32 td_dma;
        __u32 data_dma;
	__u32 unused2[2];
} __attribute((aligned(32)));   /* normally 16, iso needs 32 */
typedef struct td td_t;



/* urb */
typedef struct
{
        ed_t * ed;
        __u16 length;   // number of tds associated with this request
        __u16 td_cnt;   // number of tds already serviced
        int   state;
        td_t * td[0];   // list pointer to all corresponding TDs associated with this request
} urb_priv_t;

#define URB_DEL 1

/* Hash struct used for TD/ED hashing */
struct hash_t {
        void            *virt;
        __u32      dma;
        struct hash_t   *next; // chaining for collision cases
};

/* List of TD/ED hash entries */
struct hash_list_t {
        struct hash_t   *head;
        struct hash_t   *tail;
};

#define TD_HASH_SIZE    64    /* power'o'two */
#define ED_HASH_SIZE    64    /* power'o'two */

#define TD_HASH_FUNC(td_dma) ((td_dma ^ (td_dma >> 5)) % TD_HASH_SIZE)
#define ED_HASH_FUNC(ed_dma) ((ed_dma ^ (ed_dma >> 5)) % ED_HASH_SIZE)

typedef struct ohci {
	__u32 Interrupts;
        volatile struct ohci_hcca *hcca;         /* hcca */
        __u32 hcca_dma;

        int disabled;                   /* e.g. got a UE, we're hung */
        unsigned long flags;            /* for HC bugs */
#define OHCI_QUIRK_AMD756       0x01            /* erratum #4 */
        struct ohci_regs * regs;        /* OHCI controller's memory */
        struct list_head ohci_hcd_list; /* list of all ohci_hcd */

        struct ohci * next;             // chain of ohci device contexts
        // struct list_head urb_list;   // list of all pending urbs
        // spinlock_t urb_list_lock;    // lock to keep consistency

        int ohci_int_load[32];          /* load of the 32 Interrupt Chains (for load balancing)*/
	ed_t * ed_rm_list[2];     /* lists of all endpoints to be removed */
	ed_t * ed_bulktail;       /* last endpoint of bulk list */
	ed_t * ed_controltail;    /* last endpoint of control list */
	ed_t * ed_isotail;        /* last endpoint of iso list */
	int intrstatus;
        __u32 hc_control;               /* copy of the hc control reg */
	struct usb_bus * bus;
	struct usb_device * dev[128];
	struct virt_root_hub rh;
        /* PCI device handle, settings, ... */
        struct hash_list_t      td_hash[TD_HASH_SIZE];
        struct hash_list_t      ed_hash[ED_HASH_SIZE];
	char szName[32];
} ohci_t;

#define NUM_EDS 32              /* num of preallocated endpoint descriptors */

struct ohci_device {
        ed_t ed[NUM_EDS];
        __u32 dma;
        int ed_cnt;
};

#define usb_to_ohci(usb)        ((struct ohci_device *)(usb)->hcpriv)

void BootUsbInit(ohci_t * ohci, const char *szName, void *regbase);
void BootUsbInterrupt(ohci_t *ohci);
void BootUsbTurnOff(ohci_t * ohci);

#endif

