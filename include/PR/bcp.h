#ifndef BCP_H
#define BCP_H

#include "rcp.h"

/**
 * Additional MIPS Interface (MI) Registers
 */

/**
 * [11] reboot?
 * [10:8] ? bootrom sets this based on part of the box id
 */
#define MI_10_REG   (MI_BASE_REG + 0x10)

/**
 * [3] SK exception cause: Timer
 * [2] SK exception cause: SKC (attempted read from MI_14_REG while not in secure mode)
 * [1] Swap Boot ROM and SK RAM
 * [0] Secure Mode bit
 */
#define MI_14_REG   (MI_BASE_REG + 0x14)

/**
 * [25] MD (current state, 1 if card is currently disconnected else 0)
 * [24] Power button (current state, 1 if button is currently pressed else 0)
 * [13] MD (pending interrupt)
 * [12] Power button (pending interrupt)
 * [11] usb1
 * [10] usb0
 * [9] pi_err
 * [8] ide
 * [7] aes
 * [6] flash
 * [5] dp
 * [4] pi
 * [3] vi
 * [2] ai
 * [1] si
 * [0] sp
 */
#define MI_38_REG   (MI_BASE_REG + 0x38)

#define MI_EX_INTR_SP               (1 << 0)
#define MI_EX_INTR_SI               (1 << 1)
#define MI_EX_INTR_AI               (1 << 2)
#define MI_EX_INTR_VI               (1 << 3)
#define MI_EX_INTR_PI               (1 << 4)
#define MI_EX_INTR_DP               (1 << 5)
#define MI_EX_INTR_FLASH            (1 << 6)
#define MI_EX_INTR_AES              (1 << 7)
#define MI_EX_INTR_IDE              (1 << 8)
#define MI_EX_INTR_PI_ERR           (1 << 9)
#define MI_EX_INTR_USB0             (1 << 10)
#define MI_EX_INTR_USB1             (1 << 11)
#define MI_EX_INTR_PWR_BTN          (1 << 12)
#define MI_EX_INTR_MD               (1 << 13)
#define MI_EX_INTR_PWR_BTN_PRESSED  (1 << 24) // Reflects real-time state rather than pending interrupt
#define MI_EX_INTR_CARD_NOT_PRESENT (1 << 25) // Reflects real-time state rather than pending interrupt



/**
 * Additional Peripheral Interface (PI) Registers
 */

/**
 * NAND command/status
 *
 * Writing 0 clears flash interrupt?
 *
 * [31] Execute command after write
 * [30] Raise flash interrupt when done
 * [29:24] ?
 * [23:16] NAND command
 * [15] ?
 * [14] PI Buffer offset, 0=0x000 1=0x200
 * [13:12] Device select
 * [11] Perform ECC
 * [10] NAND command is multicycle
 * [9:0] Transfer length for read/write data commands
 */
#define PI_48_REG           (PI_BASE_REG + 0x48)

/**
 * AES Control/Status
 *
 * Writing 0 clears AES interrupt?
 *
 * Write:
 * [31] Execute after write
 * [?:16] Data size to decrypt (-1, in multiples of 16 bytes) ?
 * [14] PI Buffer select or encrypt/decrypt flag?
 * [?:1] IV PI Buffer offset (-1, in multiples of 16 bytes) ?
 * [0] Chain IV from end of last command rather than fetching from PI Buffer
 *
 * Read:
 * [31] Busy
 */
#define PI_AES_CTRL_REG     (PI_BASE_REG + 0x50) // for writing
#define PI_AES_STATUS_REG   (PI_BASE_REG + 0x50) // for reading

// Write

#define PI_AES_CMD      (1 << 31)
#define PI_AES_CONTINUE (1 << 0)

// Read

#define PI_AES_BUSY     (1 << 31)

/**
 *  [31:16]   Box ID
 *    [31:30] ??? (osInitialize checks this and sets __osBbIsBb to 2 if != 0)
 *    [29:27] ??? (unused so far)
 *    [26:25] ??? (system clock speed identifier? used in sk as a divisor for delay times)
 *    [24:22] ??? (bootrom, checked against MI_10_REG and copied there if they mismatch)
 *
 *  [7:6] rtc mask
 *  [5]   error led mask
 *  [4]   power control mask
 *
 *  [3:2] rtc
 *  [1]   error led (1=on 0=off)
 *  [0]   power control (1=on 0=off)
 */
#define PI_60_REG           (PI_BASE_REG + 0x60)

#define PI_64_REG           (PI_BASE_REG + 0x64)

#define PI_64_31    (1 << 31)

/**
 * Set NAND byte address for operations?
 */
#define PI_70_REG           (PI_BASE_REG + 0x70)

#define PI_10000_BUF(offset)                    (PI_BASE_REG + 0x10000 + (offset))
#define PI_NAND_DATA_BUFFER(bufSelect, offset)  PI_10000_BUF((bufSelect) * 0x200 + (offset))
#define PI_NAND_SPARE_BUFFER(bufSelect, offset) PI_10000_BUF(0x400 + (bufSelect) * 0x10 + (offset))

#define PI_AES_EXPANDED_KEY_BUF(n)  (PI_BASE_REG + 0x10420 + 4 * (n))

#define PI_AES_IV_BUF(n)    (PI_BASE_REG + 0x104D0 + 4 * (n))

#define PI_20000_REG        (PI_BASE_REG + 0x20000)

#define PI_RDB_E0000_REG    (PI_BASE_REG + 0xE0000)

#define PI_FFFEA_REG        (PI_BASE_REG + 0xFFFEA)

#define PI_FFFFA_REG        (PI_BASE_REG + 0xFFFFA)



/**
 * RDB 
 */

#ifdef _LANGUAGE_ASSEMBLY
#define RDB_SHORT_WR(addr, reg) \
    sh reg, addr

#define RDB_SHORT_RD(addr, reg) \
    lw reg, addr

#define RDB_WRITE_16(val)                       \
    li      t7, val                            ;\
    sh      t7, PHYS_TO_K1(PI_RDB_E0000_REG)   ;\
    lw      t7, PHYS_TO_K1(PI_20000_REG)

#define RDB_WRITE_REG(reg)                      \
    sh      reg, PHYS_TO_K1(PI_RDB_E0000_REG)  ;\
    srl     t7, reg, 0x10                      ;\
    sh      t7, PHYS_TO_K1(PI_RDB_E0000_REG)   ;\
    lw      t7, PHYS_TO_K1(PI_20000_REG)

#else
#define RDB_SHORT_WR(addr, val) \
    (*(volatile u16*)PHYS_TO_K1(addr) = (val))

#define RDB_SHORT_RD(addr) \
    (*(volatile u16*)PHYS_TO_K1(addr))

#define RDB_WRITE_16(val)                       \
    {                                           \
        RDB_SHORT_WR(PI_RDB_E0000_REG, (val));  \
        RDB_SHORT_RD(PI_20000_REG);             \
    }(void)0
#endif



/**
 * Virage2
 */

#define VIRAGE2_BASE_ADDR   0x1FCA0000

/**
 * [23] Set some time after boot?
 */
#define VIRAGE2_STATUS_REG  (VIRAGE2_BASE_ADDR + 0xC000)

#define VIRAGE2_STATUS_23   (1 << 23)

// Contents offsets

// 0 <= n < 5
#define VIRAGE2_SK_HASH(n)  (VIRAGE2_BASE_ADDR + (n) * 4)

// 0 <= n < 16
#define VIRAGE2_ROMPATCH(n) (VIRAGE2_BASE_ADDR + 0x14 + (n) * 4)

#endif
