#include "boot.h"
#include "config.h"
#include "cpu.h"


extern void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
        __asm__("pushl %%ebx\n\t"
		"cpuid\n\t"
		"movl	%%ebx, %%esi\n\t"
		"popl	%%ebx\n\t"
                : "=a" (*eax),
                  "=S" (*ebx),
                  "=c" (*ecx),
                  "=d" (*edx)
                : "a" (op)
                : "cc");
}			  

extern void intel_interrupts_on()
{
    unsigned long low, high;
    
    printk("Disabling local apic...");
    
    /* this is so interrupts work. This is very limited scope --
     * linux will do better later, we hope ...
     */
    rdmsr(0x1b, low, high);
    low &= ~0x800;
    wrmsr(0x1b, low, high);
}

extern void cache_disable(void)
{
	unsigned int tmp;

	/* Disable cache */
	printk("Disable Cache\n");

	/* Write back the cache and flush TLB */
	asm volatile ("movl  %%cr0, %0\n\t"
		      "orl  $0x40000000, %0\n\t"
		      "wbinvd\n\t"
		      "movl  %0, %%cr0\n\t"
		      "wbinvd\n\t"
		      : "=r" (tmp) : : "memory");
}

extern void cache_enable(void)
{
	unsigned int tmp;
	unsigned int eax, ebx, ecx, edx;
	
	cpuid(0, &eax, &ebx, &ecx, &edx);
	cpuid(1, &eax, &ebx, &ecx, &edx);
	// Set the Latency of the Cache to 7
	rdmsr(0x11e, eax, edx);
	eax &= 0xffffffe1;
	eax |= 0x0000000a;
	wrmsr(0x11e, eax, edx);

	asm volatile ("movl  %%cr0, %0\n\t"
		      "andl  $0x9fffffff, %0\n\t"
		      "movl  %0, %%cr0\n\t"
		      :"=r" (tmp) : : "memory");

	printk("Enable Cache\n");
}

