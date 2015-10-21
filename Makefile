# Makefile for jtagknocker
#Change DEVICE to the full device name of your ARM Cortex processor and
#PLATFORM to the one appropriate for libopencm3. Defaults to the STM32F3
#Launchpad
DEVICE ?= stm32f303vct6
PLATFORM ?= STM32F3

#Change CROSS_COMPILE to match your toolchain, or provide it on the command line
CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
GDB := $(CROSS_COMPILE)gdb

TARGET := $(shell $(CC) -v 2>&1 | grep Target | cut -d " " -f 2)-$(DEVICE)

.PHONY: all clean jtagknocker test docs

all: jtagknocker

#Get OpenCM3 setup correctly
SRCLIBDIR=libopencm3
include libopencm3/ld/Makefile.linker

SOURCE_OBJS := $(addprefix build/$(TARGET)/, $(patsubst %c,%o,$(shell find source -name '*.c')))
SOURCE_CFLAGS := -c -Ilibopencm3/include -O2 -ffunction-sections -D$(PLATFORM)=1
SOURCE_LDFLAGS := -Llibopencm3/lib -T$(LDSCRIPT) -gc-sections -nostartfiles

TEST_OBJS := $(addprefix build/$(TARGET)/, $(patsubst %c,%o,$(shell find test -name '*.c')))
TEST_CFLAGS := -c -Ilibopencm3/include -O2 -ffunction-sections -D$(PLATFORM)=1
TEST_LDFLAGS := -Llibopencm3/lib -T$(LDSCRIPT) -gc-sections -nostartfiles

jtagknocker: build/$(TARGET)/jtagknocker.elf $(LDSCRIPT)

test: build/$(TARGET)/test.elf $(LDSCRIPT)

-include $(SOURCE_OBJS:.o=.d)

build/$(TARGET)/jtagknocker.elf: $(SOURCE_OBJS)
	@echo "   LD $@"
	@$(CC) -o $@ $(CFLAGS) $(SOURCE_LDFLAGS) $^ $(LDFLAGS)

build/$(TARGET)/source/%.o: source/%.c
	@echo "   CC $@"
	@mkdir -p $(dir $@)
	@$(CC) $(SOURCE_CFLAGS) $(CFLAGS) -c -MMD -MP -o $@ $<

clean:
	@rm -rf build

build/$(TARGET)/test.elf: $(TEST_OBJS)
	@echo "   LD $@"
	@$(CC) -o $@ $(CFLAGS) $(TEST_LDFLAGS) $^ $(LDFLAGS)

build/$(TARGET)/test/%.o: test/%.c
	@echo "   CC $@"
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) $(CFLAGS) -c -MMD -MP -o $@ $<

docs:
	@echo " DOCS"
	@cd doc && doxygen Doxyfile
