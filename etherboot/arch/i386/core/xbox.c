#include "etherboot.h"
#include "pci.h"
#include "nic.h"

typedef unsigned char BYTE;
#include "BootParser.h"

extern struct pci_driver forcedeth_driver;
extern uint8_t PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint16_t PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern uint32_t PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
extern void PciWriteByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint8_t value);
extern void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint16_t value);
extern uint32_t PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, uint32_t value);
extern void BootResetAction(void);
extern void BootStopUSB(void);

struct meminfo meminfo = 
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

unsigned long virt_offset = 0;

struct pci_driver* pci_drivers = &forcedeth_driver;
struct pci_driver* pci_drivers_end = &forcedeth_driver + 1;

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

extern void sleep(int);

void restart_etherboot(int status)
{
	eth_disable();
	nic.dev.state.pci.dev.driver = 0;
	BootStopUSB();
	sleep(5);
	BootResetAction();
}
static const unsigned char vendorext_magic[] = {0xE4,0x45,0x74,0x68}; /* Eth */
static unsigned char    rfc1533_cookie[5] = { RFC1533_COOKIE, RFC1533_END };

static int find_cmdline(char ** cmd_line, char* length)
{
	unsigned char* p = BOOTP_DATA_ADDR->bootp_reply.bp_vend;
	int vendorext_isvalid = 0;
	int found = 0;
	if (memcmp(p, rfc1533_cookie, 4))
	{
		found = 0;
	}
	else
	{
		p += 4;
		while (p < end_of_rfc1533) {
			unsigned char c = *p;
			if (c == RFC1533_PAD) {
				p++;
				continue;
			}
			else if (ENCAP_OPT c == RFC1533_VENDOR_MAGIC
				&& TAG_LEN(p) >= 6 &&
				!memcmp(p+2,vendorext_magic,4) &&
				p[6] == RFC1533_VENDOR_MAJOR
				)
				vendorext_isvalid++;
			else if (ENCAP_OPT c == RFC1533_VENDOR_ADDPARM && TAG_LEN(p) > 3 && vendorext_isvalid)
			{
				*length = *(p+1);
				*cmd_line = p+2;
				found = 1;
				break;
			}
			p += TAG_LEN(p) + 2;
		}
	}
	return found;
}

void xstart16 (unsigned long a, unsigned long b, char * c)
{
	printf("xstart16() is not supported for Xbox\n");
	restart_etherboot(-1);
}

extern int ExittoLinux(CONFIGENTRY *config);
extern unsigned int dwInitrdSize;

int xstart32(unsigned long entry_point, ...)
{
	CONFIGENTRY config;
	char* cmdline;
	char length;
	if (find_cmdline(&cmdline, &length))
	{
		memcpy(config.szAppend, cmdline, length);
		config.szAppend[length] = '\0';
		printf("Using cmdline from BOOTP: %s\n", cmdline);
	}
	else
	{
		config.szAppend[0] = '\0';
	}
	dwInitrdSize = 0;
	ExittoLinux(&config);
}
