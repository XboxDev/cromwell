/* Xbox Linux Clean ROM

   This code does basic machine setup, as done by the Xcodes. After we get
   control of the machine, this is the first code executed - and it's already
   in C. (mist)

   Written by Andy, Michael, Paul, Steve
   Rewritten in C by Michael.
*/

/* sure, these exist somewhere else already, I should
   use the functions defined in some .h file... (mist)
*/
static inline unsigned int get_cr0() {
	register unsigned int v = 0;
	asm volatile ( "movl %%cr0,%0" : "=r" (v) : "0" (v) );
	return v;
}

static inline void set_cr0(unsigned int v) {
	asm volatile ( "movl %0,%%cr0" : "=r" (v) : "0" (v) );
}

static inline void wrmsr(unsigned int ecx, unsigned eax, unsigned edx) {
	asm volatile ("wrmsr" : : "c"(ecx), "a"(eax), "d"(edx));
}

static inline void outb(unsigned short addr, unsigned char val) {
	asm volatile ("outb %b0,%w1": :"a" (val), "Nd" (addr));
}

static inline void outw(unsigned short addr, unsigned short val) {
	asm volatile ("outw %w0,%w1": :"a" (val), "Nd" (addr));
}

static inline void outl(unsigned short addr, unsigned int val) {
	asm volatile ("outl %0,%w1": :"a" (val), "Nd" (addr));
}

static inline unsigned char inb(unsigned short addr) {
	register unsigned char val;
	asm volatile ("inb %w1,%0":"=a" (val):"Nd" (addr));
	return val;
}

static inline unsigned int inl(unsigned char addr) {
	register unsigned int val;
	asm volatile ("inl %w1,%0":"=a" (val):"Nd" (addr));
	return val;
}

static inline void pci_outl_help1(unsigned int addr) {
	asm volatile ("movw $0x0cf8, %%dx; outl %0,%%dx": :"a" (addr));
}
static inline void pci_outl_help2(unsigned int val) {
	asm volatile ("movb $0xfc, %%dl; outl %0,%%dx": :"a" (val));
}
static inline void pci_outl(unsigned int addr, unsigned int val) {
	pci_outl_help1(addr);
	pci_outl_help2(val);
}

static inline void pci_inl_help1(unsigned int addr) {
	asm volatile ("movw $0x0cf8, %%dx; outl %0,%%dx": :"a" (addr));
}
static inline unsigned int pci_inl_help2() {
	register unsigned int val;
	asm volatile ("movb $0xfc, %%dl; inl %%dx,%0":"=a" (val));
	return val;
}
static inline unsigned int pci_inl(unsigned int addr) {
	pci_inl_help1(addr);
	return pci_inl_help2();
}

static inline void pokel(unsigned int a, unsigned int d) {
    *((volatile unsigned int*)(a)) = d;
}

static inline unsigned int peekl(unsigned int a) {
    return (*((volatile unsigned int*)(a)));
}

static inline unsigned char peekb(unsigned int a) {
    return (*((volatile unsigned char*)(a)));
}

/* this is the very first code that gets executed. Therefore this file
   *really*should* be compiled with "-fomit-frame-pointer", or else
   GCC starts it with "push %ebp; mov %esp, %ebp" which might be fatal,
   because we don't have a stack yet. (mist)
*/
void MachineInit() {
#ifdef MACHINE_INIT_IN_C
	unsigned int i;
#ifndef XBE
	unsigned int temp;

	asm ("cli");

	set_cr0(get_cr0() | 0x60000000);

	// kill MTRRs
	wrmsr(0x2ff, 0, 0);

	outb(0x61, 8);

	// xcode actions first of all

	// v1.0 ACPI IO region enable
	pci_outl(0x80000810, 0x8001);
	// v1.1 ACPI IO region enable
	pci_outl(0x80000884, 0x8001);
	// extsmi# able to control power (b0<-0 causes CPU powerdown after couple of seconds)
	outw(0x8026, 0x2201);
	pci_outl(0x80000804, 3);
	outb(0x80d6, 4);
	outb(0x80d8, 4);
	outb(0x8049, 8);

	pci_outl(0x8000f04c, 1);
	pci_outl(0x8000f018, 0x10100);
	pci_outl(0x80000084, 0x7ffffff);
	pci_outl(0x8000f020, 0xff00f00);
	pci_outl(0x8000f024, 0xf7f0f000);
	pci_outl(0x80010010, 0xf000000);
	pci_outl(0x80010014, 0xf0000000);
	pci_outl(0x80010004, 7);
	pci_outl(0x8000f004, 7);

	pokel(0x0f0010b0, 0x07633461);
	pokel(0x0f0010cc, 0x66660000);
	// new guy, move the video out of the way
	pokel(0xf600800, 0x03c00000);

	if (!((temp = peekl(0x0f101000)) & 0xc0000)) {
		pokel(0x0f101000, (temp & 0xe1f3ffff) | 0x80000000);
		pokel(0x0f0010b8, 0xeeee0000);
	} else {
		pokel(0x0f101000, (temp & 0xe1f3ffff) | 0x860c0000);
		pokel(0x0f0010b8, 0xffff0000);
	}

	pokel(0xf0010b4, 0);
	pokel(0xf0010bc, 0x5866);
	pokel(0xf0010c4, 0x0351c858);
	pokel(0xf0010c8, 0x30007d67);
	pokel(0xf0010d8, 0);
	pokel(0xf0010dc, 0xa0423635);
	pokel(0xf0010e8, 0x0c6558c6);
	pokel(0xf100200, 0x03070103);
	pokel(0xf100410, 0x11000016);
	pokel(0xf100410, 0x11000016);
	pokel(0xf100330, 0x84848888);
	pokel(0xf10032c, 0xffffcfff);
	pokel(0xf100328, 1);
	pokel(0xf100338, 0xdf);
	if(peekb(0xf000000)==0xa1) {
		//temp = 0x803d4401;
		temp = peekl(0xf101000);
	}
	pci_outl(0x80000904, 1);
	pci_outl(0x80000914, 0xc001);
	pci_outl(0x80000918, 0xc201);
	asm("mov $0x8000093c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	inl %dx, %eax ; orl	$0x7, %eax ;	outl	%eax, %dx");
	outb(0xc200, 0x70);

	// skipped unnecessary conexant init
	outb(0xc000, 0x10);
	outb(0xc004, 0x20);
	outb(0xc008, 1);
	outb(0xc006, 0);
	outb(0xc002, 0x0a);
	while(inb(0xc000)!=0x10);

	// (skipped PIC test here)
	outb(0xc000, 0x10);
	outb(0xc004, 0x21);
	outb(0xc008, 1);
	outb(0xc002, 0x0a);
	while(inb(0xc000)!=0x10);

	if(inb(0xc006)!=0x50) {
		pci_outl(0x8000036c, 0x01000000);
	}
asm("
	movb 0x0f000000, %al
	cmpb $0xa1, %al
	jnz nota1c

	mov $0x10101010, %eax ; mov 0x0f001214, %eax

	jmp donea1c

nota1c:

	mov $0x12121212, %eax ; mov 0x0f001214, %eax

donea1c:
");
	pokel(0x0f00122c, 0xaaaaaaaa);
	pokel(0x0f001230, 0xaaaaaaaa);
	pokel(0x0f001234, 0xaaaaaaaa);
	pokel(0x0f001238, 0xaaaaaaaa);
	pokel(0x0f00123c, 0x8b8b8b8b);
	pokel(0x0f001240, 0x8b8b8b8b);
	pokel(0x0f001244, 0x8b8b8b8b);
	pokel(0x0f001248, 0x8b8b8b8b);
	pokel(0x0f1002d4, 0x00000001);
	pokel(0x0f1002c4, 0x00100042);
	pokel(0x0f1002cc, 0x00100042);
	pokel(0x0f1002c0, 0x11);
	pokel(0x0f1002c8, 0x11);
	pokel(0x0f1002c0, 0x32);
	pokel(0x0f1002c8, 0x32);
	pokel(0x0f1002c0, 0x132);
	pokel(0x0f1002c8, 0x132);
	pokel(0x0f1002d0, 0x1);
	pokel(0x0f1002d0, 0x1);
	pokel(0x0f100210, 0x80000000);
	pokel(0x0f00124c, 0xaa8baa8b);
	pokel(0x0f001250, 0x0000aa8b);
	pokel(0x0f100228, 0x081205ff);
	pokel(0x0f000218, 0x00010000);

	// pci_outl(0x80000860, pci_inl(0x80000860) | 0x400); mist: compiler issue
	asm("mov $0x80000860, %eax ; movw $0xcf8, %dx ;      outl    %eax, %dx ; movw $0xcfc, %dx ;      in %dx, %eax ; orl $0x400, %eax ;       outl    %eax, %dx");
	pci_outl(0x8000084c, 0xfdde);
	pci_outl(0x8000089c, 0x871cc707);
	// pci_outl(0x800008b4, pci_inl(0x800008b4) | 0xf00); mist: compiler issue
	asm("mov $0x800008b4, %eax ; movw $0xcf8, %dx ;      outl    %eax, %dx ; movw $0xcfc, %dx ;      in %dx, %eax ; orl $0xf00, %eax ;       outl    %eax, %dx");
	pci_outl(0x80000340, 0xf0f0c0c0);
	pci_outl(0x80000344, 0x00c00000);
	pci_outl(0x8000035c, 0x04070000);
	pci_outl(0x8000036c, 0x00230801);
	pci_outl(0x8000036c, 0x01230801);
	asm("mov $8, %eax ; timloop2: dec %eax ; cmp $0, %eax ; jnz timloop2");
		// 5F1

	pokel(0xf100200, 0x03070103);
	pokel(0xf100204, 0x11448000);

	// skipped actual memory test

	// A95
	pokel(0xf100200, 0x03070003);

		// A9E
	pci_outl(0x80000084, 0x03ffffff);

	outb(0xc006, 0x0f);
	outb(0xc004, 0x20);
	outb(0xc008, 0x13);
	outb(0xc002, 0x0a);
	while(inb(0xc000)!=0x10);
	outb(0xc000, 0x10);

	outb(0xc006, 0xf0);
	outb(0xc004, 0x20);
	outb(0xc008, 0x12);
	outb(0xc006, 0xf0);
	outb(0xc002, 0x0a);
	while(inb(0xc000)!=0x10);
	outb(0xc000, 0x10);

	pci_outl(0x8000f020, 0xfdf0fd00);
	pci_outl(0x80010010, 0xfd000000);

#endif // ndef XBE

	asm(
		"	lidt tableIdtDescriptor;"
		"	lgdt tableGdtDescriptor;"
		"	ljmp $0x10, $selftarget;"
		"selftarget:"
	);

	// this from 2bl first init

	// kill the cache

	set_cr0(get_cr0() | 0x60000000);
	asm("wbinvd");

	wrmsr(0x2ff, 0, 0);

	// Init the MTRR for Ram

	// MTRR for RAM
	// from address 0, Writeback Caching, 64MB range

	wrmsr(0x200, 6, 0);

	// MASK0 set to 0xffc000[000] == 64M
	wrmsr(0x201, 0xfc000800, 0x0f);

	// MTRR for BIOS

	wrmsr(0x202, 0xfff00006, 0);

	// MASK0 set to 0xff0000[000] == 16M
	wrmsr(0x203, 0xfff00800, 0x0f);


	// MTRR for Video Memory (last 4MByte of shared Ram)
	// Writethrough type trumps Writeback for overlapping region

	wrmsr(0x204, 0x03c00004, 0);

	// MASK0 set to 0xfffC00[000] == 4M
	wrmsr(0x205, 0xffc00800, 0x0f);

	for(i = 0x206; i <= 0x20a; i++) wrmsr(i, 0, 0);

// madeline

	wrmsr(0x2ff, 0x800, 0);

	/* turn on normal cache */
	set_cr0(get_cr0() & 0x9fffffff);

	// set up selectors for everything

	asm(
		"mov $0x18, %eax;"
		"mov %ax, %ds;"  // from 2bl first init
		"movl $0x00080000, %esp;"
		"movw %ax, %ds;"
		"movw %ax, %es;"
		"movw %ax, %ss;"

		"xor %eax, %eax;"
		"movw %ax, %fs;"
		"movw %ax, %gs;"

		"cld;"
	);

#else // !MACHINE_INIT_IN_C


	asm("
//#ifdef XBE
//	jmp bootloader2
//#endif

	cli

	mov %cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	invd

	xorl	%edx, %edx
	movl	$0x2ff, %ecx
	movl	$0x000, %eax
	wrmsr

//		mov	$0, %al
//		mov	$0x72, %dx
//		out	%al, %dx

	mov	$0x8, %al
	mov	$0x61, %dx
	out	%al, %dx

		// xcode actions first of all

	mov $0x80000810, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx
	mov $0x80000884, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx
	movw $0x8026, %dx ;	movw $0x2201, %ax ;	outb %al, %dx

	mov $0x80000804, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x3, %eax ;	outl	%eax, %dx

	movw $0x80d6, %dx ;	movb $4, %al ;	outb %al, %dx
	movw $0x80d8, %dx ;	movb $4, %al ;	outb %al, %dx
	movw $0x8049, %dx ;	movb $8, %al ;	outb %al, %dx
//	movw $0x80d9, %dx ;	movb $0, %al ;	outb %al, %dx

//	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0, %eax ;	outl	%eax, %dx
//	mov $0x80000340, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf81c4400, %eax ;	outl	%eax, %dx

	mov $0x8000f04c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000001, %eax ;	outl	%eax, %dx
	mov $0x8000f018, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00010100, %eax ;	outl	%eax, %dx
	mov $0x80000084, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x07ffffff, %eax ;	outl	%eax, %dx
	mov $0x8000f020, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0ff00f00, %eax ;	outl	%eax, %dx
	mov $0x8000f024, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf7f0f000, %eax ;	outl	%eax, %dx
	mov $0x80010010, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0f000000, %eax ;	outl	%eax, %dx
	mov $0x80010014, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf0000000, %eax ;	outl	%eax, %dx
	mov $0x80010004, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000007, %eax ;	outl	%eax, %dx
	mov $0x8000f004, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000007, %eax ;	outl	%eax, %dx

	movl $0x07633461, 0x0f0010b0
	movl $0x66660000, 0x0f0010cc

	movl $0x03c00000, 0x0f600800
//	movl $0x0, 0x0f680600

	cld

	movl 0x0f101000, %eax
	testl $0x000c0000, %eax
	jnz nota1a

	andl	$0xe1f3ffff, %eax
	orl $0x80000000, %eax
	movl %eax, 0x0f101000
	movl	$0xeeee0000, 0x0f0010b8

//	movl $0x07633451, 0x0f0010b0
//	movl $0x0, 0x0f0010cc
//	movl $0xffff0000, 0x0f0010b8
//	movl $0x5, 0x0f0010d4

	jmp donea1a

nota1a:

	andl	$0xe1f3ffff, %eax
	orl $0x860c0000, %eax
	movl %eax, 0x0f101000
	movl	$0xffff0000, 0x0f0010b8

//	movl $0x07633461, 0x0f0010b0
//	movl $0x66660000, 0x0f0010cc
//	movl $0xffff0000, 0x0f0010b8
//	movl $0x9, 0x0f0010d4

donea1a:

	movl $0x0, 0x0f0010b4
	movl $0x5866, 0x0f0010bc
	movl $0x0351c858, 0x0f0010c4
	movl $0x30007d67, 0x0f0010c8
	movl $0x0, 0x0f0010d8
	movl $0xa0423635, 0x0f0010dc
	movl $0x0c6558c6, 0x0f0010e8

	movl $0x03070103, 0x0f100200

	movl $0x11000016, 0x0f100410
	movl $0x11000016, 0x0f100410
	movl $0x84848888, 0x0f100330

	movl $0xffffcfff, 0x0f10032c
	movl $0x00000001, 0x0f100328
	movl $0x000000df, 0x0f100338

	movb 0x0f000000, %al
	cmpb $0xa1, %al
	jnz nota1b

	mov $0x803d4401, %eax ; mov 0x0f101000, %eax

nota1b:

	mov $0x80000904, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000001, %eax ;	outl	%eax, %dx
	mov $0x80000914, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c001, %eax ;	outl	%eax, %dx
	mov $0x80000918, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c201, %eax ;	outl	%eax, %dx

	mov $0x8000093c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	inl %dx, %eax ; orl	$0x7, %eax ;	outl	%eax, %dx

	movw $0xc200, %dx ;	movb $0x70, %al ;	outb %al, %dx
//	movw $0xc002, %dx ;	movb $0x10, %al ;	outb %al, %dx

		// skipped unnecessary conexant init

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x01, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0x00, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

spin1:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin1

	  // (skipped PIC test here)
	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x21, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x01, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

spin1a:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin1a

	movw $0xc006, %dx
	inb %dx, %al
	cmp $0x50, %al
	jz skipPicDecidedInit

	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x01000000, %eax ;	outl	%eax, %dx

skipPicDecidedInit:

	movb 0x0f000000, %al
	cmpb $0xa1, %al
	jnz nota1c

	mov $0x10101010, %eax ; mov 0x0f001214, %eax

	jmp donea1c

nota1c:

	mov $0x12121212, %eax ; mov 0x0f001214, %eax

donea1c:

	movl $0xaaaaaaaa, 0x0f00122c
	movl $0xaaaaaaaa, 0x0f001230
	movl $0xaaaaaaaa, 0x0f001234
	movl $0xaaaaaaaa, 0x0f001238
	movl $0x8b8b8b8b, 0x0f00123c
	movl $0x8b8b8b8b, 0x0f001240
	movl $0x8b8b8b8b, 0x0f001244
	movl $0x8b8b8b8b, 0x0f001248
	movl $0x00000001, 0x0f1002d4
	movl $0x00100042, 0x0f1002c4
	movl $0x00100042, 0x0f1002cc
	movl $0x11, 0x0f1002c0
	movl $0x11, 0x0f1002c8
	movl $0x32, 0x0f1002c0
	movl $0x32, 0x0f1002c8
	movl $0x132, 0x0f1002c0
	movl $0x132, 0x0f1002c8
	movl $0x1, 0x0f1002d0
	movl $0x1, 0x0f1002d0
	movl $0x80000000, 0x0f100210
	movl $0xaa8baa8b, 0x0f00124c
	movl $0x0000aa8b, 0x0f001250
	movl $0x081205ff, 0x0f100228
	movl $0x00010000, 0x0f000218

	mov $0x80000860, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	in %dx, %eax ; orl $0x400, %eax ;	outl	%eax, %dx
	mov $0x8000084c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfdde, %eax ;	outl	%eax, %dx
	mov $0x8000089c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x871cc707, %eax ;	outl	%eax, %dx

	mov $0x800008b4, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	in %dx, %eax ; orl $0xf00, %eax ;	outl	%eax, %dx

	mov $0x80000340, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf0f0c0c0, %eax ;	outl	%eax, %dx
	mov $0x80000344, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00c00000, %eax ;	outl	%eax, %dx
	mov $0x8000035c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x04070000, %eax ;	outl	%eax, %dx
	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00230801, %eax ;	outl	%eax, %dx
	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x01230801, %eax ;	outl	%eax, %dx

	mov $8, %eax ; timloop2: dec %eax ; cmp $0, %eax ; jnz timloop2

		// 5F1

	movl $0x03070103, 0x0f100200
	movl $0x11448000, 0x0f100204

			// skipped actual memory test

		// A95
	movl $0x03070003, 0x0f100200

		// A9E
	mov $0x80000084, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x03ffffff, %eax ;	outl	%eax, %dx

	movw $0xc006, %dx ;	movb $0x0f, %al ;	outb %al, %dx

	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x13, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

		// B2E
spin2:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin2

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x12, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

		// B76
spin3:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin3

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx

	mov $0x8000f020, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfdf0fd00, %eax ;	outl	%eax, %dx
	mov $0x80010010, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfd000000, %eax ;	outl	%eax, %dx





bootloader2:

	lidt tableIdtDescriptor
	lgdt tableGdtDescriptor
	ljmp $0x10, $selftarget

selftarget:

	// this from 2bl first init

		// kill the cache

	mov %cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

	mov	%cr3, %eax
	mov	%eax, %cr3

	movl	$0x2ff, %ecx
		xor		%eax, %eax
		xor		%edx, %edx
		wrmsr

		// Init the MTRR for Ram

		movl	$0x200, %ecx

			// MTRR for RAM
			// from address 0, Writeback Caching, 64MB range

		movl	$0x00000000, %edx
		movl	$0x00000006, %eax
		wrmsr
		inc		%ecx

			// MASK0 set to 0xffc000[000] == 64M
		movl	$0x0f, %edx
		movl	$0xfc000800, %eax
		wrmsr
		inc %ecx


			// MTRR for BIOS

		movl	$0x00000000, %edx
		movl	$0xFFF00006, %eax
		wrmsr
		inc		%ecx

			// MASK0 set to 0xff0000[000] == 16M
		movl	$0x0000000f, %edx
		movl	$0xFFF00800, %eax
		wrmsr
		inc %ecx


			// MTRR for Video Memory (last 4MByte of shared Ram)
			// Writethrough type trumps Writeback for overlapping region

		movl	$0x00000000, %edx
		movl	$0x03C00004, %eax
		wrmsr
		inc		%ecx
			// MASK0 set to 0xfffC00[000] == 4M
		movl	$0x0000000f, %edx
		movl	$0xFFC00800, %eax
		wrmsr
		inc %ecx

/*
			// MTRR for Filtror comms area

		movl	$0x00000000, %edx // 0x00
		movl	$0xffffe000, %eax // == Writethrough == 4
		wrmsr
		inc		%ecx
			// MASK0 set to 0xfffC00[000] == 4M
		movl	$0x00000000, %edx // 0x0f
		movl	$0xFFFFF800, %eax  // 0xffC00800
		wrmsr
		inc %ecx
*/

		xor		%eax, %eax
		xor		%edx, %edx
cleardown:
		wrmsr
		inc	%ecx
		cmpb	$0xf, %cl
		jna cleardown

// madeline

		movl	$0x2ff, %ecx
		movl	$0x800, %eax
		wrmsr

			/* turn on normal cache */
//		invd
		movl	%cr0, %eax
		mov %eax, %ebx
		andl	$0x9FFFFFFF,%eax
		movl	%eax, %cr0


			// set up selectors for everything

		mov	$0x18, %eax
		.byte 0x8e, 0xd8
//		movl	$ 0x00200000,%esp
		movl	$ 0x00080000,%esp
		movw    %ax, %ds
		movw    %ax, %es
		movw    %ax, %ss

		xor		%eax, %eax
		movw    %ax, %fs
		movw    %ax, %gs

		cld


");
#endif // MACHINE_INIT_IN_C


	/* We do the rest in assembly. This should be rewritten in C, too!
	   (mist)
	*/
	asm("
		// set up the page directory

	mov $0xf000, %edi
	mov $0x40, %ecx
	mov $0xe3, %eax

	// +0-+FF AND +800-+8FF, 256M contiguous region pointing at linear 0-256M

patspin:
	movl %eax, 0x800(%edi)
	stosl
	add $0x400000, %eax
	loop	patspin

	// +100-+7FF AND +900-+FFF, EMPTY

	mov	$0x1c0, %ecx
	xor	%eax, %eax
	mov $0xfffc00f3, %eax

patspin2:
	movl %eax, 0x800(%edi)
	stosl
	loop	patspin2

	mov $0xf000, %edi

	mov $0x3c000f3, %eax
	mov %eax, 0x3c(%edi)
	mov %eax, 0x83c(%edi)

	mov $0xf063, %eax
	mov %eax, 0xc00(%edi)

	mov $0xff0000f3, %eax
	mov %eax, 0xff0(%edi)
	mov $0xff4000e3, %eax
	mov %eax, 0xff4(%edi)
	mov $0xff8000e3, %eax
	mov %eax, 0xff8(%edi)
	mov $0xffc000e3, %eax
	mov %eax, 0xffc(%edi)

	mov $0xfd0000fb, %eax
	mov	%eax, %ebx

	shr	$0x14, %ebx
	add	%edi, %ebx

		// contiguous 16M area starting 0xfd000000 - 0xfdffffff in page tbl at 0xffd0+

	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)

	mov $0xfe0000fb, %eax
	mov	%eax, %ebx

	shr	$0x14, %ebx
	add	%edi, %ebx

		// contiguous 16M area starting 0xfe000000 - 0xfeffffff in page tbl at 0xffe0+

	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)


	mov	%cr0, %eax
	mov	%eax, %ebx
	and $0xdfffffff, %eax
	or	$0x40000000, %eax
	mov	%eax, %cr0
	wbinvd

	mov	$0x277, %ecx
	mov	$0x70106, %eax
	mov	%eax, %edx
	wrmsr
//	invd

	mov	%ebx, %cr0

	mov	%cr4, %eax
	or	$0x610, %eax
	mov	%eax, %cr4

	mov	$0xf000, %eax
	mov	%eax, %cr3

	mov	%cr0, %eax
	or	$0x80010020, %eax
	mov	%eax, %cr0

	jmp selfPipeKill
selfPipeKill:

	mov $0x00040000, %esp


/*
  cld             // clear direction flag

	movl $0, %edi
	xor	%eax, %eax
 	movl	$0x100000, %ecx
	rep
  stosb

//	mov $0x00040000, %esp

        // copy initiliazed data to ram

        leal    _start_load_data, %esi
        leal    _start_data, %edi
        movl    $_end_load_data, %ecx
        subl    %esi, %ecx
        jz      .nodata
        rep
        movsb
.nodata:

    // clear bss
        leal    _bss, %edi
        movl    $_ebss, %ecx
        subl    %edi, %ecx
        jz      .nobss
        xorl    %eax, %eax
        rep
        stosb
.nobss:
*/

	jmp BootResetAction

");
}
