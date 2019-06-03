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

	LpcEnterConfiguration();
	LpcSetSerialState(CurrentSerialState ? 0 : 1);
	LpcExitConfiguration();

	CurrentSerialState = IsSerialEnabled();

	strcpy(text, "Serial COM1: ");
	strcat(text, (CurrentSerialState ? "Enabled" : "Disabled"));
}
