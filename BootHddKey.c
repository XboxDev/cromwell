
#include "boot.h"
#include <stdarg.h>
#include <string.h>
#include "sha1.h"
#include "rc4.h"

static inline size_t strlen(const char * s)
{
int d0;
register int __res;
__asm__ __volatile__(
	"repne\n\t"
	"scasb\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
return __res;
}


/* This is basically originally all speedbump's work

 	  2002-09-18 franz@chaos.at    Changes to use a single SMAC_SHA1_calculation() routine
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

int HMAC1Reset(SHA1Context *context);
int HMAC2Reset(SHA1Context *context);
void SHA1ProcessMessageBlock(SHA1Context *context);
void HMAC_SHA1_calculation(unsigned char *HMAC_result, ... );


void quick_SHA1( unsigned char *SHA1_result, ... )
{
	va_list args;
	struct SHA1Context context;

	va_start(args,SHA1_result);
	SHA1Reset(&context);

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

void HMAC_SHA1(
	BYTE * baResult,
	BYTE *key,
	int key_length,
	BYTE *baText1,
	int nLengthText1,
	BYTE *baText2,
	int nLengthText2
) {
	BYTE baState1[0x40];
	BYTE baState2[0x40+0x14];
	int n;

	for(n=0x40-1; n>=key_length;--n) baState1[n] = 0x36;
	for(;n>=0;--n) baState1[n] = key[n] ^ 0x36;

	quick_SHA1 ( &baState2[0x40], baState1, 0x40, baText1, nLengthText1, baText2, nLengthText2, NULL );

	for(n=0x40-1; n>=key_length;--n) baState2[n] = 0x5C;
	for(;n>=0;--n) baState2[n] = key[n] ^ 0x5C;

	quick_SHA1 ( baResult, baState2, 0x40+0x14, NULL );
}


/*
void HMAC_SHA1_calculation(unsigned char *HMAC_result, ... )
{
	va_list args;
	struct SHA1Context context;			
	unsigned char middle_result[20];
        va_start(args,HMAC_result);

	HMAC1Reset(&context);

	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;

		if (buffer == NULL) break;

		length = va_arg(args,int);

		SHA1Input(&context,buffer,length);

	}
	va_end(args);

	SHA1Result(&context,&middle_result[0]);
	HMAC2Reset(&context);
        SHA1Input(&context,&middle_result[0],20);
        SHA1Result(&context,HMAC_result);

}
*/

void HMAC_SHA1_calculation(unsigned char *HMAC_result, ... )
{
	va_list args;
	struct SHA1Context context;			
        va_start(args,HMAC_result);
        
	HMAC1Reset(&context);
	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;
		
		if (buffer == NULL) break;
		length = va_arg(args,int);
		SHA1Input(&context,buffer,length);
		
	}
	va_end(args);
	
	SHA1Result(&context,&context.Message_Block[0]);  
	HMAC2Reset(&context);
        SHA1Input(&context,&context.Message_Block[0],0x14);
        SHA1Result(&context,HMAC_result);
}

DWORD BootHddKeyGenerateEepromKeyData(
		BYTE *pbEeprom_data,
		BYTE *pbResult
) {

	BYTE baKeyHash[20];
	BYTE baDataHashConfirm[20];
	struct rc4_key RC4_key;

	HMAC_SHA1_calculation(baKeyHash,pbEeprom_data, 20, NULL);

		//initialize RC4 key
	rc4_prepare_key(baKeyHash, 20, &RC4_key);

		//decrypt data (from eeprom) with generated key
	rc4_crypt(&pbEeprom_data[20],8,&RC4_key);		//confounder of some kind?
	rc4_crypt(&pbEeprom_data[28],20,&RC4_key);		//"real" data

	HMAC_SHA1_calculation(baDataHashConfirm, &pbEeprom_data[20], 8, &pbEeprom_data[28], 20, NULL);

	{ int n; for(n=0;n<0x14;n++) if(pbEeprom_data[n]!=baDataHashConfirm[n]) return 0; }

		//copy out HDKey
	memcpy(pbResult,&pbEeprom_data[28],16);
	return 1;
}


void genHDPass(
		BYTE * pbEEPROMComputedKey,
		BYTE * pbszHDSerial,
		BYTE * pbHDModel,
		BYTE * pbHDPass
) {

	HMAC_SHA1 ( pbHDPass, pbEEPROMComputedKey, 0x10, pbHDModel, strlen(pbHDModel), pbszHDSerial, strlen(pbszHDSerial) );
}

