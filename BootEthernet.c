/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-01-02 andy@warmcat.com  Created

 */

// killed temporarily to allow clean CVS checkin


#include "nvn/basetype.h"
#include "nvn/adapter.h"
#include "nvn/os.h"
#include "nvn/nvnet.h"

#define bool_already_defined_
#include "boot.h"
#include "BootEEPROM.h"

#ifdef DO_ETHERNET

extern EEPROMDATA eeprom;

#define DEBUG_ETH 0


typedef struct {
	char szName32[32];
} CROMWELL_DEVICE_DESCRIPTOR;


// these are service we have to provide from our "Operating System"

int BootEthernetMemoryAlloc(PVOID pOSCX, PMEMORY_BLOCK pMem) {
//	printk("BootEthernetMemoryAlloc(0x%x)\n", pMem->uiLength);
	pMem->pLogical=pMem->pPhysical=malloc(pMem->uiLength);
	return 1;
}
int BootEthernetMemoryFree(PVOID pOSCX, PMEMORY_BLOCK pMem) {
#if DEBUG_ETH
	printk("BootEthernetMemoryFree()\n");
#endif
	return 1;
}
int BootEthernetClearMemory(PVOID pOSCX, PVOID pMem, int iLength) {
#if DEBUG_ETH
//	printk("BootEthernetClearMemory()\n");
#endif
	memset(pMem, 0, iLength);
	return 1;

}
int BootEthernetStallExecution(PVOID pOSCX, ULONG ulTimeInMicroseconds) {
#if DEBUG_ETH
	printk("BootEthernetStallExecution()\n");
#endif
	return 1;
}
int BootEthernetAllocRxBuffer(PVOID pOSCX, PMEMORY_BLOCK pMem, PVOID *ppvID) {
#if DEBUG_ETH
	printk("BootEthernetAllocRxBuffer()\n");
#endif
	return 1;
}
int BootEthernetFreeRxBuffer (PVOID pOSCX, PMEMORY_BLOCK pMem, PVOID pvID) {
#if DEBUG_ETH
	printk("BootEthernetFreeRxBuffer()\n");
#endif
	return 1;
}
int BootEthernetPacketWasSent (PVOID pOSCX, PVOID pvID, ULONG ulSuccess) {
#if DEBUG_ETH
	printk("BootEthernetPacketWasSent()\n");
#endif
	return 1;
}
int BootEthernetPacketWasReceived (PVOID pOSCX, PVOID pvADReadData, ULONG ulSuccess) {
#if DEBUG_ETH
	printk("BootEthernetPacketWasReceived()\n");
#endif
	return 1;
}
int BootEthernetLinkStateHasChanged (PVOID pOSCX, int nEnabled) {
#if DEBUG_ETH
	printk("BootEthernetLinkStateHasChanged()\n");
#endif
	return 1;
}
#ifdef AMDHPNA10
int BootEthernetPeriodicTimer (PVOID pOSCX, PVOID pPhyTimerApi) {
#if DEBUG_ETH
	printk("BootEthernetPeriodicTimer()\n");
#endif
	return 1;

}
#endif
int BootEthernetAllocateTimer (PVOID pvContext, PVOID *ppvTimer) {
#if DEBUG_ETH
	printk("BootEthernetAllocateTimer()\n");
#endif
	return 1;
}
int BootEthernetFreeTimer (PVOID pvContext, PVOID pvTimer) {
#if DEBUG_ETH
	printk("BootEthernetFreeTimer()\n");
#endif
	return 1;
}
int BootEthernetInitTimer (PVOID pvContext, PVOID pvTimer, PTIMER_FUNC pvFunc, PVOID pvFuncParameter) {
#if DEBUG_ETH
	printk("BootEthernetInitTimer()\n");
#endif
	return 1;
}
int BootEthernetSetTimer (PVOID pvContext, PVOID pvTimer, ULONG dwMillisecondsDelay) {
#if DEBUG_ETH
	printk("BootEthernetSetTimer()\n");
#endif
	return 1;
}
int BootEthernetCancelTimer (PVOID pvContext, PVOID pvTimer) {
#if DEBUG_ETH
	printk("BootEthernetCancelTimer()\n");
#endif
	return 1;
}
int BootEthernetPreprocessPacket (PVOID pvContext, PVOID pvADReadData, ULONG ulSuccess, PVOID *ppvID, ULONG *pulStatus) {
#if DEBUG_ETH
	printk("BootEthernetPreprocessPacket()\n");
#endif
	return 1;
}
int BootEthernetIndicatePackets (PVOID pvContext, PVOID *ppvID, ULONG *pulStatus, ULONG ulNumPacket) {
#if DEBUG_ETH
	printk("BootEthernetIndicatePackets()\n");
#endif
	return 1;
}
int BootEthernetLockAlloc (PVOID pOSCX, int iLockType, PVOID *ppvLock) {
#if DEBUG_ETH
	printk("BootEthernetLockAlloc()\n");
#endif
	return 1;
}
int BootEthernetLockAcquire (PVOID pOSCX, int iLockType, PVOID pvLock) {
#if DEBUG_ETH
	printk("BootEthernetLockAcquire()\n");
#endif
	return 1;
}
int BootEthernetLockRelease (PVOID pOSCX, int iLockType, PVOID pvLock) {
#if DEBUG_ETH
	printk("BootEthernetLockRelease()\n");
#endif
	return 1;
}


///////////////////////////////////////////
//
//

typedef struct pci_dev PCI_DEV;
typedef struct nvnet_private NVNET_PRIVATE;

int BootStartUpEthernet()
{
	int n;
	BYTE realmac[6];
	ADAPTER_API adapterapi;
	NVNET_PRIVATE priv;
	OS_API cromwellapi;
	CROMWELL_DEVICE_DESCRIPTOR cromwellDeviceDescriptor;

	cromwellapi.pOSCX=&cromwellDeviceDescriptor; // confirmed action from nvnet.c

	cromwellapi.pfnAllocMemory=BootEthernetMemoryAlloc;
  cromwellapi.pfnFreeMemory=BootEthernetMemoryFree;
  cromwellapi.pfnClearMemory=BootEthernetClearMemory;
  cromwellapi.pfnAllocMemoryNonCached=BootEthernetMemoryAlloc;
  cromwellapi.pfnFreeMemoryNonCached=BootEthernetMemoryFree;
  cromwellapi.pfnStallExecution=BootEthernetStallExecution;
  cromwellapi.pfnAllocReceiveBuffer=BootEthernetAllocRxBuffer;
  cromwellapi.pfnFreeReceiveBuffer=BootEthernetFreeRxBuffer;
  cromwellapi.pfnPacketWasSent=BootEthernetPacketWasSent;
  cromwellapi.pfnPacketWasReceived=BootEthernetPacketWasReceived;
  cromwellapi.pfnLinkStateHasChanged=BootEthernetLinkStateHasChanged;
	cromwellapi.pfnAllocTimer=BootEthernetAllocateTimer;
	cromwellapi.pfnFreeTimer=BootEthernetFreeTimer;
	cromwellapi.pfnInitializeTimer=BootEthernetInitTimer;
	cromwellapi.pfnSetTimer=BootEthernetSetTimer;
	cromwellapi.pfnCancelTimer=BootEthernetCancelTimer;
#ifdef AMDHPNA10
  cromwellapi.pfnPeriodicTimer=BootEthernetPeriodicTimer;
#endif
  cromwellapi.pfnPreprocessPacket=BootEthernetPreprocessPacket;
  cromwellapi.pfnIndicatePackets=BootEthernetIndicatePackets;
	cromwellapi.pfnLockAlloc=BootEthernetLockAlloc;
	cromwellapi.pfnLockAcquire=BootEthernetLockAcquire;
	cromwellapi.pfnLockRelease=BootEthernetLockRelease;

//	priv.pdev=&pcidev;
//	priv.next = NULL;
	priv.regs=(void *)0xfef00000;
	priv.hwapi=&adapterapi;
	priv.linuxapi=cromwellapi;
	priv.hpna=1;
	priv.phyaddr=0xe000;
	priv.initacpi=0; // guess
	priv.linkup=0;
	priv.linkspeed=2; // 0=autoneg, 1=10Mbps, 2=100Mpbs
	priv.fullduplex=1;
	priv.phyiso=1;
	priv.promisc=1;
	priv.power=POWER_STATE_ALL;
	priv.lockflags=0;

//	memset(&priv.stats, 0, sizeof(ADAPTER_STATS));

#if DEBUG_ETH
	printk("Eth: Initializing\n");
#endif

	n = ADAPTER_Open(
		&cromwellapi,
		priv.regs,
		400,  // requested poll interval in mS
		(void **)&priv.hwapi,
		&priv.hpna,
		&priv.phyaddr
	);

        realmac[0] = eeprom.MACAddress[5];
        realmac[1] = eeprom.MACAddress[4];
        realmac[2] = eeprom.MACAddress[3];
        realmac[3] = eeprom.MACAddress[2];
        realmac[4] = eeprom.MACAddress[1];
        realmac[5] = eeprom.MACAddress[0];
        priv.hwapi->pfnSetNodeAddress(priv.hwapi->pADCX, realmac);
								
/*
	int n=PHY_Open(&baOsStruct[0], &phyapi, &ulIsHPNAPhy, &ulPhyAddr, &ulPhyConnected);

	printk("Eth: n=%d, phyapi.pPHYCX=%X\nulIsHPNAPhy=%x, ulPhyAddr=%x, ulPhyConnected=%x\n",
		n, phyapi.pPHYCX, ulIsHPNAPhy, ulPhyAddr, ulPhyConnected
	);
*/

#if DEBUG_ETH
	printk("Eth: n=%d\n", n);
#endif

	return n;
}

#endif
