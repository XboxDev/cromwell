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


#include "boot.h"
#include "config-rom.h"
#include "BootEEPROM.h"

#include "nvn/basetype.h"
#include "nvn/adapter.h"
#include "nvn/os.h"
#include "nvn/nvnet.h"



#ifdef DO_ETHERNET

extern EEPROMDATA eeprom;

#define DEBUG_ETH 1

void dev_skb_destructor(struct sk_buff * skb)
{
	printk("dev_skb_destructor\n");
	free(skb);
}


struct sk_buff * dev_alloc_skb(CROMWELL_DEVICE_DESCRIPTOR *pcromwelldevice, int nMemHeadroom)
{
	struct sk_buff *skb = malloc(sizeof(skb) + 16 + nMemHeadroom);
	if(skb!=NULL) {
		skb->next=NULL;
		skb->prev=NULL;
		skb->list=&pcromwelldevice->skbuffhead;
		skb->sk=NULL;
		skb->dev=pcromwelldevice;
		skb->h.raw=NULL;
		skb->nh.raw=NULL;
		skb->mac.raw=NULL;
		skb->dst=NULL;
		skb->len=0;
		skb->csum=0;
		skb->used=0;
		skb->cloned=0;
		skb->pkt_type=0;
		skb->ip_summed=0;
		skb->priority=0;
		skb->users=1;
		skb->protocol=0;
		skb->security=0;
		skb->truesize=sizeof(skb) + 16 + nMemHeadroom;
		skb->head=((BYTE *)skb) + sizeof(skb);
		skb->data=skb->head;
		skb->tail=skb->head;
		skb->end=skb->head+16 + nMemHeadroom;
		skb->destructor=dev_skb_destructor;

		if(pcromwelldevice->skbuffhead.next==NULL) {
			pcromwelldevice->skbuffhead.next=skb;
		}
	}
	return skb;
}


// these are service we have to provide from our "Operating System"

int BootEthernetMemoryAlloc(PVOID pOSCX, PMEMORY_BLOCK pMem) {
	pMem->pLogical=pMem->pPhysical=malloc(pMem->uiLength);
	printk("BootEthernetMemoryAlloc(0x%x) -> %x\n", pMem->uiLength, pMem->pLogical);
	return 1;
}

int BootEthernetMemoryFree(PVOID pOSCX, PMEMORY_BLOCK pMem) {
#if DEBUG_ETH
	printk("BootEthernetMemoryFree()\n");
#endif
	free(pMem->pLogical);
	return 1;
}
int BootEthernetClearMemory(PVOID pOSCX, PVOID pMem, int iLength) {
#if DEBUG_ETH
	printk("BootEthernetClearMemory(p=%x, len=%x)\n", (int)pMem, iLength);
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
	printk("BootEthernetAllocRxBuffer(uiLength=%x).. ", pMem->uiLength);
#endif
		CROMWELL_DEVICE_DESCRIPTOR *pcromwelldevice   = (CROMWELL_DEVICE_DESCRIPTOR *)pOSCX;
    struct nvnet_private *ppriv = (struct nvnet_private *)pcromwelldevice->m_ppriv;
    struct sk_buff *skb;
    struct nvnet_rx_info *info;

    skb = dev_alloc_skb(pcromwelldevice, MAX_PACKET_SIZE + 2 + sizeof(struct nvnet_rx_info));
    if(!skb)
    {
        printk("BootEthernetAllocRxBuffer - skb allocation failed\n");
        return 0;
    }

    info = (struct nvnet_rx_info *)((BYTE *)skb->tail + 2 + MAX_PACKET_SIZE);

    info->skb      = skb;
    info->uiLength = MAX_PACKET_SIZE;

    pMem->pLogical = (void *)skb;
    pMem->uiLength = MAX_PACKET_SIZE;
    *ppvID           = (void *)info;

		/*
    info->dma  = pci_map_single(m_ppriv->pdev, skb->tail, MAX_PACKET_SIZE, PCI_DMA_FROMDEVICE);
    mem->pPhysical = (void *)cpu_to_le32(info->dma);
		*/
		 pMem->pPhysical = pMem->pLogical;
#if DEBUG_ETH
	printk("Done\n ");
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
	printk("BootEthernetLockAlloc(type =%d)\n", iLockType);
#endif
	ppvLock=malloc(sizeof(struct spinlock));
	((struct spinlock *)*(int *)ppvLock)->m_fAcquired=false;
	return 1;
}
int BootEthernetLockAcquire (PVOID pOSCX, int iLockType, PVOID pvLock) {
#if DEBUG_ETH
	printk("BootEthernetLockAcquire(type=%d, initial=%d)\n", iLockType, ((struct spinlock *)*(int *)pvLock)->m_fAcquired);
#endif
	((struct spinlock *)*(int *)pvLock)->m_fAcquired=true;
	return 1;
}
int BootEthernetLockRelease (PVOID pOSCX, int iLockType, PVOID pvLock) {
#if DEBUG_ETH
	printk("BootEthernetLockRelease(type=%d, initial=%d)\n", iLockType, ((struct spinlock *)*(int *)pvLock)->m_fAcquired);
#endif
	((struct spinlock *)*(int *)pvLock)->m_fAcquired=false;
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
	cromwellDeviceDescriptor.m_ppriv = &priv;
	cromwellDeviceDescriptor.skbuffhead.prev=NULL;
	cromwellDeviceDescriptor.skbuffhead.next=NULL;

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

	printk("AdapterOpen returned %d\n", n);

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
