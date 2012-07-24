/*-
 * Copyright (c) 2008 FUKAUMI Naoki.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 ●付の似非DESブロック暗号データ復号処理をcで書いてみた ver 0.9.1
*/

typedef unsigned char uchar;
typedef unsigned int uint;

static void packetDecrypt ( const uchar* src, uchar* dst, int nPacket );

static const uint table[][64] = {
  {
    0x01010400, 0x00000000, 0x00010000, 0x01010404, 0x01010004, 0x00010404, 0x00000004, 0x00010000,
    0x00000400, 0x01010400, 0x01010404, 0x00000400, 0x01000404, 0x01010004, 0x01000000, 0x00000004,
    0x00000404, 0x01000400, 0x01000400, 0x00010400, 0x00010400, 0x01010000, 0x01010000, 0x01000404,
    0x00010004, 0x01000004, 0x01000004, 0x00010004, 0x00000000, 0x00000404, 0x00010404, 0x01000000,
    0x00010000, 0x01010404, 0x00000004, 0x01010000, 0x01010400, 0x01000000, 0x01000000, 0x00000400,
    0x01010004, 0x00010000, 0x00010400, 0x01000004, 0x00000400, 0x00000004, 0x01000404, 0x00010404,
    0x01010404, 0x00010004, 0x01010000, 0x01000404, 0x01000004, 0x00000404, 0x00010404, 0x01010400,
    0x00000404, 0x01000400, 0x01000400, 0x00000000, 0x00010004, 0x00010400, 0x00000000, 0x01010004
  },
  {
    0x80108020, 0x80008000, 0x00008000, 0x00108020, 0x00100000, 0x00000020, 0x80100020, 0x80008020,
    0x80000020, 0x80108020, 0x80108000, 0x80000000, 0x80008000, 0x00100000, 0x00000020, 0x80100020,
    0x00108000, 0x00100020, 0x80008020, 0x00000000, 0x80000000, 0x00008000, 0x00108020, 0x80100000,
    0x00100020, 0x80000020, 0x00000000, 0x00108000, 0x00008020, 0x80108000, 0x80100000, 0x00008020,
    0x00000000, 0x00108020, 0x80100020, 0x00100000, 0x80008020, 0x80100000, 0x80108000, 0x00008000,
    0x80100000, 0x80008000, 0x00000020, 0x80108020, 0x00108020, 0x00000020, 0x00008000, 0x80000000,
    0x00008020, 0x80108000, 0x00100000, 0x80000020, 0x00100020, 0x80008020, 0x80000020, 0x00100020,
    0x00108000, 0x00000000, 0x80008000, 0x00008020, 0x80000000, 0x80100020, 0x80108020, 0x00108000
  },
  {
    0x00000208, 0x08020200, 0x00000000, 0x08020008, 0x08000200, 0x00000000, 0x00020208, 0x08000200,
    0x00020008, 0x08000008, 0x08000008, 0x00020000, 0x08020208, 0x00020008, 0x08020000, 0x00000208,
    0x08000000, 0x00000008, 0x08020200, 0x00000200, 0x00020200, 0x08020000, 0x08020008, 0x00020208,
    0x08000208, 0x00020200, 0x00020000, 0x08000208, 0x00000008, 0x08020208, 0x00000200, 0x08000000,
    0x08020200, 0x08000000, 0x00020008, 0x00000208, 0x00020000, 0x08020200, 0x08000200, 0x00000000,
    0x00000200, 0x00020008, 0x08020208, 0x08000200, 0x08000008, 0x00000200, 0x00000000, 0x08020008,
    0x08000208, 0x00020000, 0x08000000, 0x08020208, 0x00000008, 0x00020208, 0x00020200, 0x08000008,
    0x08020000, 0x08000208, 0x00000208, 0x08020000, 0x00020208, 0x00000008, 0x08020008, 0x00020200
  },
  {
    0x00802001, 0x00002081, 0x00002081, 0x00000080, 0x00802080, 0x00800081, 0x00800001, 0x00002001,
    0x00000000, 0x00802000, 0x00802000, 0x00802081, 0x00000081, 0x00000000, 0x00800080, 0x00800001,
    0x00000001, 0x00002000, 0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002001, 0x00002080,
    0x00800081, 0x00000001, 0x00002080, 0x00800080, 0x00002000, 0x00802080, 0x00802081, 0x00000081,
    0x00800080, 0x00800001, 0x00802000, 0x00802081, 0x00000081, 0x00000000, 0x00000000, 0x00802000,
    0x00002080, 0x00800080, 0x00800081, 0x00000001, 0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802081, 0x00000081, 0x00000001, 0x00002000, 0x00800001, 0x00002001, 0x00802080, 0x00800081,
    0x00002001, 0x00002080, 0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002000, 0x00802080
  },
  {
    0x00000100, 0x02080100, 0x02080000, 0x42000100, 0x00080000, 0x00000100, 0x40000000, 0x02080000,
    0x40080100, 0x00080000, 0x02000100, 0x40080100, 0x42000100, 0x42080000, 0x00080100, 0x40000000,
    0x02000000, 0x40080000, 0x40080000, 0x00000000, 0x40000100, 0x42080100, 0x42080100, 0x02000100,
    0x42080000, 0x40000100, 0x00000000, 0x42000000, 0x02080100, 0x02000000, 0x42000000, 0x00080100,
    0x00080000, 0x42000100, 0x00000100, 0x02000000, 0x40000000, 0x02080000, 0x42000100, 0x40080100,
    0x02000100, 0x40000000, 0x42080000, 0x02080100, 0x40080100, 0x00000100, 0x02000000, 0x42080000,
    0x42080100, 0x00080100, 0x42000000, 0x42080100, 0x02080000, 0x00000000, 0x40080000, 0x42000000,
    0x00080100, 0x02000100, 0x40000100, 0x00080000, 0x00000000, 0x40080000, 0x02080100, 0x40000100
  },
  {
    0x20000010, 0x20400000, 0x00004000, 0x20404010, 0x20400000, 0x00000010, 0x20404010, 0x00400000,
    0x20004000, 0x00404010, 0x00400000, 0x20000010, 0x00400010, 0x20004000, 0x20000000, 0x00004010,
    0x00000000, 0x00400010, 0x20004010, 0x00004000, 0x00404000, 0x20004010, 0x00000010, 0x20400010,
    0x20400010, 0x00000000, 0x00404010, 0x20404000, 0x00004010, 0x00404000, 0x20404000, 0x20000000,
    0x20004000, 0x00000010, 0x20400010, 0x00404000, 0x20404010, 0x00400000, 0x00004010, 0x20000010,
    0x00400000, 0x20004000, 0x20000000, 0x00004010, 0x20000010, 0x20404010, 0x00404000, 0x20400000,
    0x00404010, 0x20404000, 0x00000000, 0x20400010, 0x00000010, 0x00004000, 0x20400000, 0x00404010,
    0x00004000, 0x00400010, 0x20004010, 0x00000000, 0x20404000, 0x20000000, 0x00400010, 0x20004010
  },
  {
    0x00200000, 0x04200002, 0x04000802, 0x00000000, 0x00000800, 0x04000802, 0x00200802, 0x04200800,
    0x04200802, 0x00200000, 0x00000000, 0x04000002, 0x00000002, 0x04000000, 0x04200002, 0x00000802,
    0x04000800, 0x00200802, 0x00200002, 0x04000800, 0x04000002, 0x04200000, 0x04200800, 0x00200002,
    0x04200000, 0x00000800, 0x00000802, 0x04200802, 0x00200800, 0x00000002, 0x04000000, 0x00200800,
    0x04000000, 0x00200800, 0x00200000, 0x04000802, 0x04000802, 0x04200002, 0x04200002, 0x00000002,
    0x00200002, 0x04000000, 0x04000800, 0x00200000, 0x04200800, 0x00000802, 0x00200802, 0x04200800,
    0x00000802, 0x04000002, 0x04200802, 0x04200000, 0x00200800, 0x00000000, 0x00000002, 0x04200802,
    0x00000000, 0x00200802, 0x04200000, 0x00000800, 0x04000002, 0x04000800, 0x00000800, 0x00200002
  },
  {
    0x10001040, 0x00001000, 0x00040000, 0x10041040, 0x10000000, 0x10001040, 0x00000040, 0x10000000,
    0x00040040, 0x10040000, 0x10041040, 0x00041000, 0x10041000, 0x00041040, 0x00001000, 0x00000040,
    0x10040000, 0x10000040, 0x10001000, 0x00001040, 0x00041000, 0x00040040, 0x10040040, 0x10041000,
    0x00001040, 0x00000000, 0x00000000, 0x10040040, 0x10000040, 0x10001000, 0x00041040, 0x00040000,
    0x00041040, 0x00040000, 0x10041000, 0x00001000, 0x00000040, 0x10040040, 0x00001000, 0x00041040,
    0x10001000, 0x00000040, 0x10000040, 0x10040000, 0x10040040, 0x10000000, 0x00040000, 0x10001040,
    0x00000000, 0x10041040, 0x00040040, 0x10000040, 0x10040000, 0x10001000, 0x10001040, 0x00000000,
    0x10041040, 0x00041000, 0x00041000, 0x00001040, 0x00001040, 0x00040040, 0x10000000, 0x10041000
  }
};

inline static uint ror ( uint a, uint b ) { return (a >> b) | (a << (32 - b)); }
inline static uint rol ( uint a, uint b ) { return (a << b) | (a >> (32 - b)); }

inline static void stageX ( uint* a, uint* b, uint sv, uint mask ) {
  uint tmp = ((*a >> sv) ^ *b) & mask; *a ^= tmp << sv; *b ^= tmp;
}

inline static void stage0 ( uint* l, uint* r ) { stageX( l, r,  4, 0x0f0f0f0f ); }
inline static void stage1 ( uint* l, uint* r ) { stageX( l, r, 16, 0x0000ffff ); }
inline static void stage2 ( uint* l, uint* r ) { stageX( r, l,  2, 0x33333333 ); }
inline static void stage3 ( uint* l, uint* r ) { stageX( r, l,  8, 0x00ff00ff ); }

inline static void stage4 ( uint* l, uint* r ) {
  uint tmp = (*l ^ *r) & 0xaaaaaaaa; *l ^= tmp; *r ^= tmp;
}

inline static uint _big2 ( const uchar* cp ) {
  return (cp[0] << 24) | (cp[1] << 16) | (cp[2] << 8) | cp[3];
}

inline static void _2big ( uchar* cp, uint v ) {
  cp[0] = (uchar)(v >> 24);
  cp[1] = (uchar)(v >> 16);
  cp[2] = (uchar)(v >> 8);
  cp[3] = (uchar)v;
}

static void
blockDecrypt (
  uint* key, const uchar* src, uchar* dst, int nBlock )
{
  while ( nBlock-- > 0 ) {
    uint v0 = _big2( src );
    uint v1 = _big2( src+4 );

    stage0( &v0, &v1 );
    stage1( &v0, &v1 );
    stage2( &v0, &v1 );
    stage3( &v0, &v1 );
    v1 = rol( v1, 1 );
    stage4( &v0, &v1 );
    v0 = rol( v0, 1 );
    {
      uint* kp = key;
      int round = 8;
      do { // key 4 * 8 * 4 = 128byte
        uint ind;
        ind = v1 ^ *kp++;
        v0 ^= table[5][(ind >>  8) & 0x3f] ^ table[3][(ind >> 16) & 0x3f] ^
              table[1][(ind >> 24) & 0x3f] ^ table[7][ind & 0x3f];
        ind = ror( v1, 4 ) ^ *kp++;
        v0 ^= table[4][(ind >>  8) & 0x3f] ^ table[2][(ind >> 16) & 0x3f] ^
              table[0][(ind >> 24) & 0x3f] ^ table[6][ind & 0x3f];
        ind = v0 ^ *kp++;
        v1 ^= table[5][(ind >>  8) & 0x3f] ^ table[3][(ind >> 16) & 0x3f] ^
              table[1][(ind >> 24) & 0x3f] ^ table[7][ind & 0x3f];
        ind = ror( v0, 4 ) ^ *kp++;
        v1 ^= table[4][(ind >>  8) & 0x3f] ^ table[2][(ind >> 16) & 0x3f] ^
              table[0][(ind >> 24) & 0x3f] ^ table[6][ind & 0x3f];
      } while ( --round > 0 );
    }
    v1 = ror( v1, 1 );
    stage4( &v1, &v0 );
    v0 = ror( v0, 1 );
    stage3( &v1, &v0 );
    stage2( &v1, &v0 );
    stage1( &v1, &v0 );
    stage0( &v1, &v0 );

    _2big( dst, v1 );
    _2big( dst+4, v0 );
    src += 8;
    dst += 8;
  }
}

/*
 ASIE5606レジスタ10-1F設定 All 0 の場合 
*/

static uint key4allZero[][32] = {
  {
    0x02121903, 0x3D131236, 0x0C0C270A, 0x20393131,
    0x032C082B, 0x3E011F00, 0x212A0702, 0x063A1C1F,
    0x3A042A0C, 0x270A2B0C, 0x303D2203, 0x31223415,
    0x10313C01, 0x07140D0A, 0x261D0617, 0x0C06260F,
    0x33183332, 0x011D0402, 0x0719151D, 0x27280C2C,
    0x20302238, 0x2A2F362A, 0x352A1125, 0x29003038,
    0x083F0D38, 0x0B092406, 0x2916350C, 0x3204122D,
    0x2C242439, 0x1D183310, 0x07360B38, 0x102C0B0D
  },
  {
    0x04290000, 0x0E151B28, 0x30151501, 0x11080025,
    0x00192038, 0x29140302, 0x06320107, 0x0E041030,
    0x2C020828, 0x08230802, 0x19061304, 0x19011005,
    0x091B2018, 0x10180100, 0x0A001003, 0x361C1424,
    0x09272602, 0x01002001, 0x081C1001, 0x140C0B00,
    0x04040612, 0x36120412, 0x32200804, 0x0C130A28,
    0x13030220, 0x05222411, 0x18151900, 0x002E0108,
    0x1409041A, 0x30042401, 0x39102830, 0x03220000
  }
};

static uchar xorValue[] = { 0x00, 0x00, 0xB1, 0xF2, 0x00, 0x04, 0xF2, 0x04 };

static void
packetDecrypt ( const uchar* src, uchar* dst, int nPacket )
{
  while ( nPacket-- > 0 ) {
    int i;
    for ( i = 0; i < 4; i++ )
      *dst++ = *src++;
    for ( i = 0; i < 0xB8; i++ )
      dst[i] = src[i] ^ xorValue[i & 0x07];
    blockDecrypt( key4allZero[0], dst     , dst     , 0x10 );
    blockDecrypt( key4allZero[1], dst+0x80, dst+0x80, 0x07 );
    src += 0xB8;
    dst += 0xB8;
  }
}

#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if 0
int
main(int argc, char *argv[])
{
    char* buffer = NULL;
    size_t syssize;

	union {
		uint32_t w[47];
		uint8_t b[188];
	} buf;
	uint32_t *xor, *np;
	uint8_t *header;
	size_t offset;
	int i, null = 0, xorfd;

	if (argc > 2) {
		fprintf(stderr, "usage: %s [file] < in.ts > out.ts\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc == 2) {
		if ((xorfd = open(argv[1], O_RDONLY)) == -1)
			err(EXIT_FAILURE, argv[1]);
	} else {
		if ((xorfd = open("SKNET_HDTV_BDA.sys", O_RDONLY)) == -1)
			err(EXIT_FAILURE, "SKNET_HDTV_BDA.sys");
	}

        syssize = 0x12da8 + 184;
        buffer = (char*)malloc( syssize );
        if( read( xorfd, buffer, syssize ) != syssize ){
            fprintf( stderr, "could not read SKNET_HDTV_BDA.sys\n");
            goto err;
        }

        header = buffer;
	if (header[0] == 0x47 && header[1] == 0x1f && header[2] == 0xff) {
		null = 1; /* XOR'ed null packet */
		offset = 4;
	} else if (header[0x2c8] == 0x8d && header[0x2c9] == 0x5f)
		offset = 0xf628; /* CD 1.0 */
	else if (header[0x2c0] == 0x31 && header[0x2c1] == 0xe6)
		offset = 0x12da8; /* CD 1.1 */
	else {
		fprintf(stderr, "unknown file\n");
		exit(EXIT_FAILURE);
	}

        fprintf( stderr, "offset = 0x%x\n", offset );

	if (null == 1) {
		if ((xor = malloc(184)) == NULL)
			err(EXIT_FAILURE, "malloc()");
                np = (uint32_t *)( buffer + offset );

		for (i = 0; i < 46; i++) {
			xor[i] = np[i] ^ 0xffffffff;
		}

		close(xorfd);
	} else {
            xor = (uint32_t *)( buffer + offset );
	}

	/* buf.b == xxor */
	memset(buf.b, 0, 188);

 sync:
	while (read(STDIN_FILENO, buf.b, 1) == 1) {
		if (buf.b[0] == 0x47) /* XXX: may not be sync byte */
			if (read(STDIN_FILENO, &buf.b[1], 187) == 187)
				break;
		fprintf(stderr, ".");
	}

	if (buf.b[0] != 0x47)
		goto err;

	for (;;) {
		uchar outbuffer[188];
		packetDecrypt(buf.b, outbuffer, 1);
		write(STDOUT_FILENO, outbuffer, 188);
		if (read(STDIN_FILENO, buf.w, 188) != 188)
			break;
		if (buf.b[0] != 0x47)
			goto sync;
	}

 err:
	if (null == 1) {
		free(xor);
	} else {
		close(xorfd);
	}

        if( buffer ) free( buffer );
	return EXIT_SUCCESS;
}
#endif
