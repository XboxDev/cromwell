/*
 * video-related stuff
 * note we are not yet able to init the video to get a raster.... help needed
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"

// these two table are used to set the video mode

const BYTE baConextantAddressesToWrite[] = { // text:8001364C dword_0_8001364C	dd
	0xd6, 0x2e, 0x32, 0x3c, 0x3e, 0x40, 0xc4, 0x6c, 0xce, 0xa0, 0x9e, 0x9c, 0x6c
};

const BYTE baConextantValuesToWriteDependingOnVideoType[][13] =
{
	{ 0x0c, 0xad, 0x48, 0x90, 0x8c, 0x8c, 0x01, 0x9c, 0xe1, 0x8c, 0x00, 0x00, 0x46 }, // dummy entry for illegal 0 option
	{ 0x0c, 0xad, 0x48, 0x90, 0x8c, 0x8c, 0x01, 0x9c, 0xe1, 0x8c, 0x00, 0x00, 0x46 },
	{ 0x0c, 0xaa, 0x49, 0x90, 0x8c, 0x8c, 0x01, 0x9c, 0xe1, 0x21, 0x00, 0x00, 0x46 },
	{ 0x0c, 0xab, 0x49, 0x90, 0x8c, 0x8c, 0x01, 0x9c, 0xe1, 0x21, 0x00, 0x00, 0x46 }
};

const WORD waConextantInitNon10CommandMSB[] = {
	0x2e00, 0x3000, 0x3228, 0x3480, 0x3800, 0x3a00, 0x3c80, 0x4080,
	0x6000, 0x6200, 0x6400, 0x6c46, 0x6e00, 0x700f, 0x7200, 0x7401,
	0xc401, 0xc69c, 0xc89b, 0xcac0, 0xccc0, 0xd840
};

const BYTE baConextantAddressesToWrite2[] = { // text:80013680 dword_0_80013680	dd
	0xd6, 0x36, 0x3e, 0x5a, 0x5c, 0x65, 0x68, 0x6a, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84,
	0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4,
	0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xce, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4,
	0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0x00, 0x00, 0x00
};

const DWORD dwaConextantValuesToWriteDependingOnVideoType2[] =  // munged up DWORDS, autually table of 0x3f byte arrays
{
	0x1FF80A4, 0x10004B15, 0x44857288, 0x26F213ED, 0x3730800,
	0x6E0240D, 0x0DA681000,	0x7CE40A0A, 0x129A8FCB, 0x5A258699,
	0x20FC0F19, 0x0F6FD0, 0x9F30C00,	0x90B566BD, 0x7DB2, 0x0A4000000,
	0x0F801FF80, 0x0D8CC0042, 0x75C9880, 0x26F213, 0x0D037F66,
	0x6E024, 0x0C2AF750, 0x0CA7BE40A, 0x0A4C89A8E, 0x195E2178,
	0x0D020FC0F, 0x0F6F, 0x0BD09F30C, 0x0B290B566, 0x7D, 0x80A40000,
	0x4B1501FF, 0x72881000,	0x13ED4485, 0x80026F2, 0x240D0373,
	0x100006E0, 0x80ADA68, 0x9ADC7CE4, 0x869912A7, 0x0F195A25,
	0x6FD020FC, 0x0C00000F,	0x66BD09F3, 0x7DB290B5, 0x0, 0xFF80A400,
	0x42F801, 0x9880D8CC, 0x0F213075C, 0x7F660026, 0x0E0240D03,
	0x0F7500006, 0x0E4080C2A, 0x0A79ADB7B, 0x2178A4C8, 0x0FC0F195E,
	0x0F6FD020, 0x0F30C0000, 0x0B566BD09, 0x7DB290, 0x0, 0x80A4,
	0x10004B15, 0x4C8E7288,	0x26F213ED, 0x3730800, 0x6E0240D,
	0x0DA681000, 0x58F02A0A, 0x8FA492CD, 0x5A257C0A, 0x20FC0F19,
	0x0F6FD0, 0x9F30C00, 0x90B566BD,	0x7DB2, 0x0A4000000, 0x0F8000080,
	0x0D8CC0042, 0x7649F80,	0x26F213, 0x0D037F66, 0x6E024, 0x0C2AF750,
	0x8057F00A, 0x3A438C48,	0x195A216F, 0x0D020FC0F, 0x0F6F,
	0x0BD09F30C, 0x0B290B566, 0x7D, 0x80E40000, 0x35410000, 0x76883C03,
	0x1615448C, 0x0B600A620, 0x230D03F7, 0x4AE106E0, 0x240B1555,
	0x9BD95EF0, 0x839833A3,	0x519522D, 0x6E402057, 0x0F51F47E,
	0x78D305F1, 0x0A55425A2, 0x0, 0x0FF80E400,	0x2E70800, 0x9A82D8E4,
	0x20162758, 0x81A00A6, 0x0E0230D0C, 0x4AE106, 0x0F0240C40,
	0x0A499D75C, 0x292DEB3A, 0x57051956, 0x7E6E4020, 0x0F10F51F4,
	0x0A278D305, 0x0A55425,	0x0, 0x0FF80D2, 0x28033FAD,	0x428A7488,
	0x0A622160A, 0x37E1400,	0x0A402A71, 0x0F1C75000, 0x5EF0240A,
	0x18A49AD9, 0x2E1775, 0x20570519, 0x0F47E6E40, 0x5F10F51,
	0x25A278D3, 0x0A554, 0x0D2000000, 0x0AF000080, 0x0D8EC02E3,
	0x2B5A9C82, 0x0A62216, 0x71038D76, 0x0A402A, 0x0C4E3950,
	0x0D75DF024, 0x5357A399, 0x195628FE, 0x40205705, 0x51F47E6E,
	0x0D305F10F, 0x5425A278, 0x0A5, 0x80A40000, 0x4B150000, 0x72881000,
	0x13ED4088, 0x80026F2, 0x240D0373, 0x100006E0, 0x200ADA68,
	0x9BD95EF0, 0x7AB77DA3,	0x0F19552E, 0x6FD020FC, 0x0C00000F,
	0x66BD09F3, 0x7DB290B5,	0x0, 0x80C000, 0x42F800, 0x9A80D8CC,
	0x0F2130756, 0x7F660026, 0x0E0240D03, 0x0F7500006, 0x0F0200C2A
};

const BYTE baRegisterLoading[] = { //.text:80013408 dword_0_80013408	dd
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x20, 0x52, 0xd2, 0x33,
	0x39, 0x41,
		// 0x22

	0x56, 0x4f, 0x4f, 0x9c, 0x51, 0x35, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xeb, 0x0e, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0x30, 0xfe, 0x0f, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11,
	0xff, 0x00,
		// 0x44

	0x65, 0x59, 0x59, 0x89, 0x5b, 0xbf, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe8, 0xbe, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11,
	0xff, 0x00
};
/*
 4F6Fh, 0B955934Fh
.text:80013408		       dd 40003E0Bh, 0,	0BE80000h, 0DF0000DFh, 0FFE30Ch, 1080053Ah
.text:80013408		       dd 0FF1100h, 9359596Fh, 3E0BBF5Bh, 4000h, 0, 0DF0BE8h
.text:80013408		       dd 0E30CDF00h, 53A00FFh,	11001080h, 4F5600FFh, 35519C4Fh
.text:80013408		       dd 6000F06Fh, 0,	8450000h, 3F00003Fh, 0FFE370h, 1080053Ah
.text:80013408		       dd 0FF1100h, 89595965h, 0F06FBF5Bh, 6000h, 0, 3F0845h
.text:80013408		       dd 0E3703F00h, 53A00FFh,	11001080h, 4F5900FFh, 39519D4Fh
.text:80013408		       dd 40003E0Bh, 0,	0EE80000h, 0DF0000DFh, 0FFE30Ch, 1080053Ah
.text:80013408		       dd 0FF1100h, 87595963h, 3E0BA35Bh, 4000h, 0, 0DF0EE8h
.text:80013408		       dd 0E30CDF00h, 53A00FFh,	11001080h, 4F7800FFh, 0A1579C4Fh
.text:80013408		       dd 40001FFCh, 0,	9E40000h, 0DF0000DFh, 0FFE3FDh,	1080053Ah
.text:80013408		       dd 0FF1100h, 8C9F9FC8h, 0F0EC31A7h, 6000h, 0, 0CF09D4h
.text:80013408		       dd 0E3EDCF00h, 53800FFh,	11001080h, 4F6700FFh, 0BF548B4Fh
.text:80013408		       dd 40001103h, 0,	6F10000h, 0EF0000EFh, 0FFE304h,	1080053Ah
.text:80013408		       dd 361100h, 88EFEF04h, 0F02F3FF4h, 6000h, 0, 1B021Dh, 0E3301B00h
.text:80013408		       dd 53800FFh, 11010080h, 4F650010h, 0BF5B894Fh, 40003E0Bh
.text:80013408		       dd 0, 0BE80000h,	0DF0000DFh, 0FFE30Ch, 1080053Ah, 0FF1100h
.text:80013408		       dd 824F4F7Eh, 3E0B3F5Bh,	4000h, 0, 0DF0BE8h, 0E30CDF00h
.text:80013408		       dd 53A00FFh, 11000080h, 576100FFh, 0BF598557h, 40003E0Bh
.text:80013408		       dd 0, 0EE80000h,	0DF0000DFh, 0FFE30Ch, 1080053Ah, 0FF1100h
.text:80013408		       dd 9C4F4F65h, 0F06F3551h, 6000h,	0, 3F0845h, 0E3703F00h
.text:80013408		       dd 53A00FFh, 11001080h, 0FFh
*/

// these are some kind of calibration table
// used to assess results of initial sampling/averaging action

const BYTE baVideoAvgCalibration[]={  // unknown function, first few bytes must be present for video to come up with ms bootloader

	// b7 clear on all bytes

	32,
	78,
	78,  //
	62,
	86,  // first comparison
	67,
	93,
	74,
	100,
	80,
	106,
	85

};

const BYTE baVideoInitTable2Standard[] = {

	// 2 x 5 x 19-byte structs follow, all seem to follow 4, 4, 4, 4, 3 structure
	// only low 4 bits used on each byte

	// first set of 5 structs have 1 in first byte of triplet

9,9,14,8,  // always ... 14, 8
15,15,15,15,  // all 0F s appears only in second DWORD of struct, always same
9,9,14,8,  // always ... 14, 8
9,9,14,8,  // always ... 14, 8
1,0,0, // always ... 0

//0x13
9,9,14,8,
15,15,15,15,
9,9,14,8,
9,9,14,8,
1,1,0,

//0x26
9,9,14,8,
15,15,15,15,
9,9,14,8,
9,9,14,8,
1,2,0,

//0x39
10,11,14,8,
15,15,15,15,
10,11,14,8,
10,11,14,8,
1,2,0,

//0x4c
11,14,14,8,
15,15,15,15,
11,14,14,8,
11,14,14,8,
1,3,0

};

const BYTE baVideoInitTable2Alternate[] = {
	// second set of structs have 0 in first byte of triplet
	// these are chosen if b17b16 of a DWORD read of MMIO+101000h are not 11

9,9,14,8,
15,15,15,15,
9,9,14,8,
9,9,14,8,
0,0,0,

9,9,14,8,
15,15,15,15,
9,9,14,8,
9,9,14,8,
0,1,0,

9,9,14,8,
15,15,15,15,
9,9,14,8,
9,9,14,8,
0,2,0,

10,11,14,8,
15,15,15,15,
10,11,14,8,
10,11,14,8,
0,2,0,

11,14,14,8,
15,15,15,15,
11,14,14,8,
11,14,14,8,
0,3,0
};


const BYTE baGraInit[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff };
const BYTE baSequencerInit[] = { 0x03, 0x21, 0x0f, 0x00, 0x06 };
const DWORD dwaVideoUnlockIoOfssets[] = {
	0xfd00122c, 0xfd00123c, 0xfd001230, 0xfd001240, 0xfd001234, 0xfd001244, 0xfd001238, 0xfd001248, 0xfd001214
};

/*
 dword_0_80012BD0	dd 3020100h, 7060504h, 0B0A0908h, 0F0E0D0Ch, 0F4A01h
				       ; DATA XREF: AvSetDisplayMode+2FFo
.text:80012BE4		       db 0
.text:80012BE5 byte_0_80012BE5 db 3 dup(0)	       ; DATA XREF: AvSetDisplayMode+310o
.text:80012BE8 dword_0_80012BE8	dd 0, 0F054000h	       ; DATA XREF: AvSetDisplayMode+2E0o
.text:80012BF0		       db 0FFh
.text:80012BF1 byte_0_80012BF1 db 3 dup(0)	       ; DATA XREF: AvSetDisplayMode+2F1o
.text:80012BF4 dword_0_80012BF4	dd 0F2103h	       ; DATA XREF: AvSetDisplayMode+2C1o
.text:80012BF8		       db 6
.text:80012BF9 dword_0_80012BF9	dd 0		       ; DATA XREF: AvSetDisplayMode+2D2o
.text:80012BFD		       align 4
.text:80012C00 dword_0_80012C00	dd 680880h	       ; DATA XREF: AvSetDisplayMode+3D2r
.text:80012C04		       dd 6 dup(21101100h), 21101101h, 21101100h, 21101115h, 21101111h
.text:80012C04		       dd 21101100h, 21101155h,	21101151h, 3 dup(21101100h), 21101101h
.text:80012C04		       dd 21101100h, 0
*/

//; dword_0_80012C50  DATA XREF: AvSetDisplayMode+282o

const DWORD dwaKernelVideoLookupTable[] = {
	0x680898, 0x68089C, 0x6808C0, 0x6808C4, 0x68084C,	0x680630,
	0x680800, 0x680804, 0x680808, 0x68080C, 0x680810, 0x680814,
	0x680818, 0x680820, 0x680824, 0x680828, 0x68082C, 0x680830,
	0x680834, 0x680838, 0x680848, 0x680680, 0x680684, 0x680688,
	0x68068C, 0x680690,

	// mode 1
	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080, 0x2, 0x1DF, 0x20C,
	0x1DF, 0x1EA, 0x1ED, 0x0, 0x1DF, 0x27F, 0x307, 0x257,
	0x28D, 0x2AD, 0x0, 0x27F, 0x10100111,	0x0C6ED0, 0x20D, 0x9B, 0x26C, 0x0,

	// mode 2
	0x10000000, 0x10000000, 0x0, 0x40801080,	0x801080,
	0x2, 0x1DF, 0x20C, 0x1DF, 0x1EA, 0x1ED, 0x0, 0x1DF, 0x2CF, 0x365,
	0x2A7,	0x2DF, 0x2FF, 0x0, 0x2CF, 0x10100111,	0x0DF05C, 0x20D,
	0x0AE,	0x2B8, 0x0,

	//mode 3
	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080,
	0x2, 0x1DF, 0x20C, 0x1DF, 0x1E8, 0x1EB, 0x0, 0x1DF, 0x27F, 0x3B5,
	0x257,	0x2B7, 0x2D7, 0x0, 0x27F, 0x10100111,	0x0F387C, 0x271,
	0x0BE,	0x2F8, 0x0,
	//mode 4
	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080,
	0x2, 0x1DF, 0x20C, 0x1DF, 0x1E8, 0x1EB, 0x0, 0x1DF, 0x2CF, 0x419,
	0x2AF,	0x30B, 0x32B, 0x0, 0x2CF, 0x10100111,	0x10D2A4, 0x271,
	0x0D2,	0x348, 0x0,
	// mode 5
	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080,
	0x2, 0x23F, 0x270, 0x23F, 0x249, 0x24C, 0x0, 0x23F, 0x27F, 0x313,
	0x257,	0x28F, 0x2AF, 0x0, 0x27F, 0x10100111,	0x0F07A8, 0x271,
	0x9D, 0x276, 0x0,
	// mode 6
	0x10000000, 0x10000000, 0x0, 0x40801080,	0x801080,
	0x2, 0x23F, 0x270, 0x23F, 0x249, 0x24C, 0x0, 0x23F, 0x2CF, 0x375,
	0x2AF,	0x2E1, 0x302, 0x0, 0x2CF, 0x10100111,	0x10E62C, 0x271,
	0x0B1,	0x2C4, 0x0,
	// mode 7
	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080,
	0x2, 0x1DF, 0x20C, 0x1DF, 0x1E8, 0x1EE, 0x0, 0x1DF, 0x2CF, 0x359,
	0x29F,	0x2E1, 0x320, 0x0, 0x2CF, 0x10100011,	0x35A, 0x1, 0x0AB,
	0x2AE,	0x1,
	// mode 8
	0x10000000, 0x10000000, 0x0,	0x40801080, 0x801080, 0x2,
	0x1DF,	0x20C, 0x1DF, 0x1E8, 0x1EE,	0x0, 0x1DF, 0x2CF, 0x359, 0x29F,
	0x2E1,	0x320, 0x0, 0x2CF, 0x10100111, 0x35A,	0x1, 0x0AB, 0x2AE,
	0x1, 0x0AA94000,

	// this seems wrong, but the disassembly shows only one 0x10000000 here

	0x10000000, 0x0, 0x40801080, 0x801080, 0x3, 0x2CF,
	0x2ED,	0x2CF, 0x2D4, 0x2D9, 0x0, 0x2CF, 0x4FF, 0x671, 0x4FF, 0x545,
	0x595,	0x0A0, 0x45F, 0x10100011, 0x672, 0x1,	0x14A, 0x528, 0x1,
	0x2,

	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080, 0x3, 0x2CF, 0x2ED,
	0x2CF,	0x2D4, 0x2D9, 0x0, 0x2CF, 0x4FF, 0x671, 0x4CF, 0x545, 0x595,
	0x0, 0x4FF, 0x10100011, 0x672, 0x1, 0x14A, 0x528, 0x1,

	0x10000000, 0x10000000,
	0x0, 0x40801080,	0x801080, 0x3, 0x2CF, 0x2ED,	0x2CF, 0x2D4, 0x2D9,
	0x0, 0x2CF, 0x4FF, 0x671, 0x4FF, 0x545, 0x595, 0x0, 0x4FF, 0x10100111,
	0x672,	0x1, 0x14A, 0x528, 0x1, 0x71AE000,

	0x7183800, 0x0, 0x40801080,
	0x801080, 0x3, 0x437, 0x464, 0x43C, 0x43c, 0x446, 0x0,	0x437,
	0x77F,	0x897, 0x7AA, 0x7AB, 0x803,	0x0F0, 0x68F, 0x10133011,
	0x898,	0x1, 0x1B8, 0x6E0, 0x1, 0x10000000, 0x7183800,

	0x0, 0x40801080,
	0x801080, 0x3, 0x437, 0x464, 0x43C, 0x43c, 0x446, 0x0,	0x437,
	0x77F,	0x897, 0x759, 0x7AB, 0x803,	0x0, 0x77F, 0x10133011, 0x898,
	0x1, 0x1B8, 0x6E0, 0x1,

	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080,
	0x3, 0x437, 0x464, 0x43B, 0x43b, 0x445, 0x0, 0x437, 0x77F, 0x897,
	0x7AB,	0x7AC, 0x804, 0x0, 0x77F, 0x10133111,	0x898, 0x1, 0x1B8,
	0x6E0,	0x1,

	0x10000000, 0x10000000, 0x0,	0x40801080, 0x801080, 0x2,
	0x1DF,	0x20C, 0x1DF, 0x1EA, 0x1ED,	0x0, 0x1DF, 0x27F, 0x355, 0x2A7,
	0x2B0,	0x2D0, 0x0, 0x27F, 0x10100111, 0x0DF05C, 0x20D, 0x0AE,
	0x2B8,	0x0,

	0x10000000, 0x10000000, 0x0,	0x40801080, 0x801080, 0x2,
	0x1DF,	0x20C, 0x1DF, 0x1EA, 0x1ED,	0x0, 0x1DF, 0x27F, 0x419, 0x2A7,
	0x2DB,	0x2FB, 0x0, 0x27F, 0x10100111, 0x0DF05C, 0x20D, 0x0AE,
	0x2B8,	0x0,

	0x10000000, 0x10000000, 0x0,	0x40801080, 0x801080, 0x2,
	0x1DF,	0x20C, 0x1DF, 0x1E8, 0x1EE,	0x0, 0x1DF, 0x2CF, 0x359, 0x2CF,
	0x2DF,	0x31E, 0x20, 0x2AD, 0x10100011, 0x35A, 0x1, 0x0AB, 0x2AE,
	0x1,

	0x10000000, 0x10000000, 0x0, 0x40801080, 0x801080, 0x2, 0x23F,
	0x270,	0x23F, 0x249, 0x24C, 0x0, 0x23F, 0x27F, 0x363, 0x257, 0x2B0,
	0x2D0,	0x0, 0x27F, 0x10100111, 0x0F07A8, 0x271, 0x9D,	0x276, 0x0
};

/*
.text:80013408 dword_0_80013408	dd 3020100h, 7060504h, 0B0A0908h, 0F0E0D0Ch, 13121110h
.text:80013408					       ; DATA XREF: AvSetDisplayMode+323r
.text:80013408					       ; AvSetDisplayMode+341o
.text:80013408		       dd 17161514h, 1B1A1918h,	332D2520h, 4F564139h, 35519C4Fh
.text:80013408		       dd 40003E0Bh, 0,	0EEB0000h, 0DF0000DFh, 0FFE30Ch, 1080053Ah
.text:80013408		       dd 0FF1100h, 89595965h, 3E0BBF5Bh, 4000h, 0, 0DF0BE8h
.text:80013408		       dd 0E30CDF00h, 53A00FFh,	11001080h, 4F6F00FFh, 0B955934Fh
.text:80013408		       dd 40003E0Bh, 0,	0BE80000h, 0DF0000DFh, 0FFE30Ch, 1080053Ah
.text:80013408		       dd 0FF1100h, 9359596Fh, 3E0BBF5Bh, 4000h, 0, 0DF0BE8h
.text:80013408		       dd 0E30CDF00h, 53A00FFh,	11001080h, 4F5600FFh, 35519C4Fh
.text:80013408		       dd 6000F06Fh, 0,	8450000h, 3F00003Fh, 0FFE370h, 1080053Ah
.text:80013408		       dd 0FF1100h, 89595965h, 0F06FBF5Bh, 6000h, 0, 3F0845h
.text:80013408		       dd 0E3703F00h, 53A00FFh,	11001080h, 4F5900FFh, 39519D4Fh
.text:80013408		       dd 40003E0Bh, 0,	0EE80000h, 0DF0000DFh, 0FFE30Ch, 1080053Ah
.text:80013408		       dd 0FF1100h, 87595963h, 3E0BA35Bh, 4000h, 0, 0DF0EE8h
.text:80013408		       dd 0E30CDF00h, 53A00FFh,	11001080h, 4F7800FFh, 0A1579C4Fh
.text:80013408		       dd 40001FFCh, 0,	9E40000h, 0DF0000DFh, 0FFE3FDh,	1080053Ah
.text:80013408		       dd 0FF1100h, 8C9F9FC8h, 0F0EC31A7h, 6000h, 0, 0CF09D4h
.text:80013408		       dd 0E3EDCF00h, 53800FFh,	11001080h, 4F6700FFh, 0BF548B4Fh
.text:80013408		       dd 40001103h, 0,	6F10000h, 0EF0000EFh, 0FFE304h,	1080053Ah
.text:80013408		       dd 361100h, 88EFEF04h, 0F02F3FF4h, 6000h, 0, 1B021Dh, 0E3301B00h
.text:80013408		       dd 53800FFh, 11010080h, 4F650010h, 0BF5B894Fh, 40003E0Bh
.text:80013408		       dd 0, 0BE80000h,	0DF0000DFh, 0FFE30Ch, 1080053Ah, 0FF1100h
.text:80013408		       dd 824F4F7Eh, 3E0B3F5Bh,	4000h, 0, 0DF0BE8h, 0E30CDF00h
.text:80013408		       dd 53A00FFh, 11000080h, 576100FFh, 0BF598557h, 40003E0Bh
.text:80013408		       dd 0, 0EE80000h,	0DF0000DFh, 0FFE30Ch, 1080053Ah, 0FF1100h
.text:80013408		       dd 9C4F4F65h, 0F06F3551h, 6000h,	0, 3F0845h, 0E3703F00h
.text:80013408		       dd 53A00FFh, 11001080h, 0FFh
.text:8001364C dword_0_8001364C	dd 3C322ED6h, 0C6C4403Eh, 9C9EA0CEh, 48AD0C6Ch,	18C8C90h
.text:8001364C					       ; DATA XREF: AvSetDisplayMode+194r
.text:8001364C					       ; AvSetDisplayMode+1A8o
.text:8001364C		       dd 8CE19Ch, 0AA0C4600h, 8C8C9049h, 21E19C01h, 0C460000h
.text:8001364C		       dd 8C9049ABh, 0E19C018Ch, 46000021h
.text:80013680 dword_0_80013680	dd 5A3E36D6h, 6A68665Ch, 7C7A7876h, 8482807Eh, 8C8A8886h
.text:80013680					       ; DATA XREF: AvSetDisplayMode+1F4r
.text:80013680					       ; AvSetDisplayMode+204o
.text:80013680		       dd 9492908Eh, 9C9A9896h,	0A4A2A09Eh, 0ACAAA8A6h,	0B4B2B0AEh
.text:80013680		       dd 0DCDACEB6h, 0E4E2E0DEh, 0ECEAE8E6h, 0F4F2F0EEh, 0FCFAF8F6h
.text:80013680		       dd 0, 1FF80A4h, 10004B15h, 44857288h, 26F213EDh,	3730800h
.text:80013680		       dd 6E0240Dh, 0DA681000h,	7CE40A0Ah, 129A8FCBh, 5A258699h
.text:80013680		       dd 20FC0F19h, 0F6FD0h, 9F30C00h,	90B566BDh, 7DB2h, 0A4000000h
.text:80013680		       dd 0F801FF80h, 0D8CC0042h, 75C9880h, 26F213h, 0D037F66h
.text:80013680		       dd 6E024h, 0C2AF750h, 0CA7BE40Ah, 0A4C89A8Eh, 195E2178h
.text:80013680		       dd 0D020FC0Fh, 0F6Fh, 0BD09F30Ch, 0B290B566h, 7Dh, 80A40000h
.text:80013680		       dd 4B1501FFh, 72881000h,	13ED4485h, 80026F2h, 240D0373h
.text:80013680		       dd 100006E0h, 80ADA68h, 9ADC7CE4h, 869912A7h, 0F195A25h
.text:80013680		       dd 6FD020FCh, 0C00000Fh,	66BD09F3h, 7DB290B5h, 0, 0FF80A400h
.text:80013680		       dd 42F801h, 9880D8CCh, 0F213075Ch, 7F660026h, 0E0240D03h
.text:80013680		       dd 0F7500006h, 0E4080C2Ah, 0A79ADB7Bh, 2178A4C8h, 0FC0F195Eh
.text:80013680		       dd 0F6FD020h, 0F30C0000h, 0B566BD09h, 7DB290h, 0, 80A4h
.text:80013680		       dd 10004B15h, 4C8E7288h,	26F213EDh, 3730800h, 6E0240Dh
.text:80013680		       dd 0DA681000h, 58F02A0Ah, 8FA492CDh, 5A257C0Ah, 20FC0F19h
.text:80013680		       dd 0F6FD0h, 9F30C00h, 90B566BDh,	7DB2h, 0A4000000h, 0F8000080h
.text:80013680		       dd 0D8CC0042h, 7649F80h,	26F213h, 0D037F66h, 6E024h, 0C2AF750h
.text:80013680		       dd 8057F00Ah, 3A438C48h,	195A216Fh, 0D020FC0Fh, 0F6Fh
.text:80013680		       dd 0BD09F30Ch, 0B290B566h, 7Dh, 80E40000h, 35410000h, 76883C03h
.text:80013680		       dd 1615448Ch, 0B600A620h, 230D03F7h, 4AE106E0h, 240B1555h
.text:80013680		       dd 9BD95EF0h, 839833A3h,	519522Dh, 6E402057h, 0F51F47Eh
.text:80013680		       dd 78D305F1h, 0A55425A2h, 0, 0FF80E400h,	2E70800h, 9A82D8E4h
.text:80013680		       dd 20162758h, 81A00A6h, 0E0230D0Ch, 4AE106h, 0F0240C40h
.text:80013680		       dd 0A499D75Ch, 292DEB3Ah, 57051956h, 7E6E4020h, 0F10F51F4h
.text:80013680		       dd 0A278D305h, 0A55425h,	0, 0FF80D2h, 28033FADh,	428A7488h
.text:80013680		       dd 0A622160Ah, 37E1400h,	0A402A71h, 0F1C75000h, 5EF0240Ah
.text:80013680		       dd 18A49AD9h, 2E1775h, 20570519h, 0F47E6E40h, 5F10F51h
.text:80013680		       dd 25A278D3h, 0A554h, 0D2000000h, 0AF000080h, 0D8EC02E3h
.text:80013680		       dd 2B5A9C82h, 0A62216h, 71038D76h, 0A402Ah, 0C4E3950h
.text:80013680		       dd 0D75DF024h, 5357A399h, 195628FEh, 40205705h, 51F47E6Eh
.text:80013680		       dd 0D305F10Fh, 5425A278h, 0A5h, 80A40000h, 4B150000h, 72881000h
.text:80013680		       dd 13ED4088h, 80026F2h, 240D0373h, 100006E0h, 200ADA68h
.text:80013680		       dd 9BD95EF0h, 7AB77DA3h,	0F19552Eh, 6FD020FCh, 0C00000Fh
.text:80013680		       dd 66BD09F3h, 7DB290B5h,	0, 80C000h, 42F800h, 9A80D8CCh
.text:80013680		       dd 0F2130756h, 7F660026h, 0E0240D03h, 0F7500006h, 0F0200C2Ah
.text:80013680		       dd 0A398D75Ch, 29751BB7h, 0FC0F1955h, 0F6FD020h,	0F30C0000h
.text:80013680		       dd 0B566BD09h, 7DB290h, 8000000h, 30090A4h, 10004B15h
.text:80013680		       dd 44867288h, 26F213EDh,	3730800h, 6E0240Dh, 0DA681000h
.text:80013680		       dd 77F0080Ah, 12A28D9Eh,	5A258699h, 20FC0FE1h, 0F6FD0h
.text:80013680		       dd 9F30C00h, 90B566BDh, 7DB2h, 0A4080000h, 0F8030090h
.text:80013680		       dd 0D8CC0042h, 75B9780h,	26F213h, 0D037F66h, 6E024h, 0C2AF750h
.text:80013680		       dd 9E76F008h, 0A4C8A28Dh, 0E15A2178h, 0D020FC0Fh, 0F6Fh
.text:80013680		       dd 0BD09F30Ch, 0B290B566h, 7Dh, 80A40000h, 443901FFh, 7EC0AC00h
.text:80013680		       dd 130D5695h, 560026F2h,	240D0383h, 500006E0h, 0A0BF1AEh
.text:80013680		       dd 8ECA7BE4h, 192E2A9Ah,	0F195D22h, 6FD020FCh, 0C00000Fh
.text:80013680		       dd 66BD09F3h, 7DB290B5h,	0, 0FF80A400h, 443901h,	957EC0ACh
.text:80013680		       dd 0F2130D56h, 83560026h, 0E0240D03h, 0AE500006h, 0E4080BF1h
.text:80013680		       dd 0A79ADA7Bh, 22192E2Ah, 0FC0F195Dh, 0F6FD020h,	0F30C0000h
.text:80013680		       dd 0B566BD09h, 7DB290h, 0, 80A4h, 0AC004439h, 52967EC0h
.text:80013680		       dd 26F2130Dh, 3835600h, 6E0240Dh, 0F1AE5000h, 5DF0200Bh
.text:80013680		       dd 0DEA399D7h, 552A3BF2h, 20FC0F19h, 0F6FD0h, 9F30C00h
.text:80013680		       dd 90B566BDh, 7DB2h, 0A4080000h,	39030090h, 0C0AC0044h
.text:80013680		       dd 0D568E7Eh, 26F213h, 0D038356h, 6E024h, 0BF1AE50h, 9E76F008h
.text:80013680		       dd 2E2AA28Dh, 0E15A2219h, 0D020FC0Fh, 0F6Fh, 0BD09F30Ch
.text:80013680		       dd 0B290B566h, 7Dh, 80D20000h, 354000FFh, 82C0E403h, 163B589Ah
.text:80013680		       dd 1A00A620h, 230D0C1Bh,	4AE106E0h, 240C4000h, 99D75CF0h
.text:80013680		       dd 2DEB3AA4h, 0F195629h,	6FD020FCh, 0C00000Fh, 66BD09F3h
.text:80013680		       dd 7DB290B5h, 0,	90E408h, 3354100h, 8C76883Ch, 20161544h
.text:80013680		       dd 0F7B600A6h, 0E0230D03h, 554AE106h, 0F0240B15h, 0A38E9E59h
.text:80013680		       dd 2D839833h, 5705E151h,	7E6E4020h, 0F10F51F4h, 0A278D305h
.text:80013680		       dd 0A55425h, 8000000h, 90E4h, 0E402E708h, 589A82D8h, 0A6201627h
.text:80013680		       dd 0C081A00h, 6E0230Dh, 40004AE1h, 59F0240Ch, 3AA38E9Eh
.text:80013680		       dd 55292DEBh, 205705E1h,	0F47E6E40h, 5F10F51h, 25A278D3h
.text:80013680		       dd 0A554h, 0D2080000h, 0AD000090h, 8828033Fh, 0A428A74h
.text:80013680		       dd 0A62216h, 71037E14h, 0A402Ah,	0AF1C750h, 9E5AF024h
.text:80013680		       dd 7518A38Eh, 0E1002E17h, 40205705h, 51F47E6Eh, 0D305F10Fh
.text:80013680		       dd 5425A278h, 0A5h, 90D20800h, 0E3AF0000h, 82D8EC02h, 162B5A9Ch
.text:80013680		       dd 7600A622h, 2A71038Dh,	50000A40h, 240C4E39h, 8E9E59F0h
.text:80013680		       dd 0FE5357A3h, 5E15628h,	6E402057h, 0F51F47Eh, 78D305F1h
.text:80013680		       dd 0A55425A2h, 0, 90D208h, 3354000h, 9A82C0E4h, 20163B58h
.text:80013680		       dd 1B1A00A6h, 0E0230D0Ch, 4AE106h, 0F0240C40h, 0A38E9E59h
.text:80013680		       dd 292DEB3Ah, 0FC0FE156h, 0F6FD020h, 0F30C0000h,	0B566BD09h
.text:80013680		       dd 7DB290h, 8000000h, 90A4h, 10004B15h, 40887288h, 26F213EDh
.text:80013680		       dd 3730800h, 6E0240Dh, 0DA681000h, 5AF0200Ah, 7D8C4A84h
.text:80013680		       dd 5A2E7AB7h, 20FC0FE1h,	0F6FD0h, 9F30C00h, 90B566BDh
.text:80013680		       dd 7DB2h, 0C0080000h, 0F8000090h, 0D8CC0042h, 7569A80h
.text:80013680		       dd 26F213h, 0D037F66h, 6E024h, 0C2AF750h, 8259F020h, 1BB78C49h
.text:80013680		       dd 0E15A2975h, 0D020FC0Fh, 0F6Fh, 0BD09F30Ch, 0B290B566h
.text:80013680		       dd 7Dh, 90A40800h, 44390000h, 7EC0AC00h,	130D5296h, 560026F2h
.text:80013680		       dd 240D0383h, 500006E0h,	200BF1AEh, 498259F0h, 3BF2DE8Ch
.text:80013680		       dd 0FE15A2Ah, 6FD020FCh,	0C00000Fh, 66BD09F3h, 7DB290B5h
.text:80013680		       dd 0, 0FF80D200h, 2F30500h, 9880C0C8h, 22162B54h, 8F6400A6h
.text:80013680		       dd 402A7103h, 3950000Ah,	0F0240C0Eh, 0A499D75Dh,	29D7F326h
.text:80013680		       dd 0FC0F1955h, 0F6FD020h, 0F30C0000h, 0B566BD09h, 7DB290h
.text:80013680		       dd 8000000h, 90D2h, 0C802F305h, 549880C0h, 0A6221628h
.text:80013680		       dd 38F6400h, 0A402A71h, 0E395000h, 59F0240Ch, 26A38E9Eh
.text:80013680		       dd 29D7F3h, 20FC0FE1h, 0F6FD0h, 9F30C00h, 90B566BDh, 7DB2h
.text:80013680		       dd 0, 3432302Eh,	403C3A38h, 6C646260h, 7472706Eh, 0CAC8C6C4h
.text:80013DF8		       db 0CCh,	0D8h
.text:80013DFA dword_0_80013DFA	dd 80280000h, 80800000h, 46000000h, 1000F00h, 0C09B9C01h
.text:80013DFA					       ; DATA XREF: AvSetDisplayMode+1CEo
.text:80013E0E		       db 0C0h,	40h
*/



// memory mapped IO

void vgaout(unsigned int port, unsigned char reg, unsigned char data) {
	*((volatile unsigned char*)(port)) = reg;
	*((volatile unsigned char*)(port+1)) = data;
}

void voutb(int nAds, BYTE b) {
	*((volatile unsigned char*)(nAds)) = b;
}
void voutl(int nAds, DWORD dw) {
	*((volatile DWORD*)(nAds)) = dw;
}
BYTE vinb(int nAds) {
	return *((volatile unsigned char*)(nAds)) ;
}

void GetTickCount(DWORD * pdw1, DWORD * pdw2)
{
	DWORD dw1, dw2;
	
	__asm__ __volatile__ (
		"rdtsc " : "=a" (dw1), "=d" (dw2)
	);
	*pdw1=dw1; *pdw2=dw2;
}

void BootVideoDelay(void)
{
	DWORD dw1, dw2;
	GetTickCount(&dw1, &dw2);
	{
		DWORD dw3, dw4;
		while(1) {
			GetTickCount(&dw3, &dw4);
			if((dw3-dw1)>10000) return;
		}
	}
}


DWORD vinl(int nAds) {
	return *((volatile DWORD*)(nAds)) ;
}
static int nDelay;

	void	I2CTransmitWordDelay(BYTE b, WORD w, bool f) {
				int u;
				I2CTransmitWord(b, w, f);
				for(u=0;u<100;u++) nDelay++;
		}

	int I2CTransmitByteGetReturnDelay(BYTE b1, BYTE b2) {
					int u, n;
		n=I2CTransmitByteGetReturn(b1, b2);
				for(u=0;u<100;u++) nDelay++;
			return n;
		}


DWORD Checkerboard(DWORD dwA, DWORD dwB) { return ( dwA | (dwB<<4) |	(dwA<<8) | (dwB<<12) | (dwA<<16) | (dwB<<20) | (dwA<<24) | (dwB<<28) ); }

void BootVgaInitialization() {

	int i=0;

	int nVideoEncodingType=2; // pal??
	int nVar14=1; // ????
	int nArg10=640;
	DWORD dwStash;

			////////////
			///  2bl Video init
			///////////////


		{  // functionality as in 2bl
		const BYTE * pbaVideoInitSelected = baVideoInitTable2Alternate;
		const BYTE * pbaEdi, *pbaEsi, *pbaEcx;
		DWORD dwaTemp[8];
		DWORD dw=vinl(0xfd101000);
		int n, nDivisionResult;
		DWORD dwTemp;
		DWORD dwArg[0x20] ;

		if(dw & 0x000c0000) {  // selects between the two init tables based on resigter value
			pbaVideoInitSelected=baVideoInitTable2Standard;
		}

		// +9AF

		voutl(0xfd001220, 0x0f0f0f);  // enter this setup mode

			// form two 4-sample averages

		for(n=0;n<8;n++) {
			voutl(0xfd001228, 0x40000+(n^4));
			BootVideoDelay();
			dwaTemp[n^4]=vinl(0xfd00121c);
//			bprintf("  v2bl: %d: %x\n", n^4, dwaTemp[n^4]);
		}

		voutl(0xfd001220, 0x0f0f0f);  // exit setup mode

		dwaTemp[5]=(dwaTemp[4]+dwaTemp[5]+dwaTemp[6]+dwaTemp[7])>>2;  // sum1 seeing 5e..60
		dwaTemp[4]=(dwaTemp[0]+dwaTemp[1]+dwaTemp[2]+dwaTemp[3])>>2;  // sum2 seeing 6c..6d
//		bprintf("  v2bl: sum1=%x, sum2=%x\n", dwaTemp[5], dwaTemp[4]);

			// sort the results into four categories

		if(dwaTemp[5] <= baVideoAvgCalibration[4]) {  // dwSum1 <= 0x56   CATEGORY 1/4

				nDivisionResult=((dwaTemp[5]-baVideoAvgCalibration[2])<<20) / (baVideoAvgCalibration[4]-baVideoAvgCalibration[2]);
				pbaEsi=&pbaVideoInitSelected[0x4c];
				pbaEdi=&pbaVideoInitSelected[0x39];
				if(nDivisionResult<0) dwaTemp[1]=0;

		} else { // ADFh dwSum1>0x56

			dwaTemp[0]=baVideoAvgCalibration[8];

			if(dwaTemp[5]>baVideoAvgCalibration[8]) { // >= 0x64  CATEGORY 2/4

				nDivisionResult=((dwaTemp[5]-baVideoAvgCalibration[8])<<20) / (baVideoAvgCalibration[10]-baVideoAvgCalibration[8]);

				pbaEsi=&pbaVideoInitSelected[0x13];
				pbaEdi=&pbaVideoInitSelected[0];

				if(nDivisionResult>0x100000) dwaTemp[1]=0; else dwaTemp[1]=nDivisionResult;  // bizarre but true

			} else { // dwSum1 between 0x57 and 0x64

				if(dwaTemp[5]<baVideoAvgCalibration[6]) { // B18 >= 0x5d  CATEGORY 3/4

					nDivisionResult=((dwaTemp[5]-baVideoAvgCalibration[6])<<20) / (baVideoAvgCalibration[6]-baVideoAvgCalibration[4]);  // edx

					dwaTemp[5]=(baVideoAvgCalibration[6]-baVideoAvgCalibration[4]);

					pbaEsi=&pbaVideoInitSelected[0x39];
					pbaEdi=&pbaVideoInitSelected[0x26];

					// nb - b54!!!
					dwaTemp[1]=nDivisionResult;

				} else { // B3D  CATEGORY 4/4

					nDivisionResult=((dwaTemp[5]-baVideoAvgCalibration[6])<<20) / (dwaTemp[0]-baVideoAvgCalibration[6]);  // edx

					dwaTemp[5]=(dwaTemp[0]-baVideoAvgCalibration[6]);

					pbaEsi=&pbaVideoInitSelected[0x26];
					pbaEdi=&pbaVideoInitSelected[0x13];

					// nb - b54!!!
					dwaTemp[1]=nDivisionResult;

				}
			}
		}

			// B57 - all four categories rejoin here

		{
			{
				int nd=pbaEdi[0], ns=pbaEsi[0];
				nd=nd * dwaTemp[1];
				ns = (ns * (0x100000 - dwaTemp[1]) );
				nd = nd + ns;
				dwaTemp[5]=(0x100000 - dwaTemp[1]);
				dwaTemp[0]=((nd+0x80000)>>20)&0xf;  // <----
			}
			{
				int nd=pbaEdi[1], ns=pbaEsi[1];
				ns=ns * dwaTemp[5];
				nd=nd * dwaTemp[1];
				ns+=nd;
				dwTemp=dwaTemp[1]; // this is held in ecx
				dwaTemp[1]=((ns+0x80000)>>20)&0xf;
			}
			{
				int nd=pbaEdi[2], ns=pbaEsi[2];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x40>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[3], ns=pbaEsi[3];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x28>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[4], ns=pbaEsi[4];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x48>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[5], ns=pbaEsi[5];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x30>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[6], ns=pbaEsi[6];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x54>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[7], ns=pbaEsi[7];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x20>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[8], ns=pbaEsi[8];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x38>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[9], ns=pbaEsi[9];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x50>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[0xa], ns=pbaEsi[0xa];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x4c>>2]=((ns+0x80000)>>20)&0xf;
			}

			{
				int nd=pbaEdi[0xb], ns=pbaEsi[0xb];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x44>>2]=((ns+0x80000)>>20)&0xf;
			}
			{
				int nd=pbaEdi[0xc], ns=pbaEsi[0xc];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x3c>>2]=((ns+0x80000)>>20)&0xf;
			}
			{
				int nd=pbaEdi[0xd], ns=pbaEsi[0xd];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x34>>2]=((ns+0x80000)>>20)&0xf;
			}
			{
				int nd=pbaEdi[0xe], ns=pbaEsi[0xe];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x2c>>2]=((ns+0x80000)>>20)&0xf;
			}
			{
				int nd=pbaEdi[0xf], ns=pbaEsi[0xf];
				ns=ns * dwaTemp[5];
				nd=nd * dwTemp; // from ecx
				ns+=nd;
				dwArg[0x24>>2]=((ns+0x80000)>>20)&0xf;
			}

		}

			// now we look at the second average, again there are four categories
			// dwaTemp[5] destroyed here

		pbaEsi=&pbaVideoInitSelected[0];

		if( dwaTemp[4] <= baVideoAvgCalibration[5]) { // category 1/4
			nDivisionResult=((dwaTemp[4] - baVideoAvgCalibration[3]) <<20) / (baVideoAvgCalibration[5] - baVideoAvgCalibration[3]) ; // eax / edi
			pbaEcx =&pbaVideoInitSelected[0x4c];
			pbaEsi+=0x39;
			if(nDivisionResult<0) nDivisionResult=0;
		} else {  // d23
			dwaTemp[5] =baVideoAvgCalibration[9];
			if( dwaTemp[4] >= baVideoAvgCalibration[9]) { // category 2/4
				nDivisionResult = ((  dwaTemp[4]-baVideoAvgCalibration[9] )<<20) / (baVideoAvgCalibration[0xb]-baVideoAvgCalibration[9]); // eax/edi
				pbaEcx =&pbaVideoInitSelected[0x13];
				if(nDivisionResult > 0x100000) nDivisionResult=1;
			} else { // d53
				if( dwaTemp[4] < baVideoAvgCalibration[7]) { // category 3/4
					dwaTemp[5]=(baVideoAvgCalibration[7]-baVideoAvgCalibration[5]);
					nDivisionResult = ((  dwaTemp[4]-baVideoAvgCalibration[7] )<<20) / dwaTemp[5]; // eax/edi
					pbaEcx =&pbaVideoInitSelected[0x39];
					pbaEsi+=0x26;
				} else { // d75   category 4/4

					dwaTemp[5]=(dwaTemp[5] - baVideoAvgCalibration[7]);
					pbaEcx =&pbaVideoInitSelected[0x26];
					pbaEsi+=0x13;
					nDivisionResult = ((dwaTemp[4] - baVideoAvgCalibration[7] )<<20) / dwaTemp[5];
				}
			}
		}

			// d8e we rejoin main flow

		{
			int n, n1, n2;
			DWORD dwaFinalTable[0x9], dw;

			dwaTemp[5]=(0x100000 -nDivisionResult);
			n=pbaEcx[0x12] * dwaTemp[5]; // eax

			n1=pbaEsi[0x12] *nDivisionResult;
			n1+=n;
			dwArg[0x64>>2]=((n1+0x80000)>>20)&0xf;  // video_code_base trashed here

			n1=(pbaEsi[0x10]*nDivisionResult) + (pbaEcx[0x10] *dwaTemp[5]);
			n=((n1+0x80000)>>20)&0xf; // edi

			n1=(pbaEcx[0x11] *dwaTemp[5]) + (pbaEsi[0x11] *nDivisionResult);
			n2=((n1+0x80000)>>20)&0xf; // n2=esi

			__asm__ __volatile__ ("wbinvd");

				// combine this amazing amount of crap into a short table to be poked into the chip
				// surely this is all to do with 'protection', its all too stupid and pointless
				// the last one is especially stupid

			dwaFinalTable[0] = Checkerboard(dwaTemp[0], dwaTemp[1]);
			dwaFinalTable[1] = Checkerboard(dwArg[0x40>>2], dwArg[0x28>>2]);
			dwaFinalTable[2] = Checkerboard(dwArg[0x48>>2], dwArg[0x30>>2]);
			dwaFinalTable[3] = Checkerboard(dwArg[0x54>>2], dwArg[0x20>>2]);
			dwaFinalTable[4] = Checkerboard(dwArg[0x38>>2], dwArg[0x50>>2]);
			dwaFinalTable[5] = Checkerboard(dwArg[0x4c>>2], dwArg[0x44>>2]);
			dwaFinalTable[6] = Checkerboard(dwArg[0x3c>>2], dwArg[0x34>>2]);
			dwaFinalTable[7] = Checkerboard(dwArg[0x2c>>2], dwArg[0x24>>2]);

			dw=dwArg[0x64>>2];
			dw<<=2; dw|=n2;
			dw<<=3; dw|=n;
			dw<<=3; dw|=dwArg[0x64>>2];
			dw<<=2; dw|=n2;
			dw<<=3; dw|=n;
			dw<<=3; dw|=dwArg[0x64>>2];
			dw<<=2; dw|=n2;
			dw<<=3; dw|=n;
			dw<<=3; dw|=dwArg[0x64>>2];
			dw<<=2; dw|=n2;
			dw<<=3; dw|=n;
			dwaFinalTable[8] = dw;

				// issue the final table values to a fixed set of registers

			for(n=0;n<9;n++) {
				if(dwaFinalTable[n]==0) {
					voutl(0xfd000000, 0);
				} else {
//					bprintf("  v2bl: init %d: 0x%02X\n", n, dwaFinalTable[n]);
					voutl(dwaVideoUnlockIoOfssets[n], dwaFinalTable[n]);
				}
			}
		}
	}

			////////////
			///  Kernel Video init
			///////////////

		{
			bool fMore=true;
			IoOutputByte(0x80d3, 5);  // loc_0_80030E3A:

			while(fMore) {  // some kind of wait for ready loop?
				dwStash=vinl(0xfd6806A0);  // var_18
				if( ! (((!(dwStash >> 4)) ^ (!dwStash)) & 1) ) fMore=false;
			}

			vgaout(0xfd6013D4, 0x1f, 0x57);
			vgaout(0xfd6013D4, 0x21, 0xff);

			voutl(0xfd680880,	0x21121111);

			I2CTransmitWordDelay(0x45, 0x6c46, false);
			BootVideoDelay();
			I2CTransmitWordDelay(0x45, 0x6cc6, false);
		}

		vgaout(0xfd6013D4, 0x13, nArg10/8); // width/8

		{
			int n=0;
			while(n<13) {
				I2CTransmitWordDelay(0x45, (((DWORD)baConextantAddressesToWrite[n])<<8)|baConextantValuesToWriteDependingOnVideoType[nVideoEncodingType][n], false);
				n++;
			}
		}

		for(i=0;i<(sizeof(waConextantInitNon10CommandMSB)/sizeof(WORD));i++) {  // issue Encoder setup table
			I2CTransmitWordDelay(0x45, waConextantInitNon10CommandMSB[i], false);
		}

		for(i=0;i<(sizeof(baConextantAddressesToWrite2)/sizeof(BYTE));i++) {  // issue Encoder setup table
			I2CTransmitWordDelay(0x45,
				(((DWORD)baConextantAddressesToWrite2[i])<<8)|
				*(((BYTE *)&dwaConextantValuesToWriteDependingOnVideoType2[0])+(0x3f * nVideoEncodingType)+i),
				false
			);
		}



		IoOutputByte(0x80d6, 4);
		IoOutputByte(0x80d8, 4);

		voutl(0xfd68050c,	vinl(0xfd68050c)|0x10020000);

		voutb(0xfd0C03C3, 1);
		voutb(0xfd0C03C2, 0xe3);

		voutl(0xfd680600, 0x0);  // this is actually set to a parameter to the kernel routine

		vgaout(0xfd6013D4, 0x11, 0);
		
		voutl(0xfd680630, 0);
		voutl(0xfd6808c4, 0);
		voutl(0xfd68084c, 0);

		vgaout(0xfd6013D4, 0x19, 0xe0);
		vgaout(0xfd6013D4, 0x28, 0x80);
		vgaout(0xfd6013D4, 0x28, 0x00);

		{
			int n=0, nCount=0x1a;
			int nMode=9;
			while(nCount--) {
				voutl(0xfd000000 + dwaKernelVideoLookupTable[n], dwaKernelVideoLookupTable[n+(nMode * 0x1a)]);
				n++;
			}
		}

		{
			int n=0;
			while(n<sizeof(baSequencerInit)) {
				vgaout(0xfd0C03C4, n, baSequencerInit[n]);
				n++;
			}
		}

		{
			int n=0;
			while(n<sizeof(baGraInit)) {
				vgaout(0xfd0C03CE, n, baGraInit[n]);
				n++;
			}
		}


		for(i=0;i<0x22;i++) {  // issue more register setup
			BYTE bReg=baRegisterLoading[i];
			BYTE b=baRegisterLoading[(0x22 * nVar14)+i];
			switch(bReg) {
				case 0x13:
					// .text:800310B0		       mov     cl, byte	ptr [ebp+arg_10]
					b=(BYTE)nArg10/8;
					break;
				case 0x19:
					b|=(nArg10/64)&0xe0;
					break;
				case 0x25:
					b|=(nArg10/512)&0x20;
					break;
			}
			vgaout(0xfd0C03C4, bReg, b);
		}

		I2CTransmitWordDelay( 0x10, 0x0b01, false);  //PIC register 0b <- 01??

			vgaout(0xfd6013c0, 1, 1);  // kernel: video on

			// experimental
		I2CTransmitWord(0x45, 0xa010, false);
		I2CTransmitWord(0x45, 0x0412, false);
		I2CTransmitWord(0x45, 0xd600, false);
		I2CTransmitWord(0x45, 0x9c00, false);
		I2CTransmitWord(0x45, 0x9e00, false);
		I2CTransmitWord(0x45, 0xba20, false);

/*
		i=I2CTransmitByteGetReturnDelay(0x45, 0xc8);
//		bprintf(" vidrx(Enc status normally 9b)=%02X\n", i);

		I2CTransmitWordDelay(0x45, 0xc880, false);  // select STATUS readback
		I2CTransmitWordDelay(0x45, 0x3480, false);
		i=I2CTransmitByteGetReturnDelay(0x45, 0x96);
		bprintf(" vidrx(Enc status normally 06)=%02X\n", i);
		I2CTransmitWordDelay(0x45, 0x9606, false);

		I2CTransmitWordDelay( 0x10, 0x0b00, false);  //PIC
		I2CTransmitWordDelay( 0x10, 0x0d04, false);  //PIC
		I2CTransmitWordDelay( 0x10, 0x1a01, false);  //PIC

*/
/*
	for(i=0;i<(sizeof(waVideoEncoder)/sizeof(WORD));i++) {  // issue Encoder setup table
		I2CTransmitWordDelay(0x45, waVideoEncoder[i], false);
	}
*/

	// Christer Palm reports 2002-07-29 that b7b6 of register 0xc8 controls readback selection - last written to 10 by tables
	// 00 = Chip ID (not detected by original code), 01 = "Monitor sense/Closed Caption status/Field ", 10=Status (this mode used exclusively)
	// he also reports that byte written out on readback request is ignored; notes correspondance to next write address: suggest register may have been
	// set up for next write before read call in original code

//	i= I2CTransmitByteGetReturn(0x45, 0x6c);
//		bprintf(" vidrx(Enc status normally 46)=%02X\n",i);
/*
		I2CTransmitWordDelay(0x45, 0x6cc6, false);
		I2CTransmitWordDelay(0x45, 0xba20, false);
		I2CTransmitWordDelay(0x45, 0x60c8, false);
		I2CTransmitWordDelay(0x45, 0x6200, false);
		I2CTransmitWordDelay(0x45, 0x6400, false);
*/
			// experimental
	/*
		I2CTransmitWordDelay(0x45, 0xc65c, false);
		I2CTransmitWordDelay(0x45, 0xa08a, false);
		I2CTransmitWordDelay(0x45, 0x6cce, false);
		I2CTransmitWordDelay(0x45, 0xcee4, false);
		I2CTransmitWordDelay(0x45, 0xd60c, false);

		I2CTransmitWordDelay(0x45, 0xba20, false);
*/

//		I2CTransmitWordDelay( 0x10, 0x0b00, false);

//		i=I2CTransmitByteGetReturnDelay(0x10, 0x11);
//		bprintf(" vidrx(PIC ads 11 normally 04)=%02X\n", i&0xff);  // been seeing 04, 00 here
//		I2CTransmitWordDelay( 0x10, 0x0d00 | (i&0xff), false);

//		BootVideoDelay();

//		I2CTransmitWordDelay(0x45, 0x9606, false);

//		I2CTransmitWordDelay(0x45, 0xc405, false);


//		I2CTransmitWord(0x45, 0xba04, false);


		/*

	for(i=0;i<(sizeof(waVideoEncoder2)/sizeof(WORD));i++) {
		I2CTransmitWord(0x45, waVideoEncoder2[i], false);
	}

		i= I2CTransmitByteGetReturn(0x45, 0x6c);
	//	bprintf(" vidrx(Enc ads 6c normally 46)=%02X\n",i);
		I2CTransmitWord(0x45, 0x6cc6, false);
		I2CTransmitWord(0x45, 0xba24, false);
		I2CTransmitWord(0x45, 0x60c8, false);
		I2CTransmitWord(0x45, 0x6200, false);
		I2CTransmitWord(0x45, 0x6400, false);

		i= I2CTransmitByteGetReturn(0x45, 0x06);
//		bprintf(" vidrx(Enc ads 06 normally 00)=%02X\n", i);
*/

	voutl(0xfd600800, 0);

	{
		int n;
		BYTE ba[256];
		for(n=0;n<256;n+=2) {
			ba[n]=I2CTransmitByteGetReturn(0x45, n);
			ba[n+1]=ba[n];
		}
		DumpAddressAndData(0, &ba[0], 256);
	}

/*

00000000: 62 62 01 01 52 52 04 04 : 04 04 04 04 04 04 04 04    bb..RR..........
00000010: C0 C0 00 00 80 80 9D 9D : 00 00 00 00 00 00 00 00    ................
00000020: 00 00 00 00 00 00 00 00 : 00 00 28 28 30 30 00 00    ..........((00..
00000030: 00 00 28 28 80 80 80 80 : 00 00 00 00 80 80 FF FF    ..((............
00000040: 80 80 13 13 DA DA 4B 4B : 28 28 A3 A3 9F 9F 25 25    ......KK((....%%
00000050: A3 A3 9F 9F 25 25 00 00 : 00 00 01 01 15 15 44 44    ....%%........DD
00000060: 00 00 00 00 4B 4B 00 00 : 00 00 10 10 46 46 00 00    ....KK......FF..
00000070: 02 02 00 00 01 01 88 88 : 72 72 85 85 44 44 ED ED    ........rr..DD..
00000080: 13 13 F2 F2 26 26 00 00 : 08 08 73 73 03 03 0D 0D    ....&&....ss....
00000090: 24 24 E0 E0 06 06 00 00 : 10 10 68 68 DA DA 0A 0A    $$........hh....
000000A0: 08 08 E4 E4 7C 7C DC DC : 9A 9A A7 A7 12 12 99 99    ....||..........
000000B0: 86 86 25 25 5A 5A 19 19 : 00 00 3F 3F 00 00 00 00    ..%%ZZ....??....
000000C0: 00 00 00 00 01 01 9C 9C : 00 00 00 00 00 00 0F 0F    ................
000000D0: 00 00 00 00 00 00 A4 A4 : 40 40 FC FC 20 20 D0 D0    ........@@..  ..
000000E0: 6F 6F 0F 0F 00 00 00 00 : 0C 0C F3 F3 09 09 BD BD    oo..............
000000F0: 66 66 B5 B5 90 90 B2 B2 : 7D 7D 00 00 00 00 00 00    ff......}}......

>   using "./nvtv -t -r 640,576 -s Normal -b" (displays Colorbars)
>
>   Device 0:8A = Conexant CX25871 Rev 2 (0:8A)
>     00: 62 62 00 00 16 16 00 00   00 00 00 00 00 00 00 C0
>     10: C0 C0 00 00 80 80 9D 9D   00 00 00 00 00 00 00 00
>     20: 00 00 00 00 00 00 00 00   00 00 28 28 30 30 00 00
>     30: 00 00 28 28 00 00 00 00   00 00 00 00 80 80 80 80
>     40: 6E 6E 00 00 00 00 00 00   00 00 00 00 00 00 00 00
>     50: 0F 0F 00 00 A5 A5 20 20   00 00 00 00 00 00 44 44
>     60: 00 00 00 00 00 00 0B 0B   59 59 00 00 46 46 00 00
>     70: 02 02 00 00 01 01 00 00   20 20 AA AA CA CA 9A 9A
>     80: 0D 0D 29 29 FC FC 39 39   00 00 C0 C0 8C 8C 03 03
>     90: EE EE 5F 5F 58 58 3A 3A   66 66 96 96 00 00 00 00
>     A0: 10 10 24 24 F0 F0 57 57   80 80 48 48 8C 8C 18 18
>     B0: 28 28 87 87 1F 1F 00 00   03 03 00 00 00 00 00 00
>     C0: 00 00 00 00 05 05 9C 9C   13 13 C0 C0 C0 C0 18 18
>     D0: 00 00 78 78 00 00 00 00   40 40 05 05 57 57 20 20
>     E0: 40 40 6E 6E 7E 7E F4 F4   51 51 0F 0F F1 F1 05 05
>     F0: 05 05 78 78 A2 A2 25 25   54 54 A5 A5 00 00 00 00
*/
	bprintf("  vid: inited\n");

#if 1

		{  // spam some visible junk on to the video buffer... one day we will see this
			int n=0;
			BYTE * pb=(BYTE *)0xf0000000;
			for( n=0;n<10000;n++) { pb[n]=0xaa; }
		}
#endif

}


