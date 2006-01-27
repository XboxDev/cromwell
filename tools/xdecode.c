/***************************************************************************
               xdecode - Xbox bios Xcode 'decompiler'     
  			-------------------
    begin                : Sat Jan 21 2005
    copyright            : (C) 2005 by David Pye
    email                : dmp@davidmpye.dyndns.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define XCODE_START_OFFSET 0x80
#define MAX_XCODES 1500
#define XCODE_LENGTH 9

/* The normal binary representation of an xcode - 9 bytes total */
typedef struct xcode {
	unsigned char op;
	long val1;
	long val2;
} xcode;

/* We insert this into the read-xcodes if we're asked to, just before
 * the xcode_END(x) */
const char *overflow_trick = "/* Overflow Trick */\
			      \nxcode_poke(0x00000000, 0xfc1000ea);\
			      \nxcode_poke(0x00000004, 0x000008ff);";

/* Global variables */
char *prog_name;

/* Options */
int insert_overflow_trick = 0;

/* Prototypes */
int process_xcodes(const char *filename);
void show_usage();


int main(int argc, char **argv) {
	
	struct option options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "overflow-trick", no_argument, NULL, 't' },
	};	

	char c;
	int option_index = 0;

	prog_name = argv[0];
	
	while ((c = getopt_long(argc, argv, "ht", options, &option_index)) != -1) {
		switch (c) {
			case 'h':
				show_usage();
				exit(1);
			case 't':
				insert_overflow_trick = 1;
				break;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "%s: Error - no filename specified\n", prog_name);
		show_usage();
		exit(1);
	}
	process_xcodes(argv[optind]);

	exit(0);
}

int process_xcodes(const char *filename) {
	int xcode_index;
	unsigned char* xcode_segment = malloc(MAX_XCODES * XCODE_LENGTH);
	FILE *bios_image = fopen(filename,"r");

	if (bios_image == NULL) {
		fprintf(stderr, "%s: Error - unable to open file %s: %s\n", prog_name, filename, strerror(errno));
		exit(1);
	}
	
	if (fseek(bios_image, XCODE_START_OFFSET, SEEK_SET) == -1) {
		fprintf(stderr, "%s: Seek error (offset 0x%04x): %s\n", prog_name, XCODE_START_OFFSET, strerror(errno));
		exit(1);
	}

	fread(xcode_segment, 1, MAX_XCODES * XCODE_LENGTH, bios_image);	
	
	for (xcode_index = 0; xcode_index < MAX_XCODES; xcode_index++) {
		xcode code;
		code.op = xcode_segment[XCODE_LENGTH*xcode_index];
		memcpy(&code.val1, &xcode_segment[(XCODE_LENGTH*xcode_index)+sizeof(char)], sizeof(long));
		memcpy(&code.val2, &xcode_segment[(XCODE_LENGTH*xcode_index)+sizeof(char)+sizeof(long)], sizeof(long));

		switch(code.op) {
			case 0x02:
				printf("xcode_peek(0x%08lx)\n",code.val1);
				break;
			case 0x03:
				printf("xcode_poke(0x%08lx,0x%08lx)\n", code.val1, code.val2);
				break;
			case 0x04:
				printf("xcode_pciout(0x%08lx, 0x%08lx)\n", code.val1, code.val2);
				break;
			case 0x05:
				printf("xcode_pciin_a(0x%08lx)\n", code.val1);
				break;
			case 0x06:
				printf("xcode_bittoggle(0x%08lx, 0x%08lx)\n", code.val1, code.val2);
				break;
			case 0x08:
				printf("xcode_ifgoto(0x%08lx, 0x%08lx)\n", code.val1, (code.val2/9)+1);
				break;
			case 0x11:
				printf("xcode_outb(0x%08lx, 0x%08lx)\n", code.val1, code.val2);
				break;
			case 0x12:
				printf("xcode_inb(0x%08lx)\n", code.val1);
				break;
			case 0x07:
				/* These have sub-opts */
				switch(code.val1) {
					case 0x03:
						printf("xcode_poke_a(0x%08lx)\n", code.val2);
						break;
					case 0x04:
						printf("xcode_pciout_a(0x%08lx)\n", code.val2);
						break;
					case 0x11:
						printf("xcode_outb_a(0x%08lx)\n", code.val2);
						break;
					default:
						printf("Invalid 0x07 code\n");
				}
				break;
			case 0x09:
				printf("xcode_goto(%ld)\n", (code.val2/9)+1);
				break;
			case 0xEE:
				if (insert_overflow_trick) {
					printf("%s\n", overflow_trick);				
				}
				printf("xcode_END(0x%08lx)\n",code.val1);
				free(xcode_segment);
				return 0;
			default:
				/* Unknown - just output it verbatim in 'asm' type format */
				printf(".byte 0x%02x; .long 0x%08lx; .long 0x%08lx;\n",code.op, code.val1, code.val2);
		}
	}
	/* We shouln't get here - we should have left via the 0xEE xcode_END above */
	fprintf(stderr, "%s: Warning, reached end of xcode processing without encountering xcode_END marker\n",prog_name);
	free(xcode_segment);
	return 1;
}

void show_usage() {
	fprintf(stderr, "%s: Xbox bios xcode decompiler - (c) David Pye/Xbox Linux Team 2005 - GPL\n", prog_name);
	fprintf(stderr, "Usage: %s filename\nwhere filename is an Xbox bios image, from which the xcodes are to be read\n", prog_name);
	fprintf(stderr, "\nOptions:\n\n");
	fprintf(stderr, "\t-h, --help			Display this help message\n");
	fprintf(stderr, "\t-t, --insert-overflow-trick	Inserts the Xcode overflow trick into the xcodes\n");
	fprintf(stderr, "					output. (Required if they are to be used for Cromwell)\n\n");
	fprintf(stderr, "The Xcodes are read from the filename specified, processed, and written to standard output\n");
	fprintf(stderr, "in a format suitable for use with the Cromwell bios Xcode compilation system\n");
}
