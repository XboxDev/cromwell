/************************************************************/
/* BIOS start                                               */
/*  Michael Steil                                           */
/************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"

typedef unsigned int size_t;

extern char rombios[1];
extern void * memcpy(void *dest, const void *src,  size_t size);

const unsigned long long
bios_real_mode_gdt_entries [] = {
	0x0000000000000000ULL,  /* 00h: Null descriptor */
	0x0000000000000000ULL,  /* 08h: Unused... */
	0x00cf9a000000ffffULL,	/* 10h: 32-bit 4GB code at 0x00000000 */
	0x00cf92000000ffffULL,	/* 18h: 32-bit 4GB data at 0x00000000 */
	0x00009a000000ffffULL,  /* 20h: 16-bit 64k code at 0x00000000 */
	0x000092000000ffffULL	/* 28h: 16-bit 64k data at 0x00000000 */
};

struct my_gdt_t {
        unsigned short       size __attribute__ ((packed));
        unsigned long long * base __attribute__ ((packed));
};

const struct my_gdt_t bios_real_mode_gdt = {
	sizeof (bios_real_mode_gdt_entries)-1,
	(long long*)bios_real_mode_gdt_entries
};
const struct my_gdt_t bios_real_mode_idt = { 0x3ff, 0 };

void StartBios() {
    bprintf("Copying BIOS into RAM...\n");
    /* kernel boot data */
    memcpy((void*)0xF0000, &rombios, 0x10000);

    I2cSetFrontpanelLed(0xff);
    bprintf("done. Running BIOS...\n");

	/* run kernel */
    __asm (
	"lidt  %0             \n"
	"lgdt  %1             \n"
	"mov  $0x18, %%ax     \n"
	"mov  %%ax, %%ds      \n"
	"mov  %%ax, %%es      \n"
	"mov  $code_start, %%esi \n"
	"mov  $0, %%edi       \n"
	"mov  $0x100, %%ecx   \n"
	"rep movsb            \n"
	"ljmp $0x20, $0       \n"
	"code_start:          \n"
	"mov  $0x28, %%ax     \n" /* 16-bit */
	"mov  %%ax, %%ds      \n"
	"mov  %%ax, %%es      \n"
	"mov  %%ax, %%fs      \n"
	"mov  %%ax, %%gs      \n"
	"mov  %%ax, %%ss      \n"
	"mov %%cr0, %%eax     \n"
	"andl $0xFFFFFFFE, %%eax \n"
	"movl %%eax, %%cr0    \n"
	".byte 0xea          \n"
	".word 0xFFF0 \n"
	".word 0xF000 \n"
	".code32 \n"
    : :"m" (bios_real_mode_idt), "m" (bios_real_mode_gdt));
}

#if 0
/*********************************************/
/*********************************************/

".code16 \n"
" nop\n"
" nop\n"
"\n"
"		pushl %%eax;\n"
"		pushl %%edx;\n"
"\n"
"		movl $0xc004, %%edx;\n"
"		mov $0x20, %%al;\n"
"		out %%al, %%dx;\n"
"		movw $0xc008, %%dx;\n"
"		mov $8, %%al;\n"
"		out %%al, %%dx;\n"
"		movw $0xc006, %%dx;\n"
"		mov $0xa6, %%al; /* all colors */\n"
"		out %%al, %%dx;\n"
"		movw $0xc000, %%dx;\n"
"		inw %%dx, %%ax;\n"
"		out %%al, %%dx;\n"
"		movw $0xc002, %%dx;\n"
"		mov $0x1a, %%al;\n"
"		out %%al, %%dx;\n"
"\n"
"		xor %%eax, %%eax\n"
"		movb $1, %%al\n"
"		shl $24, %%eax\n"
"xxx1:\n"
"		decl %%eax;\n"
"		jne xxx1\n"
"\n"
"		movw $0xc004, %%dx;\n"
"		mov $0x20, %%al;\n"
"		out %%al, %%dx;\n"
"		movw $0xc008, %%dx;\n"
"		mov $7, %%al;\n"
"		out %%al, %%dx;\n"
"		movw $0xc006, %%dx;\n"
"		mov $1, %%al;\n"
"		out %%al, %%dx;\n"
"		movw $0xc000, %%dx;\n"
"		inw %%dx, %%ax;\n"
"		out %%al, %%dx;\n"
"		movw $0xc002, %%dx;\n"
"		mov $0x1a, %%al;\n"
"		out %%al, %%dx;\n"
"\n"
"		popl %%edx;\n"
"		popl %%eax;\n"
/*********************************************/
/*********************************************/
#endif


