# Makefile for jtagknocker
#Change DEVICE to the full device name of your ARM Cortex processor
export DEVICE = stm32f303vct6
#STM32 Launchpad

#Change PREFIX to match your toolchain, or provide it on the command line
export PREFIX ?= arm-none-eabi


export BINARY = jtagknocker
export TOP_DIR = $(shell pwd)

.PHONY: all clean source

all: source

source:
	@make -C source 

clean:
	@make -C source clean


