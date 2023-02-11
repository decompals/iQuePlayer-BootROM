#include "asm.h"
#include "regdef.h"
#include "bcp.h"

.text

LEAF(LABEL_BFC40000) // 0xBFC40000
    mfc0    t1, C0_CONFIG
    and     t1, ~CONFIG_K0  // Cache is used in KSEG0
    and     t1, ~CONFIG_CU  // Update on Store Conditional
    mtc0    t1, C0_CONFIG

    mtc0    zero, C0_INX
    mtc0    zero, C0_ENTRYHI
    mtc0    zero, C0_ENTRYLO0
    mtc0    zero, C0_ENTRYLO1
    mtc0    zero, C0_PAGEMASK
    mtc0    zero, C0_LLADDR
    mtc0    zero, C0_CAUSE
    mtc0    zero, C0_COUNT
    mtc0    zero, C0_COMPARE

    li      t0, SR_CU0 | SR_CU1 | SR_BEV | SR_FR
    mtc0    t0, C0_SR
    nop

    la      t0, PHYS_TO_K1(MI_10_REG)
    lw      t1, (t0)
    move    t4, t1

    RDB_WRITE_REG(t1)

    la      t2, PHYS_TO_K1(PI_60_REG)
    lw      t3, (t2)

    RDB_WRITE_REG(t3)

    and     t1, t1, 0x700
    srl     t1, t1, 8
    and     t3, t3, 0x1C00000
    srl     t3, t3, 0x16

    beq     t1, t3, 1f

    ori     t4, t4, 0x800
    sll     t3, t3, 0x8
    or      t4, t4, t3

    RDB_WRITE_16(0x20)
    RDB_WRITE_REG(t4)

    sw      t4, (t0)
    nop
1:
    RDB_WRITE_16(0x24)

    // Initialize caches

    mtc0    zero, C0_TAGLO
    mtc0    zero, C0_TAGHI

    la      t0, K0BASE
    addiu   t1, t0, ICACHE_SIZE
    addiu   t1, t1, -(8 * ICACHE_LINESIZE)
2:
    cache   (CACH_PI | C_IST), (0 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (1 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (2 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (3 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (4 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (5 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (6 * ICACHE_LINESIZE)(t0)
    cache   (CACH_PI | C_IST), (7 * ICACHE_LINESIZE)(t0)
.set noreorder
    bltu    t0, t1, 2b
     addiu  t0, t0, (8 * ICACHE_LINESIZE)
.set reorder

    RDB_WRITE_16(0x26)

    la      t0, K0BASE
    addiu   t1, t0, DCACHE_SIZE
    addiu   t1, t1, -(8 * DCACHE_LINESIZE)
3:
    cache   (CACH_PD | C_IST), (0 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (1 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (2 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (3 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (4 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (5 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (6 * DCACHE_LINESIZE)(t0)
    cache   (CACH_PD | C_IST), (7 * DCACHE_LINESIZE)(t0)
.set noreorder
    bltu    t0, t1, 3b
     addiu  t0, t0, (8 * DCACHE_LINESIZE)
.set reorder

    // set up stack and gp, jump to cached memory region

    la      sp, PHYS_TO_K0(0x1FC40000) + 0x8000 - FRAMESZ(SZREG * NARGSAVE)
    la      gp, STAGE2_END_CACHED
    la      t0, LABEL_9FC401F0
    and     t0, t0, ~(K1BASE & ~K0BASE) // unnecessary K1 -> K0 conversion?

    RDB_WRITE_REG(t0)
    nop
    jr      t0
END(LABEL_BFC40000)
