#ifndef __DRIVERS_PCI_I2C_H__
#define __DRIVERS_PCI_I2C_H__
#include "boot.h"

int ReadfromSMBus(u8 Address,u8 bRegister,u8 Size,u32 *Data_to_smbus);
int WriteToSMBus(u8 Address, u8 bRegister, u8 Size, u32 Data_to_smbus);

int I2CWriteBytetoRegister(u8 bPicAddressI2cFormat, u8 bRegister, u8 wDataToWrite);
int I2cSetFrontpanelLed(u8 b);
int I2cResetFrontpanelLed(void);
bool I2CGetTemperature(int * pnLocalTemp, int * pExternalTemp);
void I2CRebootQuick(void);
void I2CRebootSlow(void);
void I2CPowerOff(void);

void DVDTrayEject(void);
void DVDTrayClose(void);

#endif
