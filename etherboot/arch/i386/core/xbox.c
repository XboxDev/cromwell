#include "etherboot.h"
#include "pci.h"
#include "nic.h"
#include "xbox.h"

#include <stdarg.h>

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
			else if (ENCAP_OPT c == RFC1533_VENDOR_ADDPARM && TAG_LEN(p) > 0 && vendorext_isvalid)
			{
				*length = TAG_LEN(p);
				*cmd_line = p+2;
				found = 1;
				break;
			}
			p += TAG_LEN(p) + 2;
		}
	}
	return found;
}

static void parse_elf_boot_notes(void *notes, union infoblock **rheader)
{
	unsigned char *note, *end;
	Elf_Nhdr *hdr;
	Elf_Bhdr *bhdr;

	bhdr = notes;
	if (bhdr->b_signature != ELF_BHDR_MAGIC) {
		printf("Error: Wrong signature in ELF header\n");
		return;
	}

	note = ((char *)bhdr) + sizeof(*bhdr);
	end  = ((char *)bhdr) + bhdr->b_size;
	while (note < end) {
		unsigned char *n_name, *n_desc, *next;
		hdr = (Elf_Nhdr *)note;
		n_name = note + sizeof(*hdr);
		n_desc = n_name + ((hdr->n_namesz + 3) & ~3);
		next = n_desc + ((hdr->n_descsz + 3) & ~3);
		if (next > end)
			break;
#if 0
		printf("n_type: %x n_name(%d): n_desc(%d): \n",
				hdr->n_type, hdr->n_namesz, hdr->n_descsz);
#endif

		if ((hdr->n_namesz == 10) &&
			(memcmp(n_name, "Etherboot", 10) == 0)) {
			switch(hdr->n_type) {
			case EB_HEADER:
				*rheader = *((void **)n_desc);
				break;
			default:
				break;
			}
		}
		note = next;
	}
}

static Elf32_Phdr *seg[S_END] = { 0 };

static void locate_elf_segs(union infoblock *header)
{
	int             i;
	Elf32_Phdr      *s;

	s = (Elf32_Phdr *)((char *)header + header->ehdr.e_phoff);
	for (i = 0; i < S_END && i < header->ehdr.e_phnum; i++, s++) {
		seg[i] = s;
	}
}

void xstart16 (unsigned long a, unsigned long b, char * c)
{
	printf("xstart16() is not supported for Xbox\n");
	restart_etherboot(-1);
}

extern int ExittoLinux(CONFIGENTRY *config);
extern unsigned int dwInitrdSize;
#define INITRD_POS 0x02000000

int xstart32(unsigned long entry_point, ...)
{
	CONFIGENTRY config;
	char* cmdline;
	char length;
	int ret = 1;
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
	va_list list;
	void* eb;
	va_start(list, entry_point);
	eb = va_arg(list, void*);
	va_end(list);
	union infoblock *header;
	parse_elf_boot_notes(eb, &header);
	switch(header->img.magic)
	{
	case ELF_MAGIC:
		locate_elf_segs(header);
		break;
	default:
		printf("Error: Unrecognized header format\n");
		dwInitrdSize = 0;
		ret = 0;
		break;
	}
	if (ret && seg[S_RAMDISK] != 0)
	{
		dwInitrdSize = seg[S_RAMDISK]->p_filesz;
		printf("Using ramdisk at 0x%X, size 0x%X\n", seg[S_RAMDISK]->p_paddr, dwInitrdSize);
		memcpy((void*)INITRD_POS, (void*)seg[S_RAMDISK]->p_paddr, dwInitrdSize);
	}
	ExittoLinux(&config);
}
