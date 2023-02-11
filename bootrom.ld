OUTPUT_ARCH(mips)
ENTRY(COLD_RESET)

COLD_RESET_ADDR = 0xBFC00000;
SRAM_ADDR_UNCACHED = 0xBFC40000;
SRAM_ADDR_CACHED = 0x9FC40000;

SECTIONS {
    _Pos = 0;

	.text.startup.0 COLD_RESET_ADDR : AT(_Pos)
	{
		PROVIDE(COLD_RESET = .);
		build/src/boot1.o(.text*)
	}
    _Pos += SIZEOF(.text.startup.0);

    STAGE2_ROMSTART = ADDR(.text.startup.0) + _Pos;
    .text.startup.1 SRAM_ADDR_UNCACHED : AT(_Pos)
    {
        build/src/boot2.o(.text*)
    }
    _Pos += SIZEOF(.text.startup.1);
    .text SRAM_ADDR_CACHED + SIZEOF(.text.startup.1) : AT(_Pos)
    {
        build/src/boot3.o(.text*)
        build/src/boot4.o(.text*)
        build/src/sha1.o(.text*)
    }
    _Pos += SIZEOF(.text);
    .data : AT(_Pos)
    {
        build/src/boot4.o(.data*)
        build/src/sha1.o(.data*)
    }
    _Pos += SIZEOF(.data);
    .rodata : AT(_Pos)
    {
        build/src/boot4.o(.rodata*)
        build/src/sha1.o(.rodata*)
    }
    _Pos += SIZEOF(.rodata);
    STAGE2_END = SRAM_ADDR_UNCACHED + _Pos - SIZEOF(.text.startup.0);
    STAGE2_END_CACHED = SRAM_ADDR_CACHED + _Pos - SIZEOF(.text.startup.0);

	/DISCARD/ :
	{
		*(*);
	}
}
