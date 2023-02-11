TARGET := build/bootrom.bin
COMPARE ?= 1

CROSS := mips-linux-gnu-
AS := $(CROSS)as
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
STRIP := $(CROSS)strip

export COMPILER_PATH := tools/egcs

CC := $(COMPILER_PATH)/gcc
INC := -I include -I include/PR -I include/sys
CFLAGS := -nostdinc -G 0 -fno-PIC -mcpu=4300 -mips3 -mgp64 -mfp32 -D_LANGUAGE_C -O2 $(INC)
ASFLAGS := -G 0 -fno-pic -mcpu=4300 -mips3 -D_LANGUAGE_ASSEMBLY $(INC)

SRC_DIRS := $(shell find src -type d)
C_FILES  := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES  := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.s))
O_FILES  := $(foreach f,$(S_FILES:.s=.o) $(C_FILES:.c=.o),build/$f)

$(shell mkdir -p build $(foreach dir,$(SRC_DIRS),build/$(dir)))

ELF := $(TARGET:.bin=.elf)

.PHONY: all clean
all: $(TARGET)
ifeq ($(COMPARE),1)
	@md5sum $(TARGET)
	@md5sum -c bootrom.md5
endif

clean:
	$(RM) -rf build
	$(MAKE) -C tools clean

distclean: clean
	$(MAKE) -C tools distclean

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary --pad-to=0x2000 $< $@

$(ELF): $(O_FILES) bootrom.ld
	$(LD) -T bootrom.ld -Map build/bootrom.map -o $@

build/src/%.o: src/%.s
	$(CC) -c -x assembler-with-cpp $< $(ASFLAGS) -o $(@:.o=.temp.o)
	$(STRIP) -N dummy_symbol_ $(@:.o=.temp.o) -o $@

build/src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $(@:.o=.temp.o)
	$(STRIP) -N dummy_symbol_ $(@:.o=.temp.o) -o $@
