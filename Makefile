BUILDDIR = build

INCLUDES += -Iinclude -I

LIB = lib/launchpad_pro.a

OUTPUT_PREFIX = $(BUILDDIR)/issho_launchpad_

# tools
TOOLS = tools
HEXTOSYX = $(BUILDDIR)/hextosyx
SIMULATOR = $(BUILDDIR)/simulator
HOST_GPP = g++
HOST_GCC = gcc
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS  = -Os -Wall -I.\
-D_STM32F103RBT6_  -D_STM3x_  -D_STM32x_ -mthumb -mcpu=cortex-m3 \
-fsigned-char  -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=6000000UL \
-DCMSIS -DUSE_GLOBAL_CONFIG -ffunction-sections -std=c99  -mlittle-endian \
$(INCLUDES) -o

LDSCRIPT = stm32_flash.ld

LDFLAGS += -T$(LDSCRIPT) -u _start -u _Minimum_Stack_Size  -mcpu=cortex-m3 -mthumb -specs=nano.specs -specs=nosys.specs -nostdlib -Wl,-static -N -nostartfiles -Wl,--gc-sections

.SECONDARY: $(wildcard *.elf) $(wildcard *.hex)


all: flow poke

flow: $(OUTPUT_PREFIX)flow.syx

poke: $(OUTPUT_PREFIX)poke.syx



# build the tool for conversion of ELF files to sysex, ready for upload to the unit
$(HEXTOSYX):
	$(HOST_GPP) -Ofast -std=c++0x -I./$(TOOLS)/libintelhex/include ./$(TOOLS)/libintelhex/src/intelhex.cc $(TOOLS)/hextosyx.cpp -o $(HEXTOSYX)

# simulate a source
$(BUILDDIR)/%.simulator: src/%.c
	$(HOST_GCC) -g3 -O0 -std=c99 -Iinclude $(TOOLS)/simulator.c $< -o $@
	$@

# build a .syx from a .hex
$(OUTPUT_PREFIX)%.syx: $(OUTPUT_PREFIX)%.hex $(HEXTOSYX) $(BUILDDIR)/%.simulator	
	./$(HEXTOSYX) $< $@

# build a .hex from a .elf
%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

# build a .elf file from a .o
$(OUTPUT_PREFIX)%.elf: $(BUILDDIR)/src/%.o
	$(LD) $(LDFLAGS) -o $@ $< $(LIB)

# build a .o from a .c
$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -MMD -o $@ $<



clean:
	rm -rf $(BUILDDIR)
