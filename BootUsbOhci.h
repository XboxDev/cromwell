
#define COUNT_ROOTHUBS 1
#define MAX_USB_PATH_DEPTH 10


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
} USB_OPERATIONAL_REGISTERS;


//
// Host Controller Endpoint Descriptor, refer to Section 4.2, Endpoint Descriptor
//
typedef struct {
	DWORD m_hcendpointcontrolControl; // dword 0
	volatile void * m_pvTailP; //physical pointer to HC_TRANSFER_DESCRIPTOR
	volatile void * m_pvHeadP; //flags + phys ptr to HC_TRANSFER_DESCRIPTOR
	volatile void * m_pvNextED; //phys ptr to HC_ENDPOINT_DESCRIPTOR
} HC_ENDPOINT_DESCRIPTOR;

#define HcEDHeadP_HALT 0x00000001 //hardware stopped bit
#define HcEDHeadP_CARRY0x00000002 //hardware toggle carry bit
//
// Host Controller Transfer Descriptor, refer to Section 4.3, Transfer Descriptors
//
typedef struct {
	DWORD m_hctransfercontrolControl; // dword 0
	void * m_pvCBP;
	volatile void * m_pvNextTD; // phys ptr to HC_TRANSFER_DESCRIPTOR
	void * m_pvBE;
} HC_GENERAL_TRANSFER_DESCRIPTOR;


typedef struct {
	LIST_ENTRY m_list;
	char m_szFriendlyName[32];
	BYTE m_baPath[MAX_USB_PATH_DEPTH];
} USB_DEVICE;


typedef struct {
	volatile USB_OPERATIONAL_REGISTERS * m_pusboperationalregisters;
	void * m_pvHostControllerCommsArea;
	size_t m_sizeAllocation;
	USB_DEVICE m_usbdeviceaRootHubDevices[COUNT_ROOTHUBS];
} USB_CONTROLLER_OBJECT;



void BootUsbInit(USB_CONTROLLER_OBJECT * pusbcontroller, void * pvOperationalRegisterBase, void * pvHostControllerCommsArea, size_t sizeAllocation);

void BootUsbPrintStatus();
void BootUsbInterrupt();

void * BootUsbDescriptorMalloc(USB_CONTROLLER_OBJECT *pusbcontroller, int nCount16ByteContiguousRegion);
void BootUsbDescriptorFree(USB_CONTROLLER_OBJECT *pusbcontroller, void *pvoid, int nCount16ByteContiguousRegion);

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

