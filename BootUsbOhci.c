
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
#include "BootUsbOhci.h"

#define SIZEOF_HCCA 0x100

const LIST_ENTRY listentryEmpty = { NULL, NULL };

void ConstructHC_GENERAL_TRANSFER_DESCRIPTOR(
	HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptor,
	DWORD dwControl,
	BYTE * pbBuffer,
	DWORD dwLengthBuffer
) {
	phcgeneraltransferdescriptor->m_hctransfercontrolControl=dwControl&(~0x10000); // force b16 to zero, inidcating 16-byte General TD (as opposed to 32-byte Isochronous one)
	phcgeneraltransferdescriptor->m_pvCBP=pbBuffer;  // buffer start, current position
	phcgeneraltransferdescriptor->m_pvNextTD=NULL; // phys ptr to HC_TRANSFER_DESCRIPTOR (inited to NULL)
	phcgeneraltransferdescriptor->m_pvBE=&pbBuffer[dwLengthBuffer-1]; // buffer end address
}

void DestructHC_GENERAL_TRANSFER_DESCRIPTOR(
	USB_CONTROLLER_OBJECT * pusbcontrollerOwner,
	HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptor
) {
	BootUsbDescriptorFree(pusbcontrollerOwner, phcgeneraltransferdescriptor, 2);
}


void AppendHC_GENERAL_TRANSFER_DESCRIPTORtoHC_ENDPOINT_DESCRIPTOR(
	HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptor,
	HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptor
) {
	if( (((int)phcendpointdescriptor->m_pvHeadP)&0xfffffff0) == 0 ) {  // first TD on this ED
//		phcendpointdescriptor->m_pvHeadP
	}
}

void ConstructHC_ENDPOINT_DESCRIPTOR(HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptor)
{
	phcendpointdescriptor->m_hcendpointcontrolControl=0x00080000; // default to 8 byte max pkt size e/p 0 fn 0
	phcendpointdescriptor->m_pvTailP=NULL;
	phcendpointdescriptor->m_pvHeadP=NULL;
	phcendpointdescriptor->m_pvNextED=NULL;
}


void DestructHC_ENDPOINT_DESCRIPTOR(USB_CONTROLLER_OBJECT * pusbcontrollerOwner, HC_ENDPOINT_DESCRIPTOR *phcendpointdescriptor)
{  // does not unlink from any lists
	HC_GENERAL_TRANSFER_DESCRIPTOR * pgeneraltransferdescriptor=(HC_GENERAL_TRANSFER_DESCRIPTOR *)((int)phcendpointdescriptor->m_pvHeadP& (~0xf));
	bool fMore=true;
		// destroy any transfer descriptors owned by this endpoint descriptor
	while(fMore) {
		HC_GENERAL_TRANSFER_DESCRIPTOR * pgeneraltransferdescriptorNext;
		int nDescriptorSizeOver16=1;

		if(pgeneraltransferdescriptor==phcendpointdescriptor->m_pvTailP) { fMore=false; continue; }
		pgeneraltransferdescriptorNext=(HC_GENERAL_TRANSFER_DESCRIPTOR *)pgeneraltransferdescriptor->m_pvNextTD;
		if(pgeneraltransferdescriptor->m_hctransfercontrolControl & 0x10000) nDescriptorSizeOver16++;  // b16 set on the descriptor indicates ISO 32-byte size one
		BootUsbDescriptorFree(pusbcontrollerOwner, pgeneraltransferdescriptor, nDescriptorSizeOver16);
		pgeneraltransferdescriptor=pgeneraltransferdescriptorNext;
	}
		// then kill the endpoint itself
	BootUsbDescriptorFree(pusbcontrollerOwner, phcendpointdescriptor, 1);
}


void ConstructUSB_DEVICE(USB_DEVICE * pusbdevice, USB_CONTROLLER_OBJECT * pusbcontrollerOwner, USB_DEVICE * pusbdeviceParent, const char * szName)
{
	HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptor=BootUsbDescriptorMalloc(pusbcontrollerOwner, 1);
	ConstructHC_ENDPOINT_DESCRIPTOR(phcendpointdescriptor);  // create an endpoint descriptor for the default pipe at endpoint 0

	pusbdevice->m_pusbcontrollerOwner=pusbcontrollerOwner;
	pusbdevice->m_pParentDevice=pusbdeviceParent;
	pusbdevice->m_pNextSiblingDevice=NULL;
	pusbdevice->m_pFirstChildDevice=NULL;
	_strncpy(pusbdevice->m_szFriendlyName, szName, sizeof(pusbdevice->m_szFriendlyName)-1);
	pusbdevice->m_bAddressUsb=0;

	pusbdevice->m_phcendpointdescriptorFirst=phcendpointdescriptor; // attach our default endpoint descriptor
}


void DestructUSB_DEVICE(USB_DEVICE * pusbdevice)  // does not unlink from parent
{
		// recurse to destruct any child devices first
	{
		USB_DEVICE *pusbdeviceChildNext, *pusbdeviceChild=pusbdevice->m_pFirstChildDevice;
		while(pusbdeviceChild!=NULL) {
			pusbdeviceChildNext=pusbdeviceChild->m_pNextSiblingDevice;
			DestructUSB_DEVICE(pusbdeviceChild);  // does not unlink from parent list, but that's okay as parent is being destroyed next
			pusbdeviceChild=pusbdeviceChildNext;
		}
	}

		// destruct our enpoint descriptors
	{
		HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptorNext, *phcendpointdescriptor=pusbdevice->m_phcendpointdescriptorFirst;

		while(phcendpointdescriptor!=NULL) {
			phcendpointdescriptorNext=(HC_ENDPOINT_DESCRIPTOR *)phcendpointdescriptor->m_pvNextED;
			DestructHC_ENDPOINT_DESCRIPTOR(pusbdevice->m_pusbcontrollerOwner, phcendpointdescriptor);
			phcendpointdescriptor=phcendpointdescriptorNext;
		}
	}

	free(pusbdevice);
}


void ConstructUSB_CONTROLLER_OBJECT(
	USB_CONTROLLER_OBJECT * pusbcontroller,
	void * pvOperationalRegisterBase,
	void * pvHostControllerCommsArea,
	size_t sizeAllocation
) {
	USB_DEVICE * * ppLastUsbDevicePtr=(USB_DEVICE * *)&pusbcontroller->m_pvUSB_DEVICERootHubFirst; // update USB device first root hub device ptr first
	int n;

	pusbcontroller->m_pusboperationalregisters = (volatile USB_OPERATIONAL_REGISTERS *)pvOperationalRegisterBase;
	pusbcontroller->m_pvHostControllerCommsArea = pvHostControllerCommsArea;
	pusbcontroller->m_sizeAllocation = sizeAllocation;
	pusbcontroller->m_dwCountInterrupts=0;

		// construct the root hub usb devices associated with this Usb controller

	for(n=0;n<COUNT_ROOTHUBS;n++) {
		char sz[32];
		USB_DEVICE * pusbdevice=malloc(sizeof(USB_DEVICE));

		*ppLastUsbDevicePtr=pusbdevice;
		sprintf(sz, "Root Hub #%u", n+1);

		ConstructUSB_DEVICE(pusbdevice, pusbcontroller, NULL, sz); // parent device is NULL as root hub is toplevel

		ppLastUsbDevicePtr=&pusbdevice->m_pNextSiblingDevice; // subsequently update last root hub's USB_DEVICE next sibling pointer
	}

		// We can just leave it at that, because shortly we can expect a RootHub Status Change interrupt for each of these guys
}

void DestructUSB_CONTROLLER_OBJECT(USB_CONTROLLER_OBJECT * pusbcontroller)  // does not deallocate mem for controller object!
{
		// destruct all of the root hubs (they will in turn destruct their child USB_DEVICEs)
	{
		USB_DEVICE * pusbdevice=(USB_DEVICE *)pusbcontroller->m_pvUSB_DEVICERootHubFirst;
		while(pusbdevice) {
			USB_DEVICE * pusbdeviceNext = (USB_DEVICE *)pusbdevice->m_pNextSiblingDevice;
			DestructUSB_DEVICE(pusbdevice);
			pusbdevice=pusbdeviceNext;
		}
	}
}


void BootUsbInit(
	USB_CONTROLLER_OBJECT * pusbcontroller,
	const char *szName,
	void * pvOperationalRegisterBase,
	void * pvHostControllerCommsArea,
	size_t sizeAllocation
) {
	DWORD dwSaved;

	_strncpy(pusbcontroller->m_szName, szName, sizeof(pusbcontroller->m_szName)-1 );

	ConstructUSB_CONTROLLER_OBJECT(pusbcontroller, pvOperationalRegisterBase, pvHostControllerCommsArea, sizeAllocation);

	memset(pusbcontroller->m_pvHostControllerCommsArea, 0, sizeAllocation);

		// init the hardware

		// what state is the hardware in at the moment?  (usually reset state)
	switch((pusbcontroller->m_pusboperationalregisters->m_dwHcControl)&0xc0)  {
		case 0x00: // in reset: a cold boot scenario
				// set up some recommended params
			pusbcontroller->m_pusboperationalregisters->m_dwHcRhDescriptorA=0x10000204; // 32mS POTPGT, Root hub always on
			pusbcontroller->m_pusboperationalregisters->m_dwHcRhDescriptorB=0x00020002; // root hub 1 removable, per-port power control
			pusbcontroller->m_pusboperationalregisters->m_dwHcRmInterval=(4096<<16)|(11999); // largest data packet =4Kbit, 12000 clocks/ms
				// take us operational
			pusbcontroller->m_pusboperationalregisters->m_dwHcControl=((pusbcontroller->m_pusboperationalregisters->m_dwHcControl)&(~0xc0))|0x80;
			Sleep(50000); // 5.1.1.3.2 says wait a little and continue setup
			break;

		case 0x80: // operational already, 5.1.1.3.2 says just continue setup
			break;

		case 0x40: // resume
		case 0xc0: // suspend
				// 5.1.1.3.2 says to stick us in resume, wait some time for that to be digested and continue setup
			pusbcontroller->m_pusboperationalregisters->m_dwHcControl=((pusbcontroller->m_pusboperationalregisters->m_dwHcControl)&(~0xc0))|0x40;
			Sleep(50000);
			break;
	}

	dwSaved=pusbcontroller->m_pusboperationalregisters->m_dwHcRmInterval;  // stash interval
	pusbcontroller->m_pusboperationalregisters->m_dwHcCommandStatus=1;  // Host controller reset
	Sleep(50); // 50uS, should only take 10uS to complete the reset action
	pusbcontroller->m_pusboperationalregisters->m_dwHcRmInterval=dwSaved; // return to old interval

		// 5.1.1.4 prep the HCCA block data before pointing hardware to it


		// 5.1.1.4 init the ED Operational registers

	pusbcontroller->m_pusboperationalregisters->m_dwHcControlHeadED=0x00000000;
	pusbcontroller->m_pusboperationalregisters->m_dwHcBulkHeadED=0x00000000;

	pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptEnable=0x8000007b; // 5.1.1.4 all interrupt reasons except SOF
	pusbcontroller->m_pusboperationalregisters->m_dwHcPeriodicStart=0x3e67; // 7.3.4 recommended value, approx 90% of HcFmInterval
	pusbcontroller->m_pusboperationalregisters->m_dwHcHCCA=(DWORD)pvHostControllerCommsArea; // 5.1.1.4 tell logic where the HCCA is in memory
	pusbcontroller->m_pusboperationalregisters->m_dwHcControl=0x000000bd; // 5.1.1.4 set control reg to 'all queues on' 0xbd==all queues on, 2:1 control/bulk ratio, no wakeup

	pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptEnable=0x80000004; // 5.1.1.5 after other setup enable SOF interrupt too


	bprintf("   Done\n");
}


void BootUsbTurnOff(USB_CONTROLLER_OBJECT * pusbcontroller)
{
	pusbcontroller->m_pusboperationalregisters->m_dwHcControl=
		((pusbcontroller->m_pusboperationalregisters->m_dwHcControl)&(~0xc0))|0x00;
	pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptDisable=0x8000007f; // kill all interrupts
	pusbcontroller->m_pusboperationalregisters->m_dwHcHCCA=(DWORD)0; // kill HCCA is in memory
}



void BootUsbPrintStatus(USB_CONTROLLER_OBJECT * pusbcontroller)
{
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcRevision<0x10) {
		printk("none detected  ");
	} else {
		printk("OHCI Rev %d.%d  ", (pusbcontroller->m_pusboperationalregisters->m_dwHcRevision&0xff)/10, (pusbcontroller->m_pusboperationalregisters->m_dwHcRevision&0xff)%10);
	}
}



void ConstructUSB_IRP(
	USB_IRP * m_pusbirp,
	USB_DEVICE *pusbdevice,
	HC_ENDPOINT_DESCRIPTOR * phcendpointdescriptor,
	USB_TRANSFER_TYPE usbtransfertype,
	BYTE * pbBuffer,
	int nLengthBuffer
) {
	m_pusbirp->m_pusbdevice=pusbdevice;
	m_pusbirp->m_phcendpointdescriptor=phcendpointdescriptor;
	m_pusbirp->m_usbtransfertype=usbtransfertype;
	m_pusbirp->m_pHC_GENERAL_TRANSFER_DESCRIPTORfirst=NULL;
	m_pusbirp->m_pbBuffer=pbBuffer;
	m_pusbirp->m_nLengthBuffer=nLengthBuffer;
	m_pusbirp->m_nCompletionCode=0;
}

void DestructUSB_IRP(USB_IRP * m_pusbirp)  // deletes IRP object too
{
		// FIXME: needs to cancel all pending transfer descriptors first
		//  without this code you can only destruct unstarted, completed or failed IRPs

		// kill all transfer descriptors created to service this IRP

	HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptor=m_pusbirp->m_pHC_GENERAL_TRANSFER_DESCRIPTORfirst;
	while(phcgeneraltransferdescriptor!=NULL) {
		HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptorTemp=phcgeneraltransferdescriptor->m_pHC_GENERAL_TRANSFER_DESCRIPTORnext;
		DestructHC_GENERAL_TRANSFER_DESCRIPTOR(m_pusbirp->m_pusbdevice->m_pusbcontrollerOwner, phcgeneraltransferdescriptor); // frees TD object too
		phcgeneraltransferdescriptor=phcgeneraltransferdescriptorTemp;
	}

	free(m_pusbirp);
}

	// construction and service are totally separate
	// service can be done in ISR

void ServiceUSB_IRP(USB_IRP *pirp)
{
	HC_GENERAL_TRANSFER_DESCRIPTOR * pgeneraltransferdescriptor, * pgeneraltransferdescriptorPrev;
	DWORD dwControl=0x00000000, dwControlPayload=0x00000000, dwControlAck=0x00000000;
	bool fHasPayload=true;
	int nCount=pirp->m_nLengthBuffer;
	BYTE * pb=pirp->m_pbBuffer;

	switch(pirp->m_usbtransfertype) {
		case UTT_SETUP: // two stage, p106 Mindshare book  setup pkt / ack in to host
			dwControl=0x00000000; // setup
			dwControlAck=0x00100000; // IN
			fHasPayload=false;
			break;
		case UTT_OUT: // out pkt / data to device / ack in to host
			dwControl=0x00080000; // OUT PID is going OUT to device
			dwControlPayload=0x00080000; // OUT payload
			dwControlAck=0x00100000; // IN
			break;
		case UTT_IN:  // in pkt / data to host / ack out to device
			dwControl=0x00080000; // IN PID going OUT to device
			dwControlPayload=0x00100000; // IN payload
			dwControlAck=0x00080000; // OUT
			break;
		case UTT_SETUP_OUT:	// setup pkt / data to device / ack in to host
			dwControl=0x00000000; // setup pkt going OUT to device
			dwControlPayload=0x00080000; // OUT
			dwControlAck=0x00100000; // IN
			break;
		case UTT_SETUP_IN:  // setup pkt / data to host / ack out to device
			dwControl=0x00000000; // setup pkt going OUT to device
			dwControlPayload=0x00100000; // IN
			dwControlAck=0x00080000; // OUT
			break;
	}

		// issue PID

	pgeneraltransferdescriptor=BootUsbDescriptorMalloc(pirp->m_pusbdevice->m_pusbcontrollerOwner, 2);
	ConstructHC_GENERAL_TRANSFER_DESCRIPTOR(pgeneraltransferdescriptor, dwControl, NULL, 0); // no payload for PID
	pirp->m_pHC_GENERAL_TRANSFER_DESCRIPTORfirst=pgeneraltransferdescriptor; // track what we create, PID is first
	pgeneraltransferdescriptorPrev=pgeneraltransferdescriptor;

		// payload packets

	if(fHasPayload) {
		while(nCount) {
			int nThisTime=(pirp->m_phcendpointdescriptor->m_hcendpointcontrolControl >>16)&0x7ff; // endpoint max payload per transfer in bytes
			if(nCount<nThisTime) nThisTime=nCount;

			pgeneraltransferdescriptor=BootUsbDescriptorMalloc(pirp->m_pusbdevice->m_pusbcontrollerOwner, 2);
			ConstructHC_GENERAL_TRANSFER_DESCRIPTOR(pgeneraltransferdescriptor, dwControlPayload, pb, nThisTime); // part of total payload
			pgeneraltransferdescriptorPrev->m_pHC_GENERAL_TRANSFER_DESCRIPTORnext=pgeneraltransferdescriptor; // track what we create, PID is first
			pgeneraltransferdescriptorPrev=pgeneraltransferdescriptor;

			nCount-=nThisTime;
			pb+=nThisTime;
		}
	}

		// ACK packet

	pgeneraltransferdescriptor=BootUsbDescriptorMalloc(pirp->m_pusbdevice->m_pusbcontrollerOwner, 2);
	ConstructHC_GENERAL_TRANSFER_DESCRIPTOR(pgeneraltransferdescriptor, dwControlAck, NULL, 0); // part of total payload
	pgeneraltransferdescriptorPrev->m_pHC_GENERAL_TRANSFER_DESCRIPTORnext=pgeneraltransferdescriptor; // track what we create, PID is first

}


	// manage response to detection of roothub status change - called by ISR
	// we get one of these after coming out of OHCI reset

void BootUsbRootHubStatusChange(USB_DEVICE *pusbdeviceRootHub)
{
//	HC_GENERAL_TRANSFER_DESCRIPTOR * phcgeneraltransferdescriptor;
			// cook a transfer descriptor

//	pgeneraltransferdescriptor=BootUsbDescriptorMalloc(pusbdeviceRootHub->m_pusbcontrollerOwner, 2);
/*
	ConstructHC_GENERAL_TRANSFER_DESCRIPTOR(
		phcgeneraltransferdescriptor,
		dwControl,
		pbBuffer,
		dwLengthBuffer
	);
*/

}



	// ISR calls through to here

void BootUsbInterrupt(USB_CONTROLLER_OBJECT * pusbcontroller)
{
	pusbcontroller->m_dwCountInterrupts++;

	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&1) {
		bprintf("%s Interrupt (%u): USB Scheduling Overrun\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=1;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&2) {
		bprintf("%s Interrupt (%u): USB Writeback Done Head\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=2;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&4) {
//		bprintf("%s Interrupt (%u): USB Start of Frame\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=4;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&8) {
		bprintf("%s Interrupt (%u): USB Resume Detected\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=8;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&16) {
		bprintf("%s Interrupt (%u): USB Unrecoverable Error\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=16;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&32) {
		bprintf("%s Interrupt (%u): USB Frame Number Overflow\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=32;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&64) {
		int n;
		USB_DEVICE * pusbdeviceRootHub=(USB_DEVICE *)pusbcontroller->m_pvUSB_DEVICERootHubFirst;
		bprintf("%s Interrupt (%u): RootHub Status Change\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		for(n=0;n<COUNT_ROOTHUBS;n++) {
			if(pusbcontroller->m_pusboperationalregisters->m_dwHcRhPortStatus[n]&0x10000) {
						// change in connection status detected in n-th root hub
					pusbcontroller->m_pusboperationalregisters->m_dwHcRhPortStatus[n]=0x10000; // clear it
					BootUsbRootHubStatusChange(pusbdeviceRootHub);
			}
			pusbdeviceRootHub=pusbdeviceRootHub->m_pNextSiblingDevice;
		}
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=64;
	}
	if(pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus&0x40000000) {
		bprintf("%s Interrupt (%u): USB Ownership Change\n", pusbcontroller->m_szName, pusbcontroller->m_dwCountInterrupts);
		pusbcontroller->m_pusboperationalregisters->m_dwHcInterruptStatus=0x40000000;
	}
}


void * BootUsbDescriptorMalloc(USB_CONTROLLER_OBJECT *pusbcontroller, int nCount16ByteContiguousRegion)
{
		// we maintain a little FAT type table at the start, each byte == 16 bytes block in use
	BYTE * pb=((BYTE *)pusbcontroller->m_pvHostControllerCommsArea)+SIZEOF_HCCA;
	DWORD dwSizeofFatInBytes=((pusbcontroller->m_sizeAllocation-SIZEOF_HCCA)/16);
	int n;

	for(n=0;n<dwSizeofFatInBytes;n++) {
		if(pb[n]==0) {
			int y=0;
			bool fBroken=false;
			for(y=1;y<nCount16ByteContiguousRegion;y++) { if(pb[n+y]) fBroken=true; }
			if(fBroken) continue;
			for(y=0;y<nCount16ByteContiguousRegion;y++) { pb[n+y]=1; }
			pusbcontroller->m_dwCountAllocatedMemoryFromDescriptorStorage+=(nCount16ByteContiguousRegion<<4);
			bprintf("BootUsbDescriptorMalloc sector %u, len %u, ptr=0x%x, 0x%x\n", n, nCount16ByteContiguousRegion, pb+dwSizeofFatInBytes+(16*n), n);
			return pb+dwSizeofFatInBytes+(16*n);
		}
	}
	return NULL;
}

void BootUsbDescriptorFree(USB_CONTROLLER_OBJECT *pusbcontroller, void *pvoid, int nCount16ByteContiguousRegion)
{
	BYTE * pb=((BYTE *)pusbcontroller->m_pvHostControllerCommsArea)+SIZEOF_HCCA;
	DWORD dwSizeofFatInBytes=((pusbcontroller->m_sizeAllocation-SIZEOF_HCCA)/16);
	int n=(((BYTE *)pvoid)-pb -dwSizeofFatInBytes)/16;
	int y;
	pusbcontroller->m_dwCountAllocatedMemoryFromDescriptorStorage-=(nCount16ByteContiguousRegion<<4);
	bprintf("BootUsbDescriptorFree sector %u, len %u\n", n, nCount16ByteContiguousRegion);
	for(y=0;y<nCount16ByteContiguousRegion;y++) { pb[n++]=0; }
}

void ListEntryInsertAt(LIST_ENTRY *plistentryCurrent, LIST_ENTRY *plistentryNew);
void ListEntryRemove(LIST_ENTRY *plistentryCurrent);


void BootUsbDumpDeviceAndSiblingsAndRecurseForChildren(USB_DEVICE *pusbdevice, int nIndent)
{
	while(pusbdevice) {
		int n;
		n=0; while(n++ <nIndent) printk(" ");
		printk("[] %s\n",
			pusbdevice->m_szFriendlyName
		);
		{
			USB_DEVICE *pusbdeviceChild=(USB_DEVICE *)pusbdevice->m_pFirstChildDevice;
			while(pusbdeviceChild) {
				BootUsbDumpDeviceAndSiblingsAndRecurseForChildren(pusbdeviceChild, nIndent+1);
				pusbdeviceChild=pusbdeviceChild->m_pNextSiblingDevice;
			}
		}
		pusbdevice=pusbdevice->m_pNextSiblingDevice;
	}
}

void BootUsbDump(USB_CONTROLLER_OBJECT *pusbcontroller)
{
	printk("%s devices:\n", pusbcontroller->m_szName);

	BootUsbDumpDeviceAndSiblingsAndRecurseForChildren((USB_DEVICE *)pusbcontroller->m_pvUSB_DEVICERootHubFirst, 1);

}


#if 0

// this code has come from the OHCI standard
// its less useful than I hoped so far, but maybe I just don't understand enough to appreciate it yet

	// endpoint descriptor lists

void
InsertEDForEndpoint (
IN PHCD_ENDPOINT Endpoint
)
{
	PHCD_DEVICE_DATA DeviceData;
	PHCD_ED_LIST List;
	PHCD_ENDPOINT_DESCRIPTOR ED, TailED;
	DeviceData = Endpoint->DeviceData;
	List = &DeviceData->EdList[Endpoint->ListIndex];
//
// Initialize an endpoint descriptor for this endpoint
//
	ED = AllocateEndpointDescriptor(DeviceData);
	ED->Endpoint = Endpoint;
	ED->ListIndex = Endpoint->ListIndex;
	ED->PhysicalAddress = PhysicalAddressOf(&ED->HcED);
	ED->HcED.Control = Endpoint->Control;
	Endpoint->HcdHeadP = AllocateTransferDescriptor(DeviceData);
	ED->HcED.HeadP = PhysicalAddressOf(&Endpoint->HcdHeadP->HcTD);
	Endpoint->HcdHeadP->PhysicalAddress = ED->HcED.TailP = ED->HcED.HeadP;
	Endpoint->HcdED = ED;
	ED->HcdHeadP->UsbdRequest = NULL;
//
// Link endpoint descriptor into HCD tracking queue
//
	if (Endpoint->Type != Isochronous || IsListEmpty(&List->Head))) {
//
// Link ED into head of ED list
//
		InsertHeadList (&List->Head, &ED->Link);
		ED->HcED.NextED = *List->PhysicalHead;
		*List->PhysicalHead = ED->PhysicalAddress;
	} else {
//
// Link ED into tail of ED list
//
		TailED = CONTAINING_RECORD (
			List->Head.Blink,
			HCD_ENDPOINT_DESCRIPTOR,
			Link
		);
		InsertTailList (&List->Head, &Endpoint->Link);
		ED->NextED = 0;
		TailED->NextED = ED->PhysicalAddress;
	}
}


void RemoveED (
	IN PHCD_ENDPOINT Endpoint,
	IN bool FreeED
)
{
	PHCD_DEVICE_DATA DeviceData;
	PHCD_ED_LIST List;
	PHCD_ENDPOINT_DESCRIPTOR ED, PeviousED;
	DWORD ListDisable;
	DeviceData = Endpoint->DeviceData;
	List = &DeviceData->EdList[Endpoint->ListIndex];
	ED = Endpoint->HcdED;
//
// Prevent Host Controller from processing this ED
//
	ED->HcED.Control.sKip = TRUE;
//
// Unlink the ED from the physical ED list
//
	if (ED->Link.Blink == &List->Head) {
//
// Remove ED from head
//
		*List->PhysicalHead = ED->HcED.NextED;
		PreviousED = NULL;
	} else {
//
// Remove ED from list
//
		PreviousED = CONTAINING_RECORD (
		ED->Link.Blink,
		HCD_ENDPOINT,
		Link);
		PreviousED->HcED.NextED = ED->HcED.NextED;
	}
//
// Unlink ED from HCD list
//
	RemoveEntryList (&ED->Link);
//
// If freeing the endpoint, remove the descriptor
//
	if (FreeED) { // TD queue must already be empty
		Endpoint->HcdED = NULL;
		ED->Endpoint = NULL;
	}

//
// Check to see if interrupt processing is required to free the ED
//
	switch (Endpoint->Type) {
		case Control:
			ListDisable = ~ControlListEnable;
			break;
		case Bulk:
			ListDisable = ~BulkListEnable;
			break;
		default:
			DeviceData->EDList[Endpoint->ListIndex].Bandwidth -= Endpoint->Bandwidth;
			DeviceData->MaxBandwidthInUse = CheckBandwidth(DeviceData,
			ED_INTERRUPT_32ms,
			&ListDisable);
			ListDisable = 0;
	}
	ED->ListIndex = ED_EOF; // ED is not on a list now
//
// Set ED for reclamation
//
	DeviceData->HC->HcInterruptStatus = HC_INT_SOF;// clear SOF interrupt pending
	if (ListDisable) {
		DeviceData->HC->HcControl &= ListDisable;
		ED->ReclaimationFrame = Get32BitFrameNumber(DeviceData) + 1;
		InsertTailList (&DeviceData->StalledEDReclamation, &ED->Link);
		DeviceData->HC-> HcInterruptEnable = HC_INT_SOF;// interrupt on next SOF
	} else {
		ED->ReclaimationFrame = Get32BitFrameNumber(DeviceData) + 1;
		InsertTailList (&DeviceData->RunningEDReclamation, &ED->Link);
	}
}

void PauseED(
	IN PCHD_ENDPOINT Endpoint
)
{
	PHCD_DEVICE_DATA DeviceData;
	PHCD_ENDPOINT_DESCRIPTOR ED;
	DeviceData = Endpoint->DeviceData;
	ED = Endpoint->HcdED;
	ED->HcED.Control.sKip = TRUE;
	if (ED->PausedFlag) return; // already awaiting pause processing
	if ( !(ED->HcED.HeadP & HcEDHeadP_HALT) ) {
//
// Endpoint is active in Host Controller, wait for SOF before processing the endpoint.
//
		ED->PausedFlag = TRUE;
		DeviceData->HC->HcInterruptStatus = HC_INT_SOF;// clear SOF interrupt pending
		ED->ReclaimationFrame = Get32BitFrameNumber(DeviceData) + 1;
		InsertTailList (&DeviceData->PausedEDRestart, &ED->PausedLink);
		DeviceData->HC-> HcInterruptEnable = HC_INT_SOF;// interrupt on next SOF
		return;
	}
//
// Endpoint already paused, do processing now
//
	ProcessPausedED(ED);
}


void ProcessPausedED (
	PHCD_ENDPOINT_DESCRIPTOR ED
)
{
	PHCD_ENDPOINT endpoint;
	PUSBD_REQUEST request;
	PHCD_TRANSFER_DESCRIPTOR td, last = NULL, *previous;
	bool B4Head = TRUE;
	endpoint = ED->Endpoint;
	if (endpoint == NULL) return;
	td = endpoint->HcdHeadP;
	previous = &endpoint->HcdHeadP;
	while (td != endpoint->HcdTailP) {
		if ((ED->HcED.HeadP & ~0xF) == td->PhysicalAddress) B4Head = FALSE;
		if (ED->ListIndex == ED_EOF || td->CancelPending) {// cancel TD
			request = td->UsbdRequest;
			RemoveListEntry(&td->RequestList);
			if (IsListEmpty(&request->HcdList) {
				request->Status = USBD_CANCELED;
				CompleteUsbdRequest(request);
			}
			*previous = td->NextHcdTD; // point around TD
			if (last != NULL) last->HcED.NextTD = td->HcED.NextTD;
			if (B4Head) td->Status = TD_CANCELED; else FreeTransferDescriptor(td);
		} else { // don’t cancel TD
			previous = &td->NextHcdTD;
			if (!B4Head) last = td;
		}
		td = *previous;
	}
	
	ED->HcED.HeadP = endpoint->HcdHeadP->PhysicalAddress | (ED->HcED.HeadP & HcEDHeadP_CARRY);
	ED->HcED.Control.sKip = FALSE;
}


void InitailizeInterruptLists (
	IN PHCD_DEVICE_DATA DeviceData
)
{
	PHC_ENDPOINT_DESCRIPTOR ED, StaticED[ED_INTERRUPT_32ms];
	DWORD i, j, k;
	static const BYTE Balance[16] = {0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};
//
// Allocate satirically disabled EDs, and set head pointers for scheduling lists
//
	for (i=0; i < ED_INTERRUPT_32ms; i+) {
		ED = AllocateEndpointDescriptor (DeviceData);
		ED->PhysicalAddress = PhysicalAddressOf(&ED->HcED);
		DeviceData->EDList[i].PhysicalHead = &ED->HcED.NextED;
		ED->HcED.Control |= sKip; // mark ED as disabled
		InitializeListHead (&DeviceData->EDList[i].Head);
		StaticED[i] = ED;
		if (i > 0) {
			DeviceData->EDList[i].Next = (i-1)/2;
			ED->HcED.NextED = StaticED[(i-1)/2]->PhysicalAddress;
		} else {
			DeviceData->EDList[i].Next = ED_EOF;
			ED->HcEd.NextED = 0;
		}
	}
//
// Set head pointers for 32ms scheduling lists which start from HCCA
//
	for (i=0, j=ED_INTERRUPT_32ms, i<32; i++, j++) {
		DeviceData->EDList[j].PhysicalHead = &DeviceData->HCCA->InterruptList[i];
		InitializeListHead (&DeviceData->EDList[j].Head);
		k = Balance[i & 0xF] + ED_INTERRUPT_16ms;
		DeviceData->EDList[j].Next = k;
		DeviceData->HCCA->InterruptList[i] = StaticED[k]->PhysicalAddress;
	}
}

USB_STATUS OpenPipe (
	IN PHCD_ENDPOINT Endpoint
)
{
	DWORD WhichList, junk;
	PHCD_DEVICE_DATA DeviceData;
	DeviceData = Endpoint->DeviceData;
//
// Determine the scheduling period.
//
	WhichList = ED_INTERRUPT_32ms;
	while ( WhichList >= Endpoint->Rate && (WhichList >>= 1) ) continue;
//
// Commit this endpoints bandwidth to the proper scheduling slot
//
	if (WhichList == ED_ISOCHRONOUS) {
		DeviceData->EDList[ED_ISOCHRONOUS ].Bandwidth += Endpoint->Bandwidth;
		DeviceData->MaxBandwidthInUse += Endpoint->Bandwidth;
	} else {
//
// Determine which scheduling list has the least bandwidth
//
		CheckBandwidth(DeviceData, WhichList, &WhichList);
		DeviceData->EDList[WhichList].Bandwidth += Endpoint->Bandwidth;
//
// Recalculate the max bandwidth which is in use. This allows 1ms (isoc) pipe opens to
// occur with few calculation
//
		DeviceData->MaxBandwidthInUse =
		CheckBandwidth(DeviceData, ED_INTERRUPT_32ms, &junk);
	}

//
// Verify the new max has not overcomitted the USB
//
	if (DeviceData->MaxBandwidthInUse > DeviceData->AvailableBandwidth) {
//
// Too much, back this bandwidth out and fail the request
//
		DeviceData->EDList[WhichList].Bandwidth -= Endpoint->Bandwidth;
		DeviceData->MaxBandwidthInUse =
		CheckBandwidth(DeviceData, ED_INTERRUPT_32ms, &junk);
		return CAN_NOT_COMMIT_BANDWIDTH;
	}
//
// Assign endpoint to list and open pipe
//
	Endpoint->ListIndex = WhichList;
//
// Add to Host Controller schedule processing
//
	InsertEDForEndpoint (Endpoint);
}


DWORD CheckBandwidth (
	IN PHCD_DEVICE_DATA DeviceData,
	IN DWORD List,
	IN PDWORD BestList
)
/*++
This routine scans all the scheduling lists of frequency determined by the base List passed in and returns the
worst bandwidth found (i.e., max in use by any given scheduling list) and the list which had the least
bandwidth in use.
List - must be a base scheduling list. I.e., it must be one of ED_INTERRUPT_1ms, ED_INTERRUPT_2ms,
ED_INTERRUPT_4ms, ..., ED_INTERRUPT_32ms.
All lists of the appropriate frequency are checked
--*/
{
	DWORD LastList, Index;
	DWORD BestBandwidth, WorstBandwidth;
	WorstBandwidth = 0;
	BestBandwidth = ~0;
	for (LastList = List + List; List <= LastList; List ++) {
//
// Sum bandwidth in use in this scheduling time
//
		Bandwidth = 0;
		for (Index=List; Index != ED_EOF; Index = DeviceData->EDList[Index].Next) {
			Bandwidth += DeviceData->EDList[index].Bandwidth;
		}
//
// Remember best and worst
//
		if (Bandwidth < BestBandwidth) {
			BestBandwidth = Bandwidth;
			*BestList = List;
		}
		if (Bandwidth > WorstBandwidth) {
			WorstBandwidth = Bandwidth;
		}
	}
	return WorstBandwidth;
}

void UnscheduleIsochronousOrInterruptEndpoint (
	IN PHCD_ENDPOINT Endpoint,
	IN bool FreeED,
	IN bool EndpointProcessingRequired
)
{
	PHCD_DEVICE_DATA DeviceData;
	DeviceData = Endpoint->DeviceData;
	RemoveED(Endpoint, FreeED); // see sample code in Section 5.2.7.1.2.
	if (EndpointProcessingRequired) {
		DeviceData->HC-> HcInterruptEnable = HC_INT_SOF;// interrupt on next SOF
	}
}

/*
Status = SetEndpointPolicies (
Endpoint,
Isochronous, // Type
1, // Rate is 1ms
Bandwidth // BandwidthRequired
);
*/

bool QueueGeneralRequest (
IN PHCD_ENDPOINT endpoint;
IN USBD_REQUEST request;
)
{
	PHCD_DEVICE_DATA DeviceData;
	PHCD_ENDPOINT_DESCRIPTOR ED;
	PHCD_TRANSFER_DESCRIPTOR TD, LastTD = NULL;
	DWORD RemainingLength, count;
	BYTE * CurrentBufferPointer;
	DeviceData = endpoint->DeviceData;
	ED = endpoint->HcdED;
	if (ED == NULL || ED->ListIndex == ED_EOF) return(FALSE); // endpoint has been removed from schedule.
	FirstTD = TD = endpoint->HcdHeadP;
	request->Status = USBD_NOT_DONE;
	RemainingLength = request->BufferLength;
	request->BufferLength = 0; // report back bytes transferred so far
	CurrentBufferPointer = request->Buffer;
	InitializeListHead(&request->HcdList);
	if (endpoint->Type == Control) {
//
// Setup a TD for setup packet
//
		InsertTailList(&request->HcdList, &TD->RequestList);
		TD->UsbdRequest = request;
		TD->Endpoint = endpoint;
		TD->CancelPending = FALSE;
		TD->HcTD.CBP = PhysicalAddressOf(&request->setup[0]);
		TD->HcTD.BE = PhysicalAddressOf(&request->setup[7]);
		TD->TransferCount = 0;
		TD->HcTD.Control.DP = request->SETUP;
		TD->HcTD.Control.Toggle = 2;
		TD->HcTD.Control.R = TRUE;
		TD->HcTD.Control.IntD = 7; // specify no interrupt
		TD->HcTD.Control.CC = NotAccessed;
		TD->NextHcdTD = AllocateTransferDescriptor(DeviceData);
		TD->NextHcdTD->PhysicalAddress = TD->HcTd.NextTD =
		PhysicalAddressOf(&TD->NextHcdTD->HcTD);
		LastTD = TD;
		TD = TD->NextHcdTD;
}

//
// Setup TD(s) for data packets
//
	while (RemainingLength || (LastTD == NULL)) {
		InsertTailList(&request->HcdList, &TD->RequestList);
		TD->UsbdRequest = request;
		TD->Endpoint = endpoint;
		TD->CancelPending = FALSE;
		if (RemainingLength) {
			TD->HcTD.CBP = PhysicalAddressOf(CurrentBufferPointer);
			count = 0x00002000 - (TD->HcTD.CBP & 0x00000FFF);
			if (count < RemainingLength) {
				count -= count % endpoint->MaxPacket;
			} else {
				count = RemainingLength;
			}
			CurrentBufferPointer += count - 1;
			TD->HcTD.BE = PhysicalAddressOf(CurrentBufferPointer++);
		} else {
			TD->HcTD.CBP = TD->HcTD.BE = count = 0;
		}
		TD->TransferCount = count;
		TD->HcTD.Control.DP = request->XferInfo;
		if (endpoint->Type == Control) {
			TD->HcTD.Control.Toggle = 3;
		} else {
			TD->HcTD.Control.Toggle = 0;
		}
		if (RemainingLength -= count && !request->ShortXferOk) {
			TD->HcTD.Control.R = TRUE;
		} else {
			TD->HcTD.Control.R = FALSE;
		}
		TD->HcTD.Control.IntD = 7; // specify no interrupt
		TD->HcTD.Control.CC = NotAccessed;
		TD->NextHcdTD = AllocateTransferDescriptor(DeviceData);
		TD->NextHcdTD->PhysicalAddress = TD->HcTd.NextTD =
		PhysicalAddressOf(&TD->NextHcdTD->HcTD);
		LastTD = TD;
		TD = TD->NextHcdTD;
	}

	if (endpoint->Type == Control) {
//
// Setup TD for status phase
//
		InsertTailList(&request->HcdList, &TD->RequestList);
		TD->Endpoint = endpoint;
		TD->UsbdRequest = request;
		TD->CancelPending = FALSE;
		TD->HcTD.CBP = 0;
		TD->HcTD.BE = 0;
		TD->TransferCount = 0;
		if (XferInfo == IN) {
			TD->HcTD.Control.DP = OUT;
		} else {
			TD->HcTD.ControlDP = IN:
		}
		TD->HcTD.Control.Toggle = 3;
		TD->HcTD.Control.R = FALSE;
		TD->HcTD.Control.IntD = 7; // specify no interrupt
		TD->HcTD.Control.CC = NotAccessed;
		TD->NextHcdTD = AllocateTransferDescriptor(DeviceData);
		TD->NextHcdTD->PhysicalAddress = TD->HcTd.NextTD =
		PhysicalAddressOf(&TD->NextHcdTD->HcTD);
		LastTD = TD;
		TD = TD->NextHcdTD;
	}
	//
	// Setup interrupt delay
	//
	LastTD->HcTD.Control.IntD = min(6, request->MaxIntDelay);
//
// Set new TailP in ED
//
	TD->UsbdRequest = NULL;
	endpoint->HcdTailP = TD;
	ED->HcED.TailP = TD->PhysicalAddress;
	switch (endpoint->Type) {
		case Control:
			DeviceData->HC->HcCommandStatus = ControlListFilled;
			break;
		case Bulk:
			DeviceData->HC->HcCommandStatus = BulkListFilled;
	}
	return(TRUE);
}

bool CancelRequest (
	IN PUSBD_REQUEST request,
) {
	PHCD_TRANSFER_DESCRIPTOR TD;
	PHCD_ENDPOINT endpoint
	//
	// If request status is ‘not done’ set status to ‘canceling’
	//
	if (request->Status != UDBD_NOT_DONE) return FALSE; // cannot cancel a completed request
	request->Status = USBD_CANCELING;
	TD = CONTAINING_RECORD(
		request->HcdList.FLink,
		HCD_TRANSFER_DESCRIPTOR,
		RequestList
	);
	while (TRUE) {
		TD->CancelPending = TRUE;
		if (TD->RequestList.FLink == request->HcdList.BLink) break;
		TD = CONTAINING_RECORD(
			TD->RequestList.FLink,
			HCD_TRANSFER_DESCRIPTOR,
			RequestList
		);
	}
	endpoint = TD->Endpoint;
	PauseED(endpoint); // stop endpoint, schedule cleanup (see Section 5.2.7.1.3)
	return TRUE;
}

void ProcessDoneQueue (
	DWORD physHcTD// HccaDoneHead
)
{
	PHCD_TRANSFER_DESCRIPTOR TD, tn, TDlist = NULL;
	PUSBD_REQUEST Request;
	PHCD_ENDPOINT Endpoint;
//
// Reverse the queue passed from controller while virtualizing addresses.
// NOTE: The following code assumes that a DWORD and a pointer are the same size
//
	if (physHcTD == 0) return;
	do {
		TD = CONTAINING_RECORD(	VirtualAddressOf(physHcTD),	HCD_TRANSFER_DESCRIPTOR,	HcTD );
		physHcTD = TD->HcTD.NextTD;
		TD->HcTD.NextTD = (DWORD) TDlist;
		TDlist = TD;
	} while (physHcTD);
//
// Process list that is now reordered to completion order
//
	while (TDlist != NULL) {
		TD = TDlist;
		TDlist = (PHCD_TRANSFER_DESCRIPTOR) (TD->HcTD.NextTD);
		if (TD->Status == TD_CANCELED) {
			FreeTransferDescriptor(TD);
			continue;
		}
		Request = TD->UsbdRequest;
		Endpoint = TD->Endpoint;
		if (Endpoint->Type != Isochronous) {
			if (TD->HcTD.CBP) {
				TD->TransferCount -=
				(((TD->HcTD.BE ^ TD->HcTD.CBP) & 0xFFFFF000) ? 0x00001000 : 0) +
				(TD->HcTD.BE & 0x00000FFF) - (TD->HcTD.CBP & 0x00000FFF) + 1;
			}
			if (TD->HcTD.Control.DP != Setup ) {
				Request->BufferLength += TD->TransferCount;
			}
			if (TD->HcTD.Control.CC == NoError) {
	//
	// TD completed without error, remove it from USBD_REQUEST list,
	// if USBD_REQUEST list is now empty, then complete the request.
	//
				Endpoint->HcdHeadP = TD->NextHcdTD;
				RemoveListEntry(&TD->RequestList);
				FreeTransferDescriptor(TD);
				if (IsListEmpty(&Request->HcdList)) {
					if (Request->Status != USBD_CANCELING)
					Request->Status = USBD_NORMAL_COMPLETION;
					else
					Request->Status = USBD_CANCELED;
					CompleteUsbdRequest(Request);
				}
			} else { // error
		//
		// TD completed with an error, remove it and other TDs for same request,
		// set appropriate status in USBD_REQUEST and then complete it. There
		// are two special cases: 1) error is DataUnderun on Bulk or Interrupt and
		// ShortXferOk is true; for this do not report error to USBD and restart
		// endpoint. 2) error is DataUnderrun on Control and ShortXferOk is true;
		// for this the final status TD for the Request should not be canceled, the
		// Request should not be completed, and the endpoint should be restarted.
		// NOTE: The endpoint has been halted by the controller
		//
				for ( tn = Endpoint->HcdHeadP;	tn != Endpoint->HcdTailP;	tn = tn->NextHcdTD ) {
					if (Request != tn->UsbdRequest ||
					( TD->HcTD.Control.CC == DataUnderrun &&
					Request->ShortXferOk &&
					Request->Status != USBD_CANCELING &&
					TD->HcTD.Control.DP != tn->HcTD.Control.DP))
						break;
				}
				Endpoint->HcdHeadP = tn;
				Endpoint->HcdED->HcED.HeadP = tn->PhysicalAddress |
				(Endpoint->HcED->HcED.HeadP &
				(HcEDHeadP_HALT | HcEDHeadP_CARRY));
				while ((tn = CONTAINING_RECORD(		RemoveListHead(&Request->HcdList), HCD_TRANSFER_DESCRIPTOR,	RequestList )) != NULL) {
					if (tn != TD && tn != Endpoint->HcdHeadP)	FreeTransferDescriptor(tn);
				}
				if (Endpoint->HcdHeadP->UsbdRequest == Request) {
					InsertTailList(&Request->HcdList,	&Endpoint->HcdHeadP->RequestList);
					Endpoint->HcdED->HcED.HeadP &= ~HcEDHeadP_HALT;
				} else {
					if (Request->ShortXferOk && (TD->HcTD.Control.CC == DataUnderrun)) {
						if (Request->Status != USBD_CANCELING)
							Request->Status = USBD_NORMAL_COMPLETION;
						else
							Request->Status = USBD_CANCELED;
						Endpoint->HcdED->HcED.HeadP &= ~HcEDHeadP_HALT;
					} else if (Request->Status != USBD_CANCELING) {
						Request->Status = USBD_CC_Table[TD->HcTD.Control.CC];
					} else {
						Request->Status = USBD_CANCELED;
					}
					CompleteUsbdRequest(Request);
				}
				FreeTransferDescriptor(TD);
			}
		} else {
	//
	// Code for Isochronous is left as an exercise to the reader
	//
		}
	}
}

bool HcdInterruptService( IN HCD_DEVICE_DATA DeviceData ) {
	// define some variables
	DWORD ContextInfo, Temp, Temp2, Frame;
//
// Is this our interrupt?
//
	if (DeviceData->HCCA->HccaDoneHead != 0) {
		ContextInfo = WritebackDoneHead; // note interrupt processing required
		if (DeviceData->HCCA->DoneHead & 1) {
			ContextInfo |= DeviceData->HC->HcInterruptStatus & DeviceData->HC->HcInterruptEnable;
		}
	} else {
		ContextInfo = DeviceData->HC->HcInterruptStatus &
		DeviceData->HC->HcInterruptEnable;
		if (ContextInfo == 0) return FALSE; // not my interrupt
	}
//
// It is our interrupt, prevent HC from doing it to us again until we are finished
//
	DeviceData->HC->HcInterruptDisable = MasterInterruptEnable;
	if (ContextInfo & UnrecoverableError) {
//
// The controller is hung, maybe resetting it can get it going again. But that code is left as an exercise to
// the reader.
//
	}

	if (ContextInfo & (SchedulingOverrun | WritebackDoneHead | StartOfFrame | FrameNumberOverflow)) ContextInfo |= MasterInterruptEnable; // flag for end of frame type interrupts
//
// Check for Schedule Overrun
//
	if (ContextInfo & SchedulingOverrun) {
		Frame = Get32BitFrameNumber(DeviceData);
		Temp2 = DeviceData->HC->HcCommandStatus & EFC_Mask;
		Temp = Temp2 - (DeviceData->SOCount & EFC_Mask);
		Temp = (((Temp >> EFC_Position) - 1) % EFC_Size) + 1;// number of bad frames since last error
		if ( !(DeviceData->SOCount & SOC_Mask) ||// start a new count?
			((DeviceData->SOCount & SOC_Mask) + DeviceData->SOStallFrame + Temp) != Frame) {
			DeviceData->SOLimitFrame = DeviceData->SOStallFrame = Frame - Temp;
			DeviceData->SOCount = Temp | Temp2;
		} else { // got a running count
			DeviceData->SOCount = (DeviceData->SOCount + Temp) & SOC_Mask | Temp2;
			while (Frame - DeviceData->SOLimitFrame >= 100) {
				DeviceData->SOLimitHit++;
				DeviceData->SOLimitFrame += 100;
			}
			if (Frame - DeviceData->SOStallFrame >= 32740) {
				DeviceData->HC->HcControl &= ~IsochronousEnable;// stop isochronous transfers
				DeviceData->SOStallHit = TRUE;
				DeviceData->SOCount = Temp2; // clear error counter
			}
		}
		DeviceData->HC->HcInterruptStatus = SchedulingOverrun;// acknowledge interrupt
		ContextInfo &= ~SchedulingOverrun;
	} else { // no schedule overrun, check for good frame.
		if (ContextInfo & MasterInterruptEnable)
		DeviceData->SOCount &= EFC_MASK;// clear counter
	}
//
// Check for Frame Number Overflow
// Note: the formula below prevents a debugger break from making the 32-bit frame number run backward.
//
	if (ContextInfo & FrameNumberOverflow) {
		DeviceData->FrameHighPart += 0x10000 -((
		DeviceData->HCCA->HccaFrameNumber ^ DeviceData->FrameHighPart) & 0x8000);
		DeviceData->HC->HcInterruptStatus = FrameNumberOverflow;// acknowledge interrupt
		ContextInfo &= ~FrameNumberOverflow;
	}
//
// Processor interrupts could be enabled here and the interrupt dismissed at the interrupt
// controller, but for simplicity this code won’t.
//

	if (ContextInfo & ResumeDetected) {
//
// Resume has been requested by a device on USB. HCD must wait 20ms then put controller in the
// UsbOperational state. This code is left as an exercise to the reader.
//
		ContextInfo &= ~ResumeDetected;
		DeviceData->HC->HcInterruptStatus = ResumeDetected;
	}
//
// Process the Done Queue
//
	if (ContextInfo & WritebackDoneHead) {
		ProcessDoneQueue(DeviceData ->HccaDoneHead);
//
// Done Queue processing complete, notify controller
//
		DeviceData->HCCA->HccaDoneHead = 0;
		DeviceData->HC->HcInterruptStatus = WritebackDoneHead;
		ContextInfo &= ~WritebackDoneHead;
	}
//
// Process Root Hub changes
//
	if (ContextInfo & RootHubStatusChange) {
//
// EmulateRootHubInterruptXfer will complete a HCD_TRANSFER_DESCRIPTOR which
// we then pass to ProcessDoneQueue to emulate an HC completion
//
		ProcessDoneQueue(EmulateRootHubInterruptXfer(DeviceData)->PhysicalAddress);
//
// leave RootHubStatusChange set in ContextInfo so that it will be masked off (it won’t be unmasked
// again until another TD is queued for the emulated endpoint)
//
	}
	if (ContextInfo & OwnershipChange) {
//
// Only SMM drivers need implement this. See Sections 5.1.1.3.3 and 5.1.1.3.6 for descriptions of what
// the code here must do.
//
	}

//
// Any interrupts left should just be masked out. (This is normal processing for StartOfFrame and
// RootHubStatusChange)
//
	if (ContextInfo & ~MasterInterruptEnable) // any unprocessed interrupts?
		DeviceData->HC->HcInterruptDisable = ContextInfo;// yes, mask them

//
// We’ve completed the actual service of the HC interrupts, now we must deal with the effects
//
//
// Check for Scheduling Overrun limit
//
	if (DeviceData->SOLimitHit) {
		do {
			PHCD_ENDPOINT_DESCRIPTOR ED;
			if (IsListEmpty(EDList[ED_ISOCHRONOUS].Head)) break; // Isochronous List is empty
			ED = CONTAINING_RECORD(
				EDList[ED_ISOCHRONOUS].Head.Blink,
				HCD_ENDPOINT_DESCRIPTOR,
				Link
			);
			if (ED->Endpoint->Type != Isochronous) break; // Only 1ms Interrupts left on list
			DeviceData->AvailableBandwidth = DeviceData->MaxBandwidthInUse - 64;
//
// It is recommended that the above bandwidth be saved in non-volatile memory for future use.
//
			RemoveED(ED->Endpoint);
		} while (--DeviceData->SOLimitHit);
		DeviceData->SOLimitHit = 0;
	}
//
// look for things on the PausedEDRestart list
//
	Frame = Get32BitFrameNumber(DeviceData);
	while (!IsListEmpty(&DeviceData->PausedEDRestart) {
		PHCD_ENDPOINT_DESCRIPTOR ED;
		ED = CONTAINING_RECORD( DeviceData->PausedEDRestart.FLink, HCD_ENDPOINT_DESCRIPTOR, PausedLink);
		if ((LONG)(ED->ReclaimationFrame - Frame) > 0) break;
		RemoveListEntry(&ED->PausedLink);
		ED->PausedFlag = FALSE;
		ProcessPausedED(ED);
	}

//
// look for things on the StalledEDReclamation list
//
	if (ContextInfo & MasterInterruptEnable && !IsListEmpty(&DeviceData->StalledEDReclamation) {
		Temp = DeviceData->HC->HcControlCurrentED;
		Temp2 = DeviceData->HC->HcBulkCurrentED;
		while (!IsListEmpty(&DeviceData->StalledEDReclamation) {
			PHCD_ENDPOINT_DESCRIPTOR ED;
			ED = CONTAINING_RECORD( DeviceData->StalledEDReclamation.FLink, HCD_ENDPOINT_DESCRIPTOR, Link);
			RemoveListEntry(&ED->Link);
			if (ED->PhysicalAddress == Temp)
				DeviceData->HC->HcControlCurrentED = Temp = ED->HcED.NextED;
			else if (ED->PhysicalAddress == Temp2) DeviceData->HC->HcBulkCurrentED = Temp2 = ED->HcED.NextED;
			if (ED->Endpoint != NULL) {
				ProcessPausedED(ED); // cancel any outstanding TDs
			} else {
				FreeEndpointDescriptor(ED);
			}
		}
		DeviceData->HC->HcControl |= ControlListEnable | BulkListEnable;// restart queues
	}
//
// look for things on the RunningEDReclamation list
//
	Frame = Get32BitFrameNumber(DeviceData);
	while (!IsListEmpty(&DeviceData->RunningEDReclamation) {
		PHCD_ENDPOINT_DESCRIPTOR ED;
		ED = CONTAINING_RECORD( DeviceData->RunningEDReclamation.FLink, HCD_ENDPOINT_DESCRIPTOR, Link);
		if ((LONG)(ED->ReclaimationFrame - Frame) > 0) break;
		RemoveListEntry(&ED->Link);
		if (ED->Endpoint != NULL) ProcessPausedED(ED); // cancel any outstanding TDs
		else
			FreeEndpointDescriptor(ED);
	}
//
// If processor interrupts were enabled earlier then they must be disabled here before we re-enable
// the interrupts at the controller.
//
	DeviceData->HC->HcInterruptEnable = MasterInterruptEnable;
	return TRUE;
}


DWORD SetFrameInterval (
	IN PHCD_DEVICE_DATA DeviceData,
	IN bool UpNotDown
) {
	DWORD FrameNumber, Interval;
	Interval |= (DeviceData->HC->HcFmInterval & 0x00003FFF);
	if (UpNotDown) ++Interval; else --Interval;
	Interval |= (((Interval - MAXIMUM_OVERHEAD) * 6) / 7) << 16;
	Interval |= 0x80000000 & (0x80000000 ^ (DeviceData->HC->HcFmRemaining));
	FrameNumber = Get32BitFrameNumber(DeviceData);
	DeviceData->HC->HcFmInterval = Interval;
	if (0x80000000 & (DeviceData->HC->HcFmRemaining ^ Interval)) {
		FrameNumber += 1);
	} else {
		FrameNumber = Get32BitFrameNumber(DeviceData);
	}
	return (FrameNumber); // return frame number new interval takes effect
}


DWORD Get32BitFrameNumber(HCD_DEVICE_DATA DeviceData)
{
	DWORD fm, hp;
//
// This code accounts for the fact that HccaFrameNumber is updated by the HC before the HCD gets an
// interrupt that will adjust FrameHighPart.
//
	hp = DeviceData->FrameHighPart;
	fm = DeviceData->HCCA->HccaFrameNumber;
	return ((fm & 0x7FFF) | hp) + ((fm ^ hp) & 0x8000);
}

#endif
