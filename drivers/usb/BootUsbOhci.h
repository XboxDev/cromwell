
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-02-13 andy@warmcat.com  First CVS version

*/

#define COUNT_ROOTHUBS 15

// actual footprint of memory-mapped
// USB_OPERATIONAL_REGISTERS

typedef struct {
		// Control and Status
	DWORD m_dwHcRevision;
	DWORD m_dwHcControl;
	DWORD m_dwHcCommandStatus;
	DWORD m_dwHcInterruptStatus;
	DWORD m_dwHcInterruptEnable;
	DWORD m_dwHcInterruptDisable;
		// Memory Pointers
	DWORD m_dwHcHCCA;
	DWORD m_dwHcPeriodCurrentED;
	DWORD m_dwHcControlHeadED;
	DWORD m_dwHcControlCurrentED;
	DWORD m_dwHcBulkHeadED;
	DWORD m_dwHcBulkCurrentED;
	DWORD m_dwHcDoneHead;
		// Frame Counter
	DWORD m_dwHcRmInterval;
	DWORD m_dwHcFmRemaining;
	DWORD m_dwHcFmNumber;
	DWORD m_dwHcPeriodicStart;
	DWORD m_dwLsThreshold;
		// Root Hub
	DWORD m_dwHcRhDescriptorA;
	DWORD m_dwHcRhDescriptorB;
	DWORD m_dwHcRhStatus;
	DWORD m_dwHcRhPortStatus[COUNT_ROOTHUBS];
} USB_OPERATIONAL_REGISTERS __attribute((aligned(32)));

typedef struct {
	volatile USB_OPERATIONAL_REGISTERS * m_pusboperationalregisters;
	void * m_pvHostControllerCommsArea;
	size_t m_sizeAllocation;
	void * m_pvUSB_DEVICERootHubFirst;  // first ROOT_HUB USB Device, list formed by sibling pointer from first Root Hub USB_DEVICE
	DWORD m_dwCountAllocatedMemoryFromDescriptorStorage;
	char m_szName[32];
	DWORD m_dwCountInterrupts;
} USB_CONTROLLER_OBJECT;


//
// Host Controller Endpoint Descriptor, refer to Section 4.2, Endpoint Descriptor
//
typedef struct {
	DWORD m_hcendpointcontrolControl; // dword 0  Max Packet Size ==b26..b16
	volatile void * m_pvTailP; //physical pointer to HC_TRANSFER_DESCRIPTOR
	volatile void * m_pvHeadP; //flags + phys ptr to HC_TRANSFER_DESCRIPTOR
	volatile void * m_pvNextED; //phys ptr to HC_ENDPOINT_DESCRIPTOR

		// past here is for driver tracking only

} HC_ENDPOINT_DESCRIPTOR;

#define HcEDHeadP_HALT 0x00000001 //hardware stopped bit
#define HcEDHeadP_CARRY 0x00000002 //hardware toggle carry bit
//
// Host Controller Transfer Descriptor, refer to Section 4.3, Transfer Descriptors
//
typedef struct _HC_GENERAL_TRANSFER_DESCRIPTOR {
	DWORD m_hctransfercontrolControl; // dword 0
	void * m_pvCBP;  // buffer start, current position
	volatile void * m_pvNextTD; // phys ptr to HC_TRANSFER_DESCRIPTOR
	void * m_pvBE; // buffer end address
	
		// past here is for driver tracking only
		
	struct _HC_GENERAL_TRANSFER_DESCRIPTOR * m_pHC_GENERAL_TRANSFER_DESCRIPTORnext; // NULL ends list - chain of transfer descriptors made for same IRP

} HC_GENERAL_TRANSFER_DESCRIPTOR;

	// representation of USB device in driver (can be a hub)

typedef struct _USB_DEVICE {
	USB_CONTROLLER_OBJECT * m_pusbcontrollerOwner;
	struct _USB_DEVICE * m_pParentDevice;
	struct _USB_DEVICE * m_pNextSiblingDevice;
	struct _USB_DEVICE * m_pFirstChildDevice;  // linked list to child devices
	char m_szFriendlyName[32];
	BYTE m_bAddressUsb;
	HC_ENDPOINT_DESCRIPTOR * m_phcendpointdescriptorFirst;
} USB_DEVICE;

typedef enum {
	UTT_SETUP=0,
	UTT_OUT,
	UTT_IN,
	UTT_SETUP_OUT,
	UTT_SETUP_IN
} USB_TRANSFER_TYPE;

typedef struct {
	USB_DEVICE * m_pusbdevice;
	HC_ENDPOINT_DESCRIPTOR * m_phcendpointdescriptor;
	USB_TRANSFER_TYPE m_usbtransfertype; // decribes global transaction
	HC_GENERAL_TRANSFER_DESCRIPTOR * m_pHC_GENERAL_TRANSFER_DESCRIPTORfirst;
	BYTE * m_pbBuffer; //
	int m_nLengthBuffer;
	int m_nCompletionCode;
} USB_IRP;


void BootUsbInit(USB_CONTROLLER_OBJECT * pusbcontroller, const char * szcName, void * pvOperationalRegisterBase, void * pvHostControllerCommsArea, size_t sizeAllocation);
void BootUsbTurnOff(USB_CONTROLLER_OBJECT * pusbcontroller);

void BootUsbPrintStatus(USB_CONTROLLER_OBJECT *pusbcontroller);
void BootUsbInterrupt();

void * BootUsbDescriptorMalloc(USB_CONTROLLER_OBJECT *pusbcontroller, int nCount16ByteContiguousRegion);
void BootUsbDescriptorFree(USB_CONTROLLER_OBJECT *pusbcontroller, void *pvoid, int nCount16ByteContiguousRegion);
void BootUsbDump(USB_CONTROLLER_OBJECT *pusbcontroller);


void ConstructUSB_IRP(
	USB_IRP * m_pusbirp,
	USB_DEVICE *pusbdevice,
	HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptor,
	USB_TRANSFER_TYPE usbtransfertype,
	BYTE * pbBuffer,
	int nLengthBuffer
);


#if 0

typedef DWORD HC_ENDPOINT_CONTROL;
typedef DWORD HC_TRANSFER_CONTROL;

typedef DWORD HC_OPERATIONAL_REGISTER;
typedef DWORD HCCA_BLOCK;

#define NO_ED_LIST 32

//
// Doubly linked list
//



//
// HCD Endpoint Descriptor
//
typedef struct _HCD_ENDPOINT_DESCRIPTOR {
	BYTE m_bListIndex;
	BYTE m_bPausedFlag;
	BYTE m_bReserved[2];
	DWORD m_dwPhysicalAddress;
	LIST_ENTRY m_listentryLink;
	void * m_pvoidhcdendpointEndpoint;
//	HCD_ENDPOINT * m_phcdendpointEndpoint;
	DWORD m_dwReclamationFrame;
	LIST_ENTRY m_listentryPausedLink;
	HC_ENDPOINT_DESCRIPTOR m_hcendpointdescriptorHcED;
} HCD_ENDPOINT_DESCRIPTOR;


//
// USBD Request
//
typedef struct _USBD_REQUEST {
	BYTE * m_pbBuffer;
	DWORD m_dwBufferLength;
	DWORD m_dwXferInfo;
	DWORD m_dwMaxIntDelay;
	bool m_fShortXferOk;
	BYTE m_bSetup[8];
	DWORD m_dwStatus;
	LIST_ENTRY m_listentryHcdList;
} USBD_REQUEST;

// HCD Transfer Descriptor
//
typedef struct _HCD_TRANSFER_DESCRIPTOR {
	BYTE m_bTDStatus;
	bool m_fCancelPending;
	DWORD m_dwPhysicalAddress;
	struct _HCD_TRANSFER_DESCRIPTOR * m_phcdtransferdescriptorNextHcdTD;
	LIST_ENTRY m_listentryRequestList;
	USBD_REQUEST * m_pusbdrequestUsbdRequest;
	HCD_ENDPOINT * m_phcdendpointEndpoint;
	DWORD m_dwTransferCount;
	HC_TRANSFER_DESCRIPTOR m_hctransferdescriptorHcTD;
} HCD_TRANSFER_DESCRIPTOR;



//
// Each Host Controller Endpoint Descriptor is also doubly linked into a list tracked by HCD.
// Each ED queue is managed via an HCD_ED_LIST
//
typedef struct _HCD_ED_LIST {
	LIST_ENTRY m_listentryHead;
	DWORD * m_dwPhysicalHead;
	WORD m_wBandwidth;
	BYTE m_bNext;
	BYTE m_bReserved;
} HCD_ED_LIST;

//
// The different ED lists are as follows.
//
#define ED_INTERRUPT_1ms 0
#define ED_INTERRUPT_2ms 1
#define ED_INTERRUPT_4ms 3
#define ED_INTERRUPT_8ms 7
#define ED_INTERRUPT_16ms 15
#define ED_INTERRUPT_32ms 31
#define ED_CONTROL 63
#define ED_BULK 64
#define ED_ISOCHRONOUS 0 // same as 1ms interrupt queue
#define NO_ED_LISTS 65
#define ED_EOF 0xff
//
// HCD Device Data
//
typedef struct _HCD_DEVICE_DATA {
	HC_OPERATIONAL_REGISTER * m_phcoperationalregisterHC;
	HCCA_BLOCK * m_phccablockHCCA;
	LIST_ENTRY m_listentryEndpoints;
	LIST_ENTRY m_listentryFreeED;
	LIST_ENTRY m_listentryFreeTD;
	LIST_ENTRY m_listentryStalledEDReclamation;
	LIST_ENTRY m_listentryRunningEDReclamation;
	LIST_ENTRY m_listentryPausedEDRestart;
	HCD_ED_LIST hcdedlistaEdList[NO_ED_LIST];
	DWORD m_dwFrameHighPart;
	DWORD m_dwAvailableBandwidth;
	DWORD m_dwMaxBandwidthInUse;
	DWORD m_dwSOCount;
	DWORD m_dwSOStallFrame;
	DWORD m_dwSOLimitFrame;
	bool m_fSOLimitHit;
	bool m_fSOStallHit;
} HCD_DEVICE_DATA;


typedef struct _HCD_ENDPOINT {
	BYTE m_bType;
	BYTE m_bListIndex;
	BYTE m_bReserved[2];
	HCD_DEVICE_DATA * m_phcddevicedataDeviceData;
	HC_ENDPOINT_CONTROL m_hcendpointcontrolControl;
	HCD_ENDPOINT_DESCRIPTOR m_phcdendpointdescriptorHcdED;
	HCD_TRANSFER_DESCRIPTOR m_phcdtransferdescriptorHcdHeadP;
	HCD_TRANSFER_DESCRIPTOR m_phcdtransferdescriptorHcdTailP;
	DWORD m_dwRate;
	DWORD m_dwBandwidth;
	DWORD m_dwMaxPacket;
} HCD_ENDPOINT;


#endif

