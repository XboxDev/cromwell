
#include "boot.h"
#include <stdarg.h>
#include <string.h>
#include "sha1.h"
#include "rc4.h"


/* This is basically originally all speedbump's work

		2002-10-13 franz@caos.at     Changes to work with v1.0 and v1.1 boxes
 	  2002-09-18 franz@caos.at     Changes to use a single SMAC_SHA1_calculation() routine
		2002-09-13 andy@warmcat.com  Stitched in Franz's excellent key removal work: his modest statement follows

		   Updated now, with no use for MS$ RC4 key now.
		   Please do not send emails to me and ask: how does it work ? / What happended ? / Where ist the key ? (the key has gone)
		   I only would say: It's a structural problem of the combination of the HMAC and the SHA-1.
		   This only happened, as the programmer only take "standard" algorithm, and not thinking, what happens there.
		   Ok, Studying Numerical Mathematical Analysis also helps a little.
		   Maybe, i will document how this happend one day (would take me about 4 days to type this) on the http://sha1.caos.at website
		   Beside, it's about 30% faster, as using the "standard" programm. (hehehe, it's a optimazion)
		   Sorry, for the "brutal" programming, i know, it could have done better, yes.
		   This was the working ,testing and probing programm, and usually if something is working, you are not touching it anyway.
		   I hope, it is working good, as i have no XBOX to test this.

		2002-06-27 andy@warmcat.com  Stitched it into crom, simplified out code that was for test, added munged key
*/

int HMAC1Reset(int, SHA1Context *context);
int HMAC2Reset(int, SHA1Context *context);
void SHA1ProcessMessageBlock(SHA1Context *context);
void HMAC_SHA1_calculation(int, unsigned char *HMAC_result, ... );

extern size_t strlen(const char * s);

/*
void quick_SHA1( int nVersion, unsigned char *SHA1_result, ... )
{
	va_list args;
	struct SHA1Context context;

	va_start(args,SHA1_result);
	SHA1Reset(nVersion, &context);

	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;

		if (buffer == NULL) break;

		length = va_arg(args,int);

		SHA1Input(&context,buffer,length);
	}

	SHA1Result(&context,SHA1_result);

	va_end(args);
}
  */
void HMAC_SHA1( unsigned char *result,
		unsigned char *key, int key_length, 
		unsigned char *text1, int text1_length,
		unsigned char *text2, int text2_length )
{
	unsigned char state1[0x40];
	unsigned char state2[0x40+0x14];
	int i;
	struct SHA1Context context;			
		
	for(i=0x40-1; i>=key_length;--i) state1[i] = 0x36;
	for(;i>=0;--i) state1[i] = key[i] ^ 0x36;
	
	SHA1Reset(&context);
	SHA1Input(&context,state1,0x40);
	SHA1Input(&context,text1,text1_length);
	SHA1Input(&context,text2,text2_length);
	SHA1Result(&context,&state2[0x40]);
	
/*	
	quick_SHA1 ( &state2[0x40],
			state1,		0x40,
			text1,		text1_length,
			text2,		text2_length,
			NULL );
*/	
	 
	for(i=0x40-1; i>=key_length;--i) state2[i] = 0x5C;
	for(;i>=0;--i) state2[i] = key[i] ^ 0x5C;
	
	/*
	quick_SHA1 ( result,
			state2,		0x40+0x14,
			NULL );
	*/
	SHA1Reset(&context);
	SHA1Input(&context,state2,0x40+0x14);
	SHA1Result(&context,result);
	
}   


void HMAC_SHA1_calculation(int version,unsigned char *HMAC_result, ... )
{
	va_list args;
	struct SHA1Context context;

	va_start(args,HMAC_result);

	HMAC1Reset(version, &context);
	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;

		if (buffer == NULL) break;
		length = va_arg(args,int);
		SHA1Input(&context,buffer,length);

	}
	va_end(args);

	SHA1Result(&context,&context.Message_Block[0]);
	HMAC2Reset(version, &context);
	SHA1Input(&context,&context.Message_Block[0],0x14);
	SHA1Result(&context,HMAC_result);
}


DWORD BootHddKeyGenerateEepromKeyData(
		BYTE *pbEeprom_data,
		BYTE *pbResult
		
) {

	BYTE baKeyHash[20];
	BYTE baDataHashConfirm[20];
	BYTE baEepromDataLocalCopy[0x30];
	struct rc4_key RC4_key;
       	int version = 0; 
       	int counter;
	int n,f;        
        
        // Static Version change not included yet
        
	for (counter=10;counter<12;counter++)
	{
		memcpy(&baEepromDataLocalCopy[0], pbEeprom_data, 0x30);
                
                // Calculate the Key-Hash
		HMAC_SHA1_calculation(counter, baKeyHash, &baEepromDataLocalCopy[0], 20, NULL);

		//initialize RC4 key
		rc4_prepare_key(baKeyHash, 20, &RC4_key);

        	//decrypt data (from eeprom) with generated key
		rc4_crypt(&baEepromDataLocalCopy[20],8,&RC4_key);		//confounder of some kind?
		rc4_crypt(&baEepromDataLocalCopy[28],20,&RC4_key);		//"real" data
                
                // Calculate the Confirm-Hash
		HMAC_SHA1_calculation(counter, baDataHashConfirm, &baEepromDataLocalCopy[20], 8, &baEepromDataLocalCopy[28], 20, NULL);

		f=0;
		for(n=0;n<0x14;n++) {
				if(baEepromDataLocalCopy[n]!=baDataHashConfirm[n]) f=1;
		}
		
		if (f==0) { 
			// Confirm Hash is correct  
			// Copy actual Xbox Version to Return Value
			version=counter;                           
			// exits the loop
			break;
			
		}
		
		       	
	
	}
	
	//copy out HDKey
	memcpy(pbResult,&baEepromDataLocalCopy[28],16);
	
	return version;
}


void genHDPass(
		BYTE * pbEEPROMComputedKey,
		BYTE * pbszHDSerial,
		BYTE * pbHDModel,
		BYTE * pbHDPass
) {

	HMAC_SHA1 ( pbHDPass, pbEEPROMComputedKey, 0x10, pbHDModel, strlen(pbHDModel), pbszHDSerial, strlen(pbszHDSerial) );
}

