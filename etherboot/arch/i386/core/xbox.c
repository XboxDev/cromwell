#include "etherboot.h"
#include "pci.h"
#include "nic.h"

extern struct pci_driver forcedeth_driver;
extern uint8_t PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint16_t PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint32_t PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern void PciWriteByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint8_t value);
extern void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint16_t value);
extern uint32_t PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint32_t value);
extern void BootResetAction(void);
extern void BootStopUSB(void);

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

int loadkernel (const char *fname)
{
	/* For test purpose only */
	while(1);
}

int etherboot(void)
{
	struct dev* dev = &nic.dev;
	print_config();
	if (eth_probe(dev) == -1)
	{
		printk("eth_probe failed\n");
	}
	else 
	{
		if (eth_load_configuration(dev) != 0)
		{
			printk("eth_load_configuration failed\n");
		}
		else
		{
			eth_load(dev);
		}
	}
}

int pcibios_read_config_byte(unsigned int bus, unsigned int device_fn, unsigned int where, uint8_t *value)
{
	*value = PciReadByte(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where);
	return 0;
}
int pcibios_read_config_word(unsigned int bus, unsigned int device_fn, unsigned int where, uint16_t *value)
{
	*value = PciReadWord(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where);
	return 0;
}
int pcibios_read_config_dword(unsigned int bus, unsigned int device_fn, unsigned int where, uint32_t *value)
{
	*value = PciReadDword(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where);
	return 0;
}

int pcibios_write_config_byte(unsigned int bus, unsigned int device_fn, unsigned int where, uint8_t value)
{
	PciWriteByte(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where, value);
	return 0;
}

int pcibios_write_config_word(unsigned int bus, unsigned int device_fn, unsigned int where, uint16_t value)
{
	PciWriteWord(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where, value);
	return 0;
}

int pcibios_write_config_dword(unsigned int bus, unsigned int device_fn, unsigned int where, uint32_t value)
{
	PciWriteDword(bus, PCI_SLOT(device_fn), PCI_FUNC(device_fn), where, value);
	return 0;
}

unsigned long pcibios_bus_base(unsigned int bus __unused)
{
	/* architecturally this must be 0 */
	return 0;
}

void find_pci(int type, struct pci_device *dev)
{
	return scan_pci_bus(type, dev);
}

static __inline uint32_t IoInputDword(uint16_t wAds) {
  uint32_t _v;

  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

unsigned long currticks(void)
{
	return IoInputDword(0x8008);
}

void ndelay(unsigned int nsecs)
{
	wait_us(nsecs * 1000);
}

void udelay(unsigned int usecs)
{
	wait_us(usecs);
}

void restart_etherboot(int status)
{
	eth_disable();
	BootStopUSB();
	BootResetAction();
}

