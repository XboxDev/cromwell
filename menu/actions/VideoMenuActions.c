/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "video.h"
#include "xbox.h"
#include "VideoInitialization.h"

void SetWidescreen(void *menuItemText) {
	char **text = (char **)menuItemText;
	if (!strcmp(*text, "Display Size: Widescreen")) {
		free(*text);
		*text = malloc(30);
		strcpy(*text, "Display Size: Normal");
		EepromSetWidescreen(0);	
	}
	else if (!strcmp(*text, "Display Size: Normal")) {
		free(*text);
		*text = malloc(30);
		strcpy(*text, "Display Size: Widescreen");
		EepromSetWidescreen(1);	
	}
}

void SetVideoStandard(void *menuItemText) {
	char **text = (char **)menuItemText;

	if (!strcmp(*text, "TV Standard: PAL")) {
		free(*text);
		*text = malloc(30);
		strcpy(*text, "TV Standard: NTSC-USA");
		EepromSetVideoStandard(TV_ENC_NTSC);
	}
	else if (!strcmp(*text, "TV Standard: NTSC-USA")) {
		free(*text);
		*text = malloc(30);
		strcpy(*text, "TV Standard: PAL");
		EepromSetVideoStandard(TV_ENC_PALBDGHI);
	}
}

