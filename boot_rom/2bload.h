/***************************************************************************
      Includes used by XBox boot code
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "consts.h"
#include "stdint.h"
#include "cromwell_types.h"

/* BIOS-wide error codes		all have b31 set  */

enum {
	ERR_SUCCESS = 0,  // completed without error

	ERR_I2C_ERROR_TIMEOUT = 0x80000001,  // I2C action failed because it did not complete in a reasonable time
	ERR_I2C_ERROR_BUS = 0x80000002, // I2C action failed due to non retryable bus error

	ERR_BOOT_PIC_ALG_BROKEN = 0x80000101 // PIC algorithm did not pass its self-test
};

// ----------------------------  IO primitives -----------------------------------------------------------

static INLINE void IoOutputByte(u16 wAds, u8 bValue)
{
    __asm__ __volatile__ ("outb %b0,%w1" : : "a" (bValue), "Nd" (wAds));
}

static INLINE void IoOutputWord(u16 wAds, u16 wValue)
{
    __asm__ __volatile__ ("outw %0,%w1" : : "a" (wValue), "Nd" (wAds));
}

static INLINE void IoOutputDword(u16 wAds, u32 dwValue)
{
    __asm__ __volatile__ ("outl %0,%w1" : : "a" (dwValue), "Nd" (wAds));
}

static INLINE u8 IoInputByte(u16 wAds)
{
    register u8 ret;

    __asm__ __volatile__ ("inb %w1,%0" : "=a" (ret) : "Nd" (wAds));

    return ret;
}

static INLINE u16 IoInputWord(u16 wAds)
{
    register u16 ret;

    __asm__ __volatile__ ("inw %w1,%0" : "=a" (ret) : "Nd" (wAds));

    return ret;
}

static INLINE u32 IoInputDword(u16 wAds)
{
    register u32 ret;

    __asm__ __volatile__ ("inl %w1,%0" : "=a" (ret) : "Nd" (wAds));

    return ret;
}

static INLINE void PciWriteDword(u32 bus, u32 dev, u32 func, u32 off, u32 data)
{
    register u32 addr = 0x80000000;

    addr |= (bus & 0xff) << 16; // bus #
    addr |= (dev & 0x1f) << 11; // device #
    addr |= (func & 0x07) << 8; // func #
    addr |= (off & 0xff);       // reg offset

    outl(addr, PCI_CFG_ADDR);
    outl(data, PCI_CFG_DATA);
}

static INLINE u32 PciReadDword(u32 bus, u32 dev, u32 func, u32 off)
{
    register u32 addr = 0x80000000;

    addr |= (bus & 0xff) << 16; // bus #
    addr |= (dev & 0x1f) << 11; // device #
    addr |= (func & 0x07) << 8; // func #
    addr |= (off & 0xff);       // reg offset

    outl(addr, PCI_CFG_ADDR);
    return inl(PCI_CFG_DATA);
}

static INLINE void smbus_write_addr(u8 addr)
{
    outb(0xff, SMBUS);      /* clear any status */
    addr <<= 1;
    outb(addr, SMBUS + 4);  /* set host address; write command */
}

static INLINE void smbus_read_addr(u8 addr)
{
    outb(0xff, SMBUS);      /* clear any status */
    addr <<= 1;
    addr  |= 1;
    outb(addr, SMBUS + 4);  /* set host address; read command */
}

static INLINE u8 smbus_wait(void)
{
    register u8 ret;

    while ((ret = inb(SMBUS)) & SMBUS_HST_BSY); /* host controller busy loop */
    outb(SMBUS_HCYC_STS, SMBUS); /* clear host cycle complete status */

    return ret;
}

static INLINE int smbus_cycle_completed(void)
{
    return smbus_wait() == SMBUS_HCYC_STS;
}

static INLINE void smbus_write_start(u8 cmd, u8 data)
{
    outb(cmd,  SMBUS + 8);  /* set host command */
    outb(data, SMBUS + 6);  /* set host data */
    outb(0x0a, SMBUS + 2);  /* set host start; CYCTYPE == read or write byte | HOSTST */
}

static INLINE void smbus_write(u8 cmd, u8 data)
{
    smbus_write_start(cmd, data);
    smbus_wait();
}

static INLINE void smbus_read_start(u8 cmd)
{
    outb(cmd,  SMBUS + 8);  /* set host command */
    outb(0x0a, SMBUS + 2);  /* set host start; CYCTYPE == read or write byte | HOSTST */
}

static INLINE u8 smbus_read_data(void)
{
    return inb(SMBUS + 6);  /* get host data */
}

static INLINE u8 smbus_read(u8 cmd)
{
    smbus_read_start(cmd);
    smbus_wait();
    return smbus_read_data();
}

static INLINE int machine_is_xbox(void)
{
    /* Original Xbox PCI 0:0.0 ID [10de:02a5] */
    return PciReadDword(BUS_0, DEV_0, FUNC_0, 0x00) == 0x02a510de;
}

/* 2bBootStartBios.c */
void BootSystemInitialization(void) __attribute__((section(".reset_vector.1bl"),naked));
void BootStartBiosLoader(void);

/* 2bPicResponseAction.c */
void smbus_smc_challenge_response(void) __attribute__((section(".low_rom")));
int I2CTransmitWord(u8 bPicAddressI2cFormat, u16 wDataToWrite);
int I2CTransmitByteGetReturn(u8 bPicAddressI2cFormat, u8 bDataToWrite);
/* LED control; see associated enum as argument to I2cSetFrontpanelLed */
enum {
	I2C_LED_RED0 = 0x80,
	I2C_LED_RED1 = 0x40,
	I2C_LED_RED2 = 0x20,
	I2C_LED_RED3 = 0x10,
	I2C_LED_GREEN0 = 0x08,
	I2C_LED_GREEN1 = 0x04,
	I2C_LED_GREEN2 = 0x02,
	I2C_LED_GREEN3 = 0x01
};
int I2cSetFrontpanelLed(u8 b);
int I2cResetFrontpanelLed(void);

void *memcpy(void *dest, const void *src,  size_t size);
void *memset(void *dest, int data,  size_t size);
int memcmp(const void *pb, const void *pb1, size_t n);

extern unsigned char *BufferIN;
extern int BufferINlen;
extern unsigned char *BufferOUT;
extern int BufferOUTPos;

