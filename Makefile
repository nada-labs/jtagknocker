# Makefile for jtagknocker
export
#Change DEVICE to the full device name of your ARM Cortex processor
DEVICE = stm32f303vct6
#STM32 Launchpad

#Change PREFIX to match your toolchain, or provide it on the command line
PREFIX ?= arm-none-eabi


BINARY = jtagknocker

CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
GDB = $(PREFIX)-gdb

CFLAGS += -c -I$(TOOLCHAIN_DIR)/include
LDFLAGS += -L$(TOOLCHAIN_DIR)/lib -T$(LDSCRIPT)

TOOLCHAIN_DIR ?= $(shell pwd)/libopencm3
BUILD_DIR = 


.PHONY: all clean source

all: source

include $(TOOLCHAIN_DIR)/ld/Makefile.linker

source:
	@make -C source 

clean:
	@make -C source clean

%o:%c
	$(CC) $(CFLAGS) -o $@ $<
