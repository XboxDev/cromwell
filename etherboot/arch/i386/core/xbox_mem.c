#include "etherboot.h"
#include "pci.h"

extern struct pci_driver forcedeth_driver;

static struct meminfo meminfo = 
{
	640,
	0,
	59*1024,
	4,
	{
		{0x0000000000000000,0x000000000009f000,E820_RAM},
		{0x000000000009f000,0x0000000000061000,E820_RESERVED},
		{0x0000000000100000,0x0000000003B00000,E820_RAM},
		{0x0000000003C00000,0x0000000000400000,E820_RESERVED}
	}
};

static unsigned long virt_offset = 0;

struct pci_driver* pci_drivers = &forcedeth_driver;
struct pci_driver* pci_drivers_end = &forcedeth_driver + 1;
