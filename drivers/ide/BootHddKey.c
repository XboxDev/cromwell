
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


void HMAC_hdd_calculation(int version,unsigned char *HMAC_result, ... );

extern size_t strlen(const char * s);


int copy_swap_trim(unsigned char *dst, unsigned char *src, int len)
{
	unsigned char tmp;
	int i;
        for (i=0; i < len; i+=2) {
		tmp = src[i];     //allow swap in place
		dst[i] = src[i+1];
		dst[i+1] = tmp;
	}

	--dst;
	for (i=len; i>0; --i) {
		if (dst[i] != ' ') {
			dst[i+1] = 0;
			break;
		}
	}
	return i;
}

int HMAC1hddReset(int version,SHA1Context *context)
{
  SHA1Reset(context);
  if (version==10) {
		context->Intermediate_Hash[0] = 0x72127625;
		context->Intermediate_Hash[1] = 0x336472B9;
		context->Intermediate_Hash[2] = 0xBE609BEA;
		context->Intermediate_Hash[3] = 0xF55E226B;
		context->Intermediate_Hash[4] = 0x99958DAC;
	}
	if (version==11) {
		context->Intermediate_Hash[0] = 0x39B06E79;
		context->Intermediate_Hash[1] = 0xC9BD25E8;
		context->Intermediate_Hash[2] = 0xDBC6B498;
		context->Intermediate_Hash[3] = 0x40B4389D;
		context->Intermediate_Hash[4] = 0x86BBD7ED;
	}

	context->Length_Low = 512;

	return shaSuccess;
}

int HMAC2hddReset(int version,SHA1Context *context)
{
	SHA1Reset(context);
	if (version==10) {
		context->Intermediate_Hash[0] = 0x76441D41;
		context->Intermediate_Hash[1] = 0x4DE82659;
		context->Intermediate_Hash[2] = 0x2E8EF85E;
		context->Intermediate_Hash[3] = 0xB256FACA;
		context->Intermediate_Hash[4] = 0xC4FE2DE8;
	}
	if (version==11) {
		context->Intermediate_Hash[0] = 0x9B49BED3;
		context->Intermediate_Hash[1] = 0x84B430FC;
		context->Intermediate_Hash[2] = 0x6B8749CD;
		context->Intermediate_Hash[3] = 0xEBFE5FE5;
		context->Intermediate_Hash[4] = 0xD96E7393;
	}
	context->Length_Low  = 512;
	return shaSuccess;

}



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

	for(i=0x40-1; i>=key_length;--i) state2[i] = 0x5C;
	for(;i>=0;--i) state2[i] = key[i] ^ 0x5C;

	SHA1Reset(&context);
	SHA1Input(&context,state2,0x40+0x14);
	SHA1Result(&context,result);

}


void HMAC_hdd_calculation(int version,unsigned char *HMAC_result, ... )
{
	va_list args;
	struct SHA1Context context;

	va_start(args,HMAC_result);

	HMAC1hddReset(version, &context);
	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;

		if (buffer == NULL) break;
		length = va_arg(args,int);
		SHA1Input(&context,buffer,length);

	}
	va_end(args);

	SHA1Result(&context,&context.Message_Block[0]);
	HMAC2hddReset(version, &context);
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
    memset(&RC4_key,0,sizeof(rc4_key));
		memcpy(&baEepromDataLocalCopy[0], pbEeprom_data, 0x30);
                
                // Calculate the Key-Hash
		HMAC_hdd_calculation(counter, baKeyHash, &baEepromDataLocalCopy[0], 20, NULL);

		//initialize RC4 key
		rc4_prepare_key(baKeyHash, 20, &RC4_key);

        	//decrypt data (from eeprom) with generated key
		rc4_crypt(&baEepromDataLocalCopy[20],8,&RC4_key);		//confounder of some kind?
		rc4_crypt(&baEepromDataLocalCopy[28],20,&RC4_key);		//"real" data
                
                // Calculate the Confirm-Hash
		HMAC_hdd_calculation(counter, baDataHashConfirm, &baEepromDataLocalCopy[20], 8, &baEepromDataLocalCopy[28], 20, NULL);

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


