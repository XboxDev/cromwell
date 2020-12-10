/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2019 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"

int CurrentSerialState;
int CurrentSerialIRQState;

int IsSerialEnabled(void)
{
	int result;

	LpcEnterConfiguration();
	result = LpcGetSerialState();
	LpcExitConfiguration();

	return (result == 1);
}

void SetSerialEnabled(void *menuItemText)
{
	char *text = (char *)menuItemText;

	CurrentSerialState = IsSerialEnabled();

	LpcEnterConfiguration();
	LpcSetSerialState((CurrentSerialState) ? 0 : 1);
	LpcExitConfiguration();

	CurrentSerialState = IsSerialEnabled();

	strcpy(text, "Serial COM1: ");
	strcat(text, (CurrentSerialState) ? "Enabled" : "Disabled");
}

int HasSerialIRQ(void)
{
	int result;

	LpcEnterConfiguration();
	result = LpcGetSerialIRQState();
	LpcExitConfiguration();

	return (result == 1);
}

void SetSerialIRQ(void *menuItemText)
{
	char *text = (char *)menuItemText;

	CurrentSerialState = IsSerialEnabled();
	CurrentSerialIRQState = HasSerialIRQ();

	if (CurrentSerialState) {
		LpcEnterConfiguration();
		LpcSetSerialState(0);
		LpcExitConfiguration();
	}
	LpcEnterConfiguration();
	LpcSetSerialIRQState((CurrentSerialIRQState) ? 0 : 1);
	if (CurrentSerialState) {
		LpcSetSerialState(1);
	}
	LpcExitConfiguration();

	CurrentSerialIRQState = HasSerialIRQ();

	strcpy(text, "Serial COM1 IRQ: ");
	strcat(text, (CurrentSerialIRQState) ? "4 (conflicts with NIC)" : "Disabled");
}
