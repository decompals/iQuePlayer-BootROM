# iQue Player Boot ROM

This repository contains a matching decompilation of the iQue Player Boot ROM ("the bootrom"), the first code that is run by the VR4300 when the device is powered on. It builds the following binary:

`MD5 77878ab922428a2f7e9e248bacee1193  build/bootrom.bin`

To build, execute `make -C tools && make`. To skip checksum comparison with the original, execute `make COMPARE=0` in place of `make`.
To use the diff script, place a matching copy of the bootrom in the project root named `bootrom_original.bin`.

The boot process is split into multiple stages
1. Verifies the "Virage2" key store and copies the rest of the ROM to an internal SRAM.
2. Initializes Coprocessor 0 and the instruction & data caches, sets up C runtime and jumps to cached memory.
3. Trampoline to jump to C code in cached memory.
4. Reads and decrypts the iQue Player Secure Kernel (SK) off the NAND storage using the hardware AES-128-CBC engine, verifies it with SHA-1 and jumps to it.
