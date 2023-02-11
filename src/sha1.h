#ifndef __SHA1_H__
#define __SHA1_H__

#ifdef _LANGUAGE_C

#define SHA1_ERR_OK   0
#define SHA1_ERR_NULL 1
#define SHA1_ERR_DONE 3

#define SHA1_BLOCK_MAX_SIZE 64

typedef unsigned char SHA1_BYTE;
typedef unsigned long SHA1_LONG;

typedef struct {
    /* 0x00 */ SHA1_LONG digest[5];
    /* 0x14 */ SHA1_LONG count_lo;
    /* 0x18 */ SHA1_LONG count_hi;
    /* 0x1C */ SHA1_LONG block_size;
    /* 0x20 */ SHA1_BYTE data[SHA1_BLOCK_MAX_SIZE];
    /* 0x60 */ SHA1_LONG done;
    /* 0x64 */ SHA1_LONG err;
} SHA1Context; // size = 0x68

#include "PR/ultratypes.h"

typedef u32 BbShaHash[5];

s32 SHA1Reset(SHA1Context* ctx);
s32 SHA1Input(SHA1Context* ctx, u8* buffer, s32 size);
s32 SHA1Result(SHA1Context* ctx, u8* hash);

#endif

#endif
