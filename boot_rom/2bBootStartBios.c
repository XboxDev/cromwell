/*

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-04-27 hamtitampti
	
 */

#include "2bload.h"
#include "sha1.h"


/*
Hints:

1) We are in a funny environment, executing out of ROM.  That means it is not possible to
 define filescope non-const variables with an initializer.  If you do so, the linker will oblige and you will end up with
 a 4MByte image instead of a 1MByte one, the linker has added the RAM init at 400000.
*/


// access to RTC CMOS memory



void BiosCmosWrite(BYTE bAds, BYTE bData) {
		IoOutputByte(0x72, bAds);
		IoOutputByte(0x73, bData);
}

BYTE BiosCmosRead(BYTE bAds)
{
		IoOutputByte(0x72, bAds);
		return IoInputByte(0x73);
}


extern void *MemoryChecksum;

//////////////////////////////////////////////////////////////////////
//
//  BootResetAction()

extern void BootStartBiosLoader ( void ) {

	// do not change this, this is linked to many many scipts
	unsigned int PROGRAMM_Memory_2bl 	= 0x00100000;
	unsigned int CROMWELL_Memory_pos 	= 0x03A00000;
	unsigned int CROMWELL_compress_temploc 	= 0x02000000;
	
	unsigned int Buildinflash_Flash[4]	= { 0xfff00000,0xfff40000,0xfff80000,0xfffc0000};
        unsigned int cromwellidentify	 	=  1;
        unsigned int flashbank		 	=  3;  // Default Bank
        unsigned int cromloadtry		=  0;
        	
	struct SHA1Context context;
      	unsigned char SHA1_result[20];
        
        unsigned char bootloaderChecksum[20];
        unsigned int bootloadersize;
        unsigned int loadretry;
	unsigned int compressed_image_start;
	unsigned int compressed_image_size;
	
	unsigned int de_compressed_image_size;
	
        int validimage;
        
	memcpy(&bootloaderChecksum[0],(void*)PROGRAMM_Memory_2bl,20);
	memcpy(&bootloadersize,(void*)(PROGRAMM_Memory_2bl+20),4);
	memcpy(&compressed_image_start,(void*)(PROGRAMM_Memory_2bl+24),4);
	memcpy(&compressed_image_size,(void*)(PROGRAMM_Memory_2bl+28),4);
	
      	SHA1Reset(&context);
	SHA1Input(&context,(void*)(PROGRAMM_Memory_2bl+20),bootloadersize-20);
	SHA1Result(&context,SHA1_result);
	        
        if (_memcmp(&bootloaderChecksum[0],&SHA1_result[0],20)==0) {
		// HEHE, the Image we copy'd into ram is SHA-1 hash identical, this is Optimum
		BootPerformPicChallengeResponseAction();
		                                      
	} else {
		// Bad, the checksum does not match, but we can nothing do now, we wait until PIC kills us
		while(1);
	}
	
      
      
      
	
	// Lets go, we have finished, the Most important Startup, we have now a valid Micro-loder im Ram
	// we are quite happy now
	
//	BootPciPeripheralInitialization();
      
        validimage=0;
        cromloadtry=0;
        flashbank=3;
	for (loadretry=0;loadretry<100;loadretry++) {
                
                cromloadtry++;
                
                // If 20 Try's are failing, we switch to the next bank
                if (loadretry==20) {
                	flashbank=0;
                	cromloadtry=1;
                	}

                // we switch to the next bank
                if (loadretry==40) {
                	flashbank=1;
                	cromloadtry=1;
                	}
      
                // we switch to the next bank
                if (loadretry==80) {
                	flashbank=2;
                	cromloadtry=1;
                	}
                
                
        	// Copy From Flash To RAM
      		memcpy(&bootloaderChecksum[0],(void*)(Buildinflash_Flash[flashbank]+compressed_image_start),20);
        	memcpy((void*)CROMWELL_compress_temploc,(void*)(Buildinflash_Flash[flashbank]+compressed_image_start+20),compressed_image_size);

		// Lets Look, if we have got a Valid thing from Flash        	
      		SHA1Reset(&context);
		SHA1Input(&context,(void*)(CROMWELL_compress_temploc),compressed_image_size);
		SHA1Result(&context,SHA1_result);
		
		if (_memcmp(&bootloaderChecksum[0],SHA1_result,20)==0) {
			// The Checksum is good                          
			// We start the Cromwell immediatly
			//I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
		
			validimage=1;                   


                	// Here, the Decompression Algorithm should work
                	
	                // As we have no Decompression, we set
        	        de_compressed_image_size = compressed_image_size;
			// We copy The Decompressed Image from Temp location to its final Location, basicall
			// The decompression Algorithm should decompress us there, but we have none to this time now
        		memcpy((void*)CROMWELL_Memory_pos,(void*)CROMWELL_compress_temploc,de_compressed_image_size);
			
			// Decompression Ends here
					
			// This is a config bit in Cromwell, telling the Cromwell, that it is a Cromwell and not a Xromwell
			memcpy((void*)(CROMWELL_Memory_pos+20),&cromwellidentify,4);
			memcpy((void*)(CROMWELL_Memory_pos+24),&cromloadtry,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+28),&flashbank,4);
		 	
		 	break;

		}
		
	}
	
	
        
        if (validimage==1) {

	        // We now jump to the cromwell, Good bye 2bl loader
		// This means: jmp CROMWELL_Memory_pos == 0x03A00000
		__asm __volatile__ (
		"ljmp $0x10, $0x03A00000\n"   
		);
		// We are not Longer here
	}
	
	// Bad, we did not get a valid im age to RAM, we stop and display a error
	//I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );	

	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
	
      
      
        
//	I2CTransmitWord(0x10, 0x1901); // no reset on eject

//	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray        



        while(1);


}
