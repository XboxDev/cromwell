/* MD5.H - header file for MD5C.C
 */
/* MD5 context. */
#define UINT4 unsigned int
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int input_len);
void MD5Final(unsigned char digest[16], MD5_CTX *context);
/*

	MD5_CTX hashcontext;
	unsigned char digest[16];
	
	MD5Init(&hashcontext);
	MD5Update(&hashcontext, file, lenght);
	MD5Final(digest, &hashcontext);
*/
