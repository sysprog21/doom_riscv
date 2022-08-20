CROSS ?= riscv-none-embed-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size

# RISC-V specific options
CFLAGS=-Wall -O2 -march=rv32im -mabi=ilp32 -ffreestanding -flto -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I..

# resolution
CFLAGS += -DSCREENWIDTH=640 -DSCREENHEIGHT=360

include ../sources.mk

# Filter out d_main, we provide our own simplified one
SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

# Filter out s_sound, we provide a dummy one
SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))


SOURCES_doom_arch := \
	d_main.c \
	i_main.c \
	i_net.c \
	i_sound.c \
	i_system.c \
	i_video.c \
	s_sound.c \
	console.c


all: doom-riscv.elf

doom-riscv.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -Wl,-Bstatic,--strip-debug -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h


%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

.PHONY: all clean
.PRECIOUS: *.elf
