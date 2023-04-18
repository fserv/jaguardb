/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 *  */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 *  */

#include "JagMD5lib.h"
static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
void MD5Init (MD5_CTX *context)
                                        /* context */
{
  context->count[0] = context->count[1] = 0;
  context->state[0] = 0x67452301;
  context->state[1] = 0xefcdab89;
  context->state[2] = 0x98badcfe;
  context->state[3] = 0x10325476;
}

/* MD5 block update operation. Continues an MD5 message-digest
  operation, processing another message block, and updating the
  context.
 */
void MD5Update (MD5_CTX *context, unsigned char *input,unsigned int inputLen  )
{
  unsigned int i, index, partLen;

  /* Compute number of bytes mod 64 */
  index = (unsigned int)((context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if ((context->count[0] += ((UINT4)inputLen << 3))
  < ((UINT4)inputLen << 3))
 context->count[1]++;
  context->count[1] += ((UINT4)inputLen >> 29);

  partLen = 64 - index;

  /* Transform as many times as possible.  */
  if (inputLen >= partLen) {
 MD5_memcpy
   ((MD5POINTER)&context->buffer[index], (MD5POINTER)input, partLen);
 MD5Transform (context->state, context->buffer);

 for (i = partLen; i + 63 < inputLen; i += 64)
   MD5Transform (context->state, &input[i]);

 index = 0;
  }
  else
 i = 0;

  /* Buffer remaining input */
  MD5_memcpy
 ((MD5POINTER)&context->buffer[index], (MD5POINTER)&input[i],
  inputLen-i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
  the message digest and zeroizing the context.
 */
void MD5Final (unsigned char digest[16], MD5_CTX *context)
{
  unsigned char bits[8];
  unsigned int index, padLen;

  /* Save number of bits */
  Encode (bits, context->count, 8);

  index = (unsigned int)((context->count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  MD5Update (context,(unsigned char*) PADDING, padLen);

  /* Append length (before padding) */
  MD5Update (context, bits, 8);
  /* Store state in digest */
  Encode (digest, context->state, 16);

  /* Zeroize sensitive information.
*/
  MD5_memset ((MD5POINTER)context, 0, sizeof (*context));
}

/* MD5 basic transformation. Transforms state based on block.  */
static void MD5Transform (UINT4 state[4], unsigned char block[64])
{
//int i=0;
 
 UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  Decode (x, block, 64);

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information.
  */
  MD5_memset ((MD5POINTER)x, 0, sizeof (x));
}

/* Encodes input (UINT4) into output (unsigned char). Assumes len is
  a multiple of 4.
 */
static void Encode (unsigned char *output, UINT4 *input, unsigned int len)
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
 output[j] = (unsigned char)(input[i] & 0xff);
 output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
 output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
 output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
  }
}

/* Decodes input (unsigned char) into output (UINT4). Assumes len is
  a multiple of 4.
 */
static void Decode (UINT4 *output,
unsigned char *input,
unsigned int len)
 
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
 output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) |
   (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24);
}

/* Note: Replace "for loop" with standard memcpy if possible.
 */

static void MD5_memcpy (MD5POINTER output, MD5POINTER input, unsigned int len)
{
  unsigned int i;

  for (i = 0; i < len; i++)
  output[i] = input[i];
}

/* Note: Replace "for loop" with standard memset if possible.
 */
static void MD5_memset (MD5POINTER output, int value, unsigned int len)
{
  unsigned int i;

  for (i = 0; i < len; i++)
 ((char *)output)[i] = (char)value;
}

/* Digests a string and print the result.  */
// Must be freeed from caller.
char* MDString (const char *string)
{
  	MD5_CTX context;
  	unsigned char digest[16];
  	//  char output1[32];
 	// static  char output[33]={"\0"};
 	char *output;
  	unsigned int len = strlen (string);
  	int i;

	output = (char*)malloc(34);
	memset( output, 0, 34 );

  if ( '\0' == *string ) return output;

  MD5Init (&context);
  MD5Update (&context, (unsigned char*)string, len);
  MD5Final (digest, &context);

  for (i = 0; i < 16; i++) {
  	/***
  	sprintf(&(output1[2*i]),"%02x",(unsigned char)digest[i]);
  	sprintf(&(output1[2*i+1]),"%02x",(unsigned char)(digest[i]<<4));
	***/
  	sprintf(&(output[2*i]),"%02x",(unsigned char)digest[i]);
  	sprintf(&(output[2*i+1]),"%02x",(unsigned char)(digest[i]<<4));
  }

  // for(i=0;i<32;i++) output[i]=output1[i];

  return output;
}

/* Digests a string and print the 32-byte result.  */
// Must be freeed from caller.
char* MDStringLen (const char *string, unsigned int len )
{

  	MD5_CTX context;
  	unsigned char digest[16];
 	char *output;
  	int i;
	output = (char*)malloc(34);
	memset( output, 0, 34 );
    if ( '\0' == *string ) return output;

    MD5Init (&context);
    MD5Update (&context, (unsigned char*)string, len);
    MD5Final (digest, &context);

    for (i = 0; i < 16; i++) {
      	sprintf(&(output[2*i]),"%02x",(unsigned char)digest[i]);
      	sprintf(&(output[2*i+1]),"%02x",(unsigned char)(digest[i]<<4));
    }

    // for(i=0;i<32;i++) output[i]=output1[i];

    return output;
}

unsigned long MD5ULong(const char *string, unsigned int strlen )
{
    if ( '\0' == *string ) return 0;

  	MD5_CTX context;
  	unsigned char digest[16];
    MD5Init (&context);
    MD5Update (&context, (unsigned char*)string, strlen);
    MD5Final (digest, &context);

    unsigned long sum = 0;
  	int i;
    for (i = 0; i < 16; i++) {
        sum = sum*16  + (unsigned char)digest[i];
    }

    return sum;
}
  
/* Digests a file and print the result.
 */
char* MDFile (char *filename)
 
{ static char output[33]={"\0"};
  FILE *file;
  MD5_CTX context;
  int len;
  unsigned char buffer[1024], digest[16];
  int i;
  char output1[32];
  if ((file = fopen (filename, "rb")) == NULL)
   { printf ("%s can't be opened\n", filename);
    return 0;
   }
  else {
   	MD5Init (&context);
 	while (len = fread (buffer, 1, 1024, file))
   	MD5Update (&context, buffer, len);
	MD5Final (digest, &context);
	fclose (file);
	for (i = 0; i < 16; i++)
	 {sprintf(&(output1[2*i]),"%02x",(unsigned char)digest[i]);
  	  sprintf(&(output1[2*i+1]),"%02x",(unsigned char)(digest[i]<<4));
  		}
        for(i=0;i<32;i++)
       output[i]=output1[i];
        return output;
       }
}

char* hmac_md5(char* text,char*  key)
{
        char   digest[16];
        char   output1[32];
         static char output[33]={"\0"};
        MD5_CTX context;
        unsigned char k_ipad[65];    /* inner padding -
                                      * key XORd with ipad
                                      */
        unsigned char k_opad[65];    /* outer padding -
                                      * key XORd with opad
                                      */
        unsigned char tk[16];
        int i;
        int text_len = strlen (text);
        int key_len=strlen(key);
        /* if key is longer than 64 bytes reset it to key=MD5(key) */
        if (key_len > 64) {

                MD5_CTX      tctx;

                MD5Init(&tctx);
                MD5Update(&tctx,(unsigned char*) key, key_len);
                MD5Final(tk, &tctx);

                key = (char*)tk;
                key_len = 16;
        }

        /*
         * the HMAC_MD5 transform looks like:
         *
         * MD5(K XOR opad, MD5(K XOR ipad, text))
         *
         * where K is an n byte key
         * ipad is the byte 0x36 repeated 64 times
         * opad is the byte 0x5c repeated 64 times
         * and text is the data being protected
         */

        /* start out by storing key in pads */
        
        /*bzero( k_ipad, sizeof k_ipad);
          bzero( k_opad, sizeof k_opad);
        */

        for(i=0;i<65;i++)
        k_ipad[i]=(unsigned char)0;
        for(i=0;i<65;i++)
        k_opad[i]=(unsigned char)0;

        /*bcopy( key, k_ipad, key_len);
          bcopy( key, k_opad, key_len);
         */
         for(i=0;i<key_len;i++)
        {k_ipad[i]=(unsigned char)key[i];
         k_opad[i]=(unsigned char)key[i];
         }

        /* XOR key with ipad and opad values */
        for (i=0; i<64; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }
        /*
         * perform inner MD5
         */
        MD5Init(&context);                   /* init context for 1st
                                              * pass */
        MD5Update(&context, k_ipad, 64);      /* start with inner pad */
        MD5Update(&context, (unsigned char*)text, text_len); /* then text of datagram */
        MD5Final((unsigned char*)digest, &context);          /* finish up 1st pass */
        /*
         * perform outer MD5
         */
        MD5Init(&context);                   /* init context for 2nd
                                              * pass */
        MD5Update(&context, k_opad, 64);     /* start with outer pad */
        MD5Update(&context,(unsigned char*) digest, 16);     /* then results of 1st
                                              * hash */
        MD5Final((unsigned char*)digest, &context);          /* finish up 2nd pass */
        for (i = 0; i < 16; i++)
        	{sprintf(&(output1[2*i]),"%02x",(unsigned char)digest[i]);
         	sprintf(&(output1[2*i+1]),"%02x",(unsigned char)(digest[i]<<4));
        }
        for(i=0;i<32;i++)
        output[i]=output1[i]; 
        return output;     
}
 
