#ifndef _PHYMENUACTIONS_H_
#define _PHYMENUACTIONS_H_
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

extern int CurrentSerialState;
extern int CurrentSerialIRQState;

int IsSerialEnabled(void);
void SetSerialEnabled(void *);
int HasSerialIRQ(void);
void SetSerialIRQ(void *);

#endif
