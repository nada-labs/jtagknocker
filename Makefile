# Makefile for jtagknocker
#Change DEVICE to the full device name of your ARM Cortex processor
export DEVICE = stm32f303vct6
#STM32 Launchpad

#Change PREFIX to match your toolchain, or provide it on the command line
export PREFIX ?= arm-none-eabi


export BINARY = jtagknocker
export TOP_DIR = $(shell pwd)

.PHONY: all clean source test docs

all: source

source:
	@make -C source 

clean:
	@make -C source clean
	@make -C test clean
	@make -C doc clean

test:
	@make -C test

docs:
	@make -C doc docs
