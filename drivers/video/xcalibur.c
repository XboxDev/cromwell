/*
 * linux/drivers/video/xbox/xcalibur.c - Xbox driver for Xcalibur encoder
 *
 * Maintainer: David Pye (dmp) <dmp@davidmpye.dyndns.org>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Known bugs and issues:
 * Overscanned composite and svideo, 480p component only.
 *
*/
#include "xcalibur.h"
#include "xcalibur-regs.h"
#include "encoder.h"

int xcalibur_calc_hdtv_mode(
	xbox_hdtv_mode hdtv_mode,
	int dotClock,
	void **regs
	){
	*regs = (void *)malloc(0x90*sizeof(char)*4);
	//Only 480p so far, sorry!
	memcpy(*regs,&HDTV_XCal_Vals_480p[0],0x90*sizeof(char)*4);	
	return 1;
}

int xcalibur_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out)
{
	//These registers consist of 4 bytes per address.
	riva_out->encoder_mode = (void *)malloc(0x90*sizeof(char)*4);
	
	//Syncs.
	switch(mode->tv_encoding) {
		case TV_ENC_PALBDGHI:
			memcpy(riva_out->encoder_mode,&Composite_XCal_Vals_PAL[0],0x90*sizeof(char)*4);
			riva_out->ext.vsyncstart = 481;
			riva_out->ext.hsyncstart = 703;
			riva_out->ext.htotal = 800 - 1;
			riva_out->ext.vtotal = 520 - 1;
			break;
			
		case TV_ENC_NTSC:
		default: // Default to NTSC
			memcpy(riva_out->encoder_mode,&Composite_XCal_Vals_NTSC[0],0x90*sizeof(char)*4);
			riva_out->ext.vsyncstart = 487;
			riva_out->ext.hsyncstart = 683;
			riva_out->ext.htotal = 780 - 1;
			riva_out->ext.vtotal = 525 - 1;
			break;
	}
		
	riva_out->ext.width = mode->xres;
	riva_out->ext.height = mode->yres;
	riva_out->ext.vcrtc = mode->yres - 1;
	riva_out->ext.vend = mode->yres - 1;
	riva_out->ext.vsyncend = riva_out->ext.vsyncstart + 3;
	riva_out->ext.vvalidstart = 0;
	riva_out->ext.vvalidend = mode->yres - 1;
	riva_out->ext.hend = mode->xres + 7 ;
	riva_out->ext.hcrtc = mode->xres - 1;
	riva_out->ext.hsyncend = riva_out->ext.hsyncstart + 32;
	riva_out->ext.hvalidstart = 0;
	riva_out->ext.hvalidend = mode->xres - 1;
	riva_out->ext.crtchdispend = mode->xres;
	riva_out->ext.crtcvstart = mode->yres + 32;
	//increased from 32
	riva_out->ext.crtcvtotal = mode->yres + 64;
	
	return 1;
}
