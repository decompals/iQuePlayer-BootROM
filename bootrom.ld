OUTPUT_ARCH(mips)
ENTRY(COLD_RESET)

MEMORY {
    bin : ORIGIN = 0, LENGTH = 0x2000
    rom (rx) : ORIGIN = 0xBFC00000, LENGTH = 0x2000
    sram_uc (rwx) : ORIGIN = 0xBFC40000, LENGTH = 0x8000
    sram_c (rwx) : ORIGIN = 0x9FC40000, LENGTH = 0x8000
}

SECTIONS {

	.text.startup.0 :
	{
		PROVIDE(COLD_RESET = .);
		build/src/boot1.o(.text*)
	} > rom AT> bin

    STAGE2_ROM_START = .;

    .text.startup.1 :
    {
        build/src/boot2.o(.text*)
    } > sram_uc AT> bin

    /* Pick up in the cached segment where the prior uncached code ended */

    .text (. & 0x1FFFFFFF | 0x80000000) :
    {
        build/src/boot3.o(.text*)
        build/src/boot4.o(.text*)
        build/src/sha1.o(.text*)
    } > sram_c AT> bin

    .data :
    {
        build/src/boot4.o(.data*)
        build/src/sha1.o(.data*)
    } > sram_c AT> bin

    .rodata :
    {
        build/src/boot4.o(.rodata*)
        build/src/sha1.o(.rodata*)
    } > sram_c AT> bin

    /* Back to uncached for end symbol */
    STAGE2_END = (. & 0x1FFFFFFF | 0xA0000000);

    _gp = .;

	/DISCARD/ :
	{
		*(*);
	}
}
