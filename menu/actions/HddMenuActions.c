/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "HddMenuActions.h"

void LockHdd(void *driveId) {
	int nIndexDrive = *(int *)driveId;
	char password[20];
	unsigned uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	if (CalculateDrivePassword(nIndexDrive,password)) {
		printk("Unable to calculate drive password - eeprom corrupt?");
		return;
	}
	if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_SET_PASSWORD, password)) {
		printk("Failed!");
	}
	wait_ms(5000);
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
}

void UnlockHdd(void *driveId) {
	int nIndexDrive = *(int *)driveId;
	char password[20];
	unsigned uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	if (CalculateDrivePassword(nIndexDrive,password)) {
		printk("Unable to calculate drive password - eeprom corrupt?");
		return;
	}
	if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_DISABLE, password)) {
		printk("Failed!");
	}
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
}

