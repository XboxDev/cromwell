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
	u8 password[20];
	unsigned uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	int i;
	if (CalculateDrivePassword(nIndexDrive,password)) {
		printk("Unable to calculate drive password - eeprom corrupt?");
		return;
	}
	printk("Locking drive\n\n");
	printk("Cromwell locks drives with a master password of \"\2XBOXLINUX\2\"\n\nPlease remember this,");
	printk("as it could save your drive!\n\n");
	printk("The normal password (user password) the drive is being locked with is as follows:\n\n");
	printk("                              ");
	VIDEO_ATTR=0xff0000;
	for (i=0; i<20; i++) {
		printk("\2%02x \2",password[i]);
		if ((i+1)%5 == 0) {
			printk("\n\n                              ");
		}
	}	
	VIDEO_ATTR=0xffffff;
	printk("\nLocking drive - please wait\n");
	if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_SET_PASSWORD, password)) {
		printk("Lock command failed!");
	}
	printk("Complete.\nMake a note of the password above, and\n");
	printk("press Button A to continue");

	while ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) != 1)) wait_ms(100);
}

void UnlockHdd(void *driveId) {
	int nIndexDrive = *(int *)driveId;
	u8 password[20];
	unsigned uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	if (CalculateDrivePassword(nIndexDrive,password)) {
		printk("Unable to calculate drive password - eeprom corrupt?  Aborting\n");
		return;
	}
	if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_DISABLE, password)) {
		printk("Failed!");
	}
	printk("\2\n\2\n\2This drive is now unlocked.\n\n");
	printk("\2Press Button A to continue");

	while ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) != 1)) wait_ms(100);
}


void DisplayHddPassword(void *driveId) {
	int nIndexDrive = *(int *)driveId;
	u8 password[20];
	int i;
	
	if (CalculateDrivePassword(nIndexDrive,password)) {
		printk("Unable to calculate drive password - eeprom corrupt?");
		return;
	}
	
	printk("The normal password (user password) for this drive is as follows:\n\n");
	printk("                              ");
	VIDEO_ATTR=0xff0000;
	for (i=0; i<20; i++) {
		printk("\2%02x \2",password[i]);
		if ((i+1)%5 == 0) {
			printk("\n\n                              ");
		}
	}	
	printk("\n\nPress Button A to continue");

	while ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) != 1)) wait_ms(100);
}
