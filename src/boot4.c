#include "PR/ultratypes.h"
#include "PR/R4300.h"
#include "PR/bcp.h"
#include "PR/bbnand.h"
#include "sha1.h"

#define ARRLEN(x) ((s32)(sizeof(x) / sizeof(x[0])))

s32 Card_ReadPage(s32 pageNum, s32 bufferSelect) {
    // Set page as target
    IO_WRITE(PI_70_REG, NAND_PAGE_TO_ADDR(pageNum));

    // Send read command
    if (bufferSelect != 0) {
        // Read to buffer + 0x200
        IO_WRITE(PI_48_REG, NAND_READ_0(NAND_BYTES_PER_PAGE + NAND_PAGE_SPARE_SIZE, 1, 0, TRUE, FALSE));
    } else {
        // Read to buffer + 0x000
        IO_WRITE(PI_48_REG, NAND_READ_0(NAND_BYTES_PER_PAGE + NAND_PAGE_SPARE_SIZE, 0, 0, TRUE, FALSE));
    }

    // Wait while busy
    do {
        if (IO_READ(MI_38_REG) & MI_EX_INTR_CARD_NOT_PRESENT) {
            // If interrupt pending, deassert it and return -2
            IO_WRITE(PI_48_REG, 0);
            return -2;
        }
    } while (IO_READ(PI_48_REG) & NAND_STATUS_BUSY);

    // Check error?
    if (IO_READ(PI_48_REG) & NAND_STATUS_ERROR) {
        return -1;
    }
    return 0;
}

s32 Card_SkipBadBlocks(s32 blockNum) {
    s32 ret;
    s32 numBadPages;
    u32 spare;
    s32 i;

    while (TRUE) {
        // Read first page for this block into PI buffer
        ret = Card_ReadPage(NAND_BLOCK_TO_PAGE(blockNum), 0);
        if (ret == -2) {
            return -2;
        }

        // Read spare data for the page containing the bad block info (second word bits [23:16])
        spare = IO_READ(PI_NAND_SPARE_BUFFER(0, 4));

        numBadPages = 0;
        for (i = 0; i < 8; i++) {
            if (!((spare >> (16 + i)) & 1)) {
                // If a bit is unset it's bad
                numBadPages++;
                RDB_WRITE_16(0x9A);
            }
        }

        blockNum++;
        if (numBadPages < 2) {
            // 0 or 1 bad is acceptable, break out (TODO why is 1 bit missing from bad block indicator acceptable? ECC?)
            break;
        }
    }
    if (ret == -1) {
        RDB_WRITE_16(0x90);
        return -1;
    }
    return blockNum - 1;
}

void AES_Run(s32 arg0, s32 continuation) {
    s32 cmd = PI_AES_CMD; // Issue AES command
    cmd |= (arg0 << 14); // Buffer select (a la Card control reg)? or encrypt/decrypt flag?
    if (continuation) {
        cmd |= PI_AES_CONTINUE;
    } else {
        cmd |= ((0x4D0/0x10) << 1); // IV PI buffer offset?
    }
    cmd |= (NAND_BYTES_PER_PAGE/0x10-1) << 16; // Data size? (0x200 bytes specified in 16-byte chunks)
    IO_WRITE(PI_AES_CTRL_REG, cmd);
}

#define PANIC() \
    __asm__ ("la $8, %0; jr $8" : : "i"(PHYS_TO_K1(0x1FC00200)) : "$8")

void LoadSKAndEnter(void) {
    // AES-128-CBC expanded key
    static const u32 SK_EXPANDED_KEY[] = {
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
    static const u32 SK_IV[4] = {
        0xA438B341, 0x0298747B, 0x0C089D8F, 0x6D2991A8,
    };
    SHA1Context sha1Ctx;
    BbShaHash hash;
    s32 blockNum;
    s32 nPagesLeft;
    s32 blockCounter = 0;
    u32* dst;
    s32 i;
    s32 j;

    RDB_WRITE_16(0x30);

    // Prepare AES hardware
    for (i = 0; i < ARRLEN(SK_EXPANDED_KEY); i++) {
        IO_WRITE(PI_AES_EXPANDED_KEY_BUF(i), SK_EXPANDED_KEY[i]);
    }
    IO_WRITE(PI_AES_IV_BUF(0), SK_IV[0]);
    IO_WRITE(PI_AES_IV_BUF(1), SK_IV[1]);
    IO_WRITE(PI_AES_IV_BUF(2), SK_IV[2]);
    IO_WRITE(PI_AES_IV_BUF(3), SK_IV[3]);

    RDB_SHORT_WR(PI_FFFEA_REG, 0);
    RDB_SHORT_RD(PI_20000_REG);

    RDB_WRITE_16(0x32);

    // Panic if the MD interrupt is pending or if the card is not detected
    if ((IO_READ(MI_38_REG) & MI_EX_INTR_MD) || (IO_READ(MI_38_REG) & MI_EX_INTR_CARD_NOT_PRESENT)) {
        RDB_WRITE_16(0x34);
        RDB_WRITE_16(0xBAAD);
        PANIC();
    }

    // Read and decrypt SK from the NAND, maximum 4 blocks or 0x10000 bytes (not including bad blocks)
    for (blockNum = 0, nPagesLeft = 4 * NAND_PAGES_PER_BLOCK, dst = (u32*)PHYS_TO_K1(0x1FC20000); nPagesLeft > 0; blockNum++) {
        // Skip any bad blocks
        blockNum = Card_SkipBadBlocks(blockNum);

        RDB_WRITE_16(0x40);
        RDB_WRITE_16(blockNum);

        blockCounter++;

        // If skipping bad blocks errored, panic
        if (blockNum < 0) {
            RDB_WRITE_16(0x42);
            if (blockNum == -2) {
                RDB_WRITE_16(0x43);
            }
            RDB_WRITE_16(0xBAAD);
            PANIC();
        }

        // Read and decrypt all pages for the current block
        for (j = 0; j < NAND_PAGES_PER_BLOCK; j++) {
            if (j == 0 && blockCounter == 1) {
                // Very first run, first page already in PI buffer and needs to use IV
                AES_Run(0, FALSE);
            } else if (j != 0) {
                // AES continuation and page needs reading
                s32 ret = Card_ReadPage(NAND_BLOCK_TO_PAGE(blockNum) + j, 0);

                // Panic on error
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
                // AES continuation and first page already in PI buffer from bad block skipping
                AES_Run(0, TRUE);
            }

            // Wait while AES is busy
            while (IO_READ(PI_AES_STATUS_REG) & PI_AES_BUSY) {
                ;
            }

            RDB_WRITE_16(0x50);

            // Copy decrypted data in PI Buffer to destination
            for (i = 0; i < NAND_BYTES_PER_PAGE; i += sizeof(*dst)) {
                *(dst++) = IO_READ(PI_NAND_DATA_BUFFER(0, i));
            }

            nPagesLeft--;
            if (nPagesLeft == 0) {
                break;
            }
        }
    }

    // Compute SHA1 hash
    SHA1Reset(&sha1Ctx);
    SHA1Input(&sha1Ctx, (u8*)PHYS_TO_K0(0x1FC20000), 4 * NAND_BYTES_PER_BLOCK);
    SHA1Result(&sha1Ctx, (u8*)hash);

    RDB_WRITE_16(0x60);

    // Verify against hash saved in Virage2
    if ((hash[0] != IO_READ(VIRAGE2_SK_HASH(0))) ||
        (hash[1] != IO_READ(VIRAGE2_SK_HASH(1))) ||
        (hash[2] != IO_READ(VIRAGE2_SK_HASH(2))) ||
        (hash[3] != IO_READ(VIRAGE2_SK_HASH(3))) ||
        (hash[4] != IO_READ(VIRAGE2_SK_HASH(4)))) {

        // Didn't match, panic
        RDB_WRITE_16(0xBAAD);
        PANIC();
    }

    RDB_WRITE_16(0x64);

    // Swap Boot ROM and SK RAM addresses (0x1FC00000 <-> 0x1FC20000), stay in secure mode
    IO_WRITE(MI_14_REG, (IO_READ(MI_14_REG) | 1) & ~2);

    // Enter SK that now resides at 0x1FC00000
    __asm__ ("nop; j %0\n" : : "i"(PHYS_TO_K0(0x1FC00000)));
}
