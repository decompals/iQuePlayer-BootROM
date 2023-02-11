#include "PR/ultratypes.h"
#include "PR/R4300.h"
#include "PR/bcp.h"
#include "sha1.h"

#define ARRLEN(x) ((s32)(sizeof(x) / sizeof(x[0])))

s32 CardReadBlock(s32 block_num, s32 dev) {
    IO_WRITE(PI_70_REG, block_num << 9); // set card block to read

    // send card command
    if (dev != 0) {
        // read to buffer + 0x200
        IO_WRITE(PI_48_REG, 0x80000000 | (0x1F << 24) | (0x00 << 16) | 0x8000 | 0x4000 | 0x800 | 0x210);
    } else {
        // read to buffer + 0x000
        IO_WRITE(PI_48_REG, 0x80000000 | (0x1F << 24) | (0x00 << 16) | 0x8000 |      0 | 0x800 | 0x210);
    }

    // wait while busy
    do {
        if (IO_READ(MI_38_REG) & 0x02000000) {
            // If interrupt pending, deassert it and return -2
            IO_WRITE(PI_48_REG, 0);
            return -2;
        }
    } while (IO_READ(PI_48_REG) & 0x80000000);

    // Check error?
    if (IO_READ(PI_48_REG) & 0x400) {
        return -1;
    }
    return 0;
}

s32 func_9FC40324(s32 arg0) {
    s32 ret;
    s32 var_a0;
    u32 var;
    s32 i;

    while (TRUE) {
        ret = CardReadBlock(arg0 << 5, 0);
        if (ret == -2) {
            return -2;
        }

        var = IO_READ(PI_10404_REG);

        var_a0 = 0;
        for (i = 0; i < 8; i++) {
            if (!((var >> (16 + i)) & 1)) {
                var_a0++;
                RDB_WRITE_16(0x9A);
            }
        }

        arg0++;
        if (var_a0 < 2) {
            break;
        }
    }
    if (ret == -1) {
        RDB_WRITE_16(0x90);
        return -1;
    }
    return arg0 - 1;
}

void AES_Run(s32 arg0, s32 continuation) {
    s32 cmd = PI_AES_CMD; // issue AES command
    cmd |= (arg0 << 14); // buffer select (a la Card control reg)? or encrypt/decrypt flag?
    if (continuation) {
        cmd |= 1; // chain IV?
    } else {
        cmd |= ((0x4D0/0x10) << 1); // IV PI buffer offset?
    }
    cmd |= (0x200/0x10-1) << 16; // data size? (0x200 bytes specified in 16-byte chunks)
    IO_WRITE(PI_AES_CTRL_REG, cmd);
}

#define PANIC() \
    __asm__ ("la $8, %0; jr $8" : : "i"(0xBFC00200) : "$8")

void LoadSKAndEnter(void) {
    // AES-128-CBC expanded key
    static const u32 sk_expanded_key[] = {
        0x7604543B, 0x46F4165E, 0x1865EA0E, 0xE96422D0,
        0x53652015, 0xE6734133, 0x33345733, 0x794E9746,
        0x66FB71EC, 0xB5166126, 0xD5471600, 0x4A7AC075,
        0x07515AC1, 0xD3ED10CA, 0x60517726, 0x9F3DD675,
        0xD20DE309, 0xD4BC4A0B, 0xB3BC67EC, 0xFF6CA153,
        0xD605848A, 0x06B1A902, 0x67002DE7, 0x4CD0C6BF,
        0xF4152062, 0xD0B42D88, 0x61B184E5, 0x2BD0EB58,
        0xAC3B3AED, 0x24A10DEA, 0xB105A96D, 0x4A616FBD,
        0x97FB955C, 0x889A3707, 0x95A4A487, 0xFB64C6D0,
        0x36281B15, 0x1F61A25B, 0x1D3E9380, 0x6EC06257,
        0xA8190276, 0x7E25DB17, 0x0F3449C5, 0xD94B162F,
    };
    // AES-128-CBC initialisation vector
    static const u32 sk_iv[4] = {
        0xA438B341, 0x0298747B, 0x0C089D8F, 0x6D2991A8,
    };
    SHA1Context sha1_ctx;
    BbShaHash hash;
    s32 var_s1;
    s32 remaining_blocks;
    s32 var_s6 = 0;
    u32* dst;
    s32 i;
    s32 j;

    RDB_WRITE_16(0x30);

    // Prepare AES hardware
    for (i = 0; i < ARRLEN(sk_expanded_key); i++) {
        IO_WRITE(PI_AES_EXPANDED_KEY_BUF(i), sk_expanded_key[i]);
    }
    IO_WRITE(PI_AES_IV_BUF(0), sk_iv[0]);
    IO_WRITE(PI_AES_IV_BUF(1), sk_iv[1]);
    IO_WRITE(PI_AES_IV_BUF(2), sk_iv[2]);
    IO_WRITE(PI_AES_IV_BUF(3), sk_iv[3]);

    RDB_SHORT_WR(PI_FFFEA_REG, 0);
    RDB_SHORT_RD(PI_20000_REG);

    RDB_WRITE_16(0x32);

    // Panic if certain interrupts are enabled? MD and ?
    if ((IO_READ(MI_38_REG) & 0x2000) || (IO_READ(MI_38_REG) & 0x02000000)) {
        RDB_WRITE_16(0x34);
        RDB_WRITE_16(0xBAAD);
        PANIC();
    }

    // Read and decrypt SK from card
    for (var_s1 = 0, remaining_blocks = 0x80, dst = (u32*)PHYS_TO_K1(0x1FC20000); remaining_blocks > 0; var_s1++) {
        var_s1 = func_9FC40324(var_s1);

        RDB_WRITE_16(0x40);
        RDB_WRITE_16(var_s1);

        var_s6++;

        if (var_s1 < 0) {
            RDB_WRITE_16(0x42);
            if (var_s1 == -2) {
                RDB_WRITE_16(0x43);
            }
            RDB_WRITE_16(0xBAAD);
            PANIC();
        }

        // Run AES decryption and read more blocks?
        for (j = 0; j < 0x20; j++) {
            if (j == 0 && var_s6 == 1) {
                // first run
                AES_Run(0, FALSE);
            } else if (j != 0) {
                // continuation and block needs reading
                s32 ret = CardReadBlock((var_s1 << 5) + j, 0);

                // if bad return, panic
                if (ret < 0) {
                    RDB_WRITE_16(0x46);
                    if (ret == -2) {
                        RDB_WRITE_16(0x47);
                    }
                    RDB_WRITE_16(0xBAAD);
                    PANIC();
                }
                AES_Run(0, TRUE);
            } else {
                // continuation and first block read
                AES_Run(0, TRUE);
            }

            // Wait while AES is busy
            while (IO_READ(PI_AES_STATUS_REG) & PI_AES_BUSY)
                ;

            RDB_WRITE_16(0x50);

            // Copy decrypted data to destination
            for (i = 0; i < 0x200; i += 4) {
                *(dst++) = IO_READ(PI_10000_BUF(i));
            }

            remaining_blocks--;
            if (remaining_blocks == 0) {
                break;
            }
        }
    }

    // Compute SHA1 hash
    SHA1Reset(&sha1_ctx);
    SHA1Input(&sha1_ctx, (u8*)PHYS_TO_K0(0x1FC20000), 0x10000);
    SHA1Result(&sha1_ctx, (u8*)hash);

    RDB_WRITE_16(0x60);

    // Verify
    if ((hash[0] != IO_READ(VIRAGE2_SK_HASH(0))) ||
        (hash[1] != IO_READ(VIRAGE2_SK_HASH(1))) ||
        (hash[2] != IO_READ(VIRAGE2_SK_HASH(2))) ||
        (hash[3] != IO_READ(VIRAGE2_SK_HASH(3))) ||
        (hash[4] != IO_READ(VIRAGE2_SK_HASH(4)))) {

        RDB_WRITE_16(0xBAAD);
        PANIC();
    }

    RDB_WRITE_16(0x64);

    // ?
    IO_WRITE(MI_14_REG, (IO_READ(MI_14_REG) | 1) & ~2);

    // Enter SK
    __asm__ ("nop; j %0\n" : : "i"(0x9FC00000));
}
