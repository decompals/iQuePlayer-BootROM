#include "asm.h"
#include "regdef.h"
#include "bcp.h"

.text

LEAF(LABEL_9FC401F0) // 0x9FC401F0
    // Enter C code

    RDB_WRITE_REG(sp)
    nop
    j       LoadSKAndEnter
END(LABEL_9FC401F0)

.balign 16 // possible file split?

    RDB_WRITE_16(0xEC01)
    la      t0, PHYS_TO_K1(PI_64_REG)
    lw      t1, (t0)
    and     t1, t1, ~0x80000000
    sw      t1, (t0)
    sh      zero, PHYS_TO_K1(PI_FFFFA_REG)
    lw      t0, PHYS_TO_K1(PI_20000_REG)
1:
    bgez    zero, 1b
