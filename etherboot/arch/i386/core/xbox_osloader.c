#include "etherboot.h"
#include "nic.h"

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
