#include "etherboot.h"
#include "pci.h"

extern uint8_t PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint16_t PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint32_t PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern void PciWriteByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint8_t value);
extern void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint16_t value);
extern uint32_t PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint32_t value);

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
