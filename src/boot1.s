#include "asm.h"
#include "regdef.h"
#include "bcp.h"

.text

LEAF(COLD_RESET) // 0xBFC00000
    // reset watchpoint
    mtc0    zero, C0_WATCHLO
    mtc0    zero, C0_WATCHHI

    // wait for bit set in V2 status
    la      t0, PHYS_TO_K1(VIRAGE2_STATUS_REG)
1:
    lw      t1, (t0)
    and     t2, t1, 0x00800000
    beqz    t2, 1b

    // compute a checksum of Virage2 contents
    la      t0, PHYS_TO_K1(VIRAGE2_BASE_ADDR)
    la      t1, PHYS_TO_K1(VIRAGE2_BASE_ADDR + 0x100)-4
    move    t4, zero
2:
    lw      t3, (t0)
    addu    t4, t4, t3
.set noreorder
    bltu    t0, t1, 2b
     addiu  t0, t0, 4
.set reorder

    // if the sum of the contents of Virage2 mod 2^32 is BBC0DE, step over
    beq     t4, 0xBBC0DE, 3f

    RDB_WRITE_16(0xBAAD)

    // jump to 0x200
    la      t0, PHYS_TO_K1(0x1FC00000 + 0x200)
    jr      t0

3:
    // run some rompatch code
    la      t0, return // set return value
    nop
    j       PHYS_TO_K1(VIRAGE2_ROMPATCH(2))
return:
    // ?
    la      t0, PHYS_TO_K1(PI_64_REG)
    lw      t1, (t0)
    and     t1, t1, ~0x80000000
    sw      t1, (t0)

    // debugging
    RDB_WRITE_16(1)

    // Copy 0xC90 bytes from 0xBFC00400 to 0xBFC40000
    la      t0, PHYS_TO_K1(0x1FC40000)
    la      t2, STAGE2_ROMSTART
    la      t1, STAGE2_END
4:
    lw      t3, (t2)
    sw      t3, (t0)
    addiu   t0, t0, 4
.set noreorder
    bltu    t0, t1, 4b
     addiu  t2, t2, 4
.set reorder

    // run more rompatch code
    la      t0, LABEL_BFC40000 // set return value
    nop
    j       PHYS_TO_K1(VIRAGE2_ROMPATCH(0))
END(COLD_RESET)



.fill 0x200 - (. - COLD_RESET)

// some sort of panic?
    RDB_WRITE_16(0xEC01)
    la      t0, PHYS_TO_K1(PI_64_REG)
    lw      t1, (t0)
    and     t1, t1, ~0x80000000
    sw      t1, (t0)
    sh      zero, PHYS_TO_K1(PI_FFFFA_REG)
    lw      t0, PHYS_TO_K1(PI_20000_REG)
1:
    bgez    zero, 1b



.fill 0x280 - (. - COLD_RESET)

    RDB_WRITE_16(0xEC01)
    la      t0, PHYS_TO_K1(PI_64_REG)
    lw      t1, (t0)
    and     t1, t1, ~0x80000000
    sw      t1, (t0)
    sh      zero, PHYS_TO_K1(PI_FFFFA_REG)
    lw      t0, PHYS_TO_K1(PI_20000_REG)
1:
    bgez    zero, 1b



.fill 0x380 - (. - COLD_RESET)

    RDB_WRITE_16(0xEC01)
    la      t0, PHYS_TO_K1(PI_64_REG)
    lw      t1, (t0)
    and     t1, t1, ~0x80000000
    sw      t1, (t0)
    sh      zero, PHYS_TO_K1(PI_FFFFA_REG)
    lw      t0, PHYS_TO_K1(PI_20000_REG)
1:
    bgez    zero, 1b



.fill 0x400 - (. - COLD_RESET)
