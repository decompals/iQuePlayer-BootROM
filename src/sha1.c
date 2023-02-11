#include "PR/ultratypes.h"
#include "sha1.h"

void SHA1Transform(SHA1Context* ctx);
void SHA1PaddedTransform(SHA1Context* ctx);

s32 SHA1Reset(SHA1Context* ctx) {
    if (ctx == NULL) {
        return SHA1_ERR_NULL;
    }
    ctx->count_lo = 0;
    ctx->count_hi = 0;
    ctx->block_size = 0;
    ctx->digest[0] = 0x67452301;
    ctx->digest[1] = 0xEFCDAB89;
    ctx->digest[2] = 0x98BADCFE;
    ctx->digest[3] = 0x10325476;
    ctx->digest[4] = 0xC3D2E1F0;
    ctx->done = FALSE;
    ctx->err = SHA1_ERR_OK;
    return SHA1_ERR_OK;
}

s32 SHA1Result(SHA1Context* ctx, u8* hash) {
    s32 i;

    if (ctx == NULL) {
        return SHA1_ERR_NULL;
    }
    if (hash == NULL) {
        return SHA1_ERR_NULL;
    }
    if (ctx->err != SHA1_ERR_OK) {
        return ctx->err;
    }

    if (!ctx->done) {
        SHA1PaddedTransform(ctx);

        for (i = 0; i < SHA1_BLOCK_MAX_SIZE; i++) {
            ctx->data[i] = 0;
        }
        ctx->count_lo = 0;
        ctx->count_hi = 0;
        ctx->done = TRUE;
    }

    // write out final hash
    for (i = 0; i < (s32)sizeof(ctx->digest); i++) {
        hash[i] = ctx->digest[i >> 2] >> ((3 - (i & 3)) * 8);
    }
    return SHA1_ERR_OK;
}

s32 SHA1Input(SHA1Context* ctx, u8* buffer, s32 size) {
    if (size == 0) {
        return SHA1_ERR_OK;
    }
    if (ctx == NULL || buffer == NULL) {
        return SHA1_ERR_NULL;
    }
    if (ctx->done) {
        ctx->err = SHA1_ERR_DONE;
        return SHA1_ERR_DONE;
    }
    if (ctx->err != SHA1_ERR_OK) {
        return ctx->err;
    }
    size--;

    while (size != -1 && ctx->err == SHA1_ERR_OK) {
        ctx->data[ctx->block_size++] = *buffer;
        ctx->count_lo += 8;
        if (ctx->count_lo == 0) {
            ctx->count_hi++;
            if (ctx->count_hi == 0) {
                ctx->err = SHA1_ERR_NULL;
            }
        }

        size--;
        if (ctx->block_size == SHA1_BLOCK_MAX_SIZE) {
            // Process block
            SHA1Transform(ctx);
        }
        buffer++;
    }
    return SHA1_ERR_OK;
}

#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

void SHA1Transform(SHA1Context* ctx) {
    u32 D0prev;
    u32 D0;
    u32 D1;
    s32 D2;
    s32 D3;
    s32 D4;
    u32 temp;
    s32 i;

    const u32 sha1_consts[4] = {
        0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6,
    };
    u32 buf[0x50];

    for (i = 0; i < SHA1_BLOCK_MAX_SIZE / 4; i++) {
        buf[i]  = ctx->data[i * 4 + 0] << 0x18;
        buf[i] |= ctx->data[i * 4 + 1] << 0x10;
        buf[i] |= ctx->data[i * 4 + 2] << 0x08;
        buf[i] |= ctx->data[i * 4 + 3] << 0x00;
    }

    for (i = 0x10; i < 0x50; i++) {
        temp = buf[0x0D + i - 0x10] ^ buf[0x08 + i - 0x10] ^ buf[0x02 + i - 0x10] ^ buf[0x00 + i - 0x10];
        buf[i] = ROL(temp, 1);
    }

    D1 = ctx->digest[1];
    D2 = ctx->digest[2];
    D3 = ctx->digest[3];
    D4 = ctx->digest[4];
    D0prev = ctx->digest[0];

    for (i = 0; i < 0x14; i++) {
        D0 = ROL(D0prev, 5) + ((D1 & D2) | (~D1 & D3)) + D4 + buf[i] + sha1_consts[0];
        D4 = D3;
        D3 = D2;
        D2 = ROL(D1, 30);
        D1 = D0prev;
        D0prev = D0;
    }

    for (i = 0x14; i < 0x28; i++) {
        D0 = ROL(D0, 5) + (D1 ^ D2 ^ D3) + D4 + buf[i] + sha1_consts[1];
        D4 = D3;
        D3 = D2;
        D2 = ROL(D1, 30);
        D1 = D0prev;
        D0prev = D0;
    }

    for (i = 0x28; i < 0x3C; i++) {
        D0 = ROL(D0, 5) + ((D1 & (D2 | D3)) | (D2 & D3)) + D4 + buf[i] + sha1_consts[2];
        D4 = D3;
        D3 = D2;
        D2 = ROL(D1, 30);
        D1 = D0prev;
        D0prev = D0;
    }

    for (i = 0x3C; i < 0x50; i++) {
        D0 = ROL(D0, 5) + (D1 ^ D2 ^ D3) + D4 + buf[i] + sha1_consts[3];
        D4 = D3;
        D3 = D2;
        D2 = ROL(D1, 30);
        D1 = D0prev;
        D0prev = D0;
    }

    ctx->digest[0] += D0;
    ctx->digest[1] += D1;
    ctx->digest[2] += D2;
    ctx->digest[3] += D3;
    ctx->digest[4] += D4;
    ctx->block_size = 0;
}

void SHA1PaddedTransform(SHA1Context* ctx) {
    if ((s32)ctx->block_size >= SHA1_BLOCK_MAX_SIZE - 8) {
        // not enough space for 8 extra bytes, pad to size and process this block
        ctx->data[ctx->block_size++] = 0x80;

        // pad to full size
        while ((s32)ctx->block_size < SHA1_BLOCK_MAX_SIZE) {
            ctx->data[ctx->block_size++] = 0;
        }

        // process block
        SHA1Transform(ctx);

        // pad to full size, leaving room for 8 bytes
        while ((s32)ctx->block_size < SHA1_BLOCK_MAX_SIZE - 8) {
            ctx->data[ctx->block_size++] = 0;
        }
    } else {
        // there is enough space for 8 extra bytes
        ctx->data[ctx->block_size++] = 0x80;

        // pad to full size, leaving room for 8 bytes
        while ((s32)ctx->block_size < SHA1_BLOCK_MAX_SIZE - 8) {
            ctx->data[ctx->block_size++] = 0;
        }
    }

    ctx->data[56] = ctx->count_hi >> 0x18;
    ctx->data[57] = ctx->count_hi >> 0x10;
    ctx->data[58] = ctx->count_hi >> 0x08;
    ctx->data[59] = ctx->count_hi & 0xFF;

    ctx->data[60] = ctx->count_lo >> 0x18;
    ctx->data[61] = ctx->count_lo >> 0x10;
    ctx->data[62] = ctx->count_lo >> 0x08;
    ctx->data[63] = ctx->count_lo & 0xFF;

    SHA1Transform(ctx);
}
