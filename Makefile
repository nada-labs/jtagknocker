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

.PHONY: all clean jtagknocker test docs upload

all: jtagknocker

#Get OpenCM3 setup correctly
SRCLIBDIR=libopencm3
include libopencm3/ld/Makefile.linker
ROM_BASE := $(shell echo $(CFLAGS) | grep -Eo "ROM_OFF=0x[0-9A-Fa-f]{8}" | sed -e 's/ROM_OFF=//')

SOURCE_OBJS := $(addprefix build/$(TARGET)/, $(patsubst %c,%o,$(shell find source -name '*.c')))
SOURCE_CFLAGS := -c -Ilibopencm3/include -O2 -ffunction-sections -D$(PLATFORM)=1
SOURCE_LDFLAGS := -Llibopencm3/lib -T$(LDSCRIPT) -gc-sections -nostartfiles

TEST_OBJS := $(addprefix build/$(TARGET)/, $(patsubst %c,%o,$(shell find test -name '*.c')))
TEST_CFLAGS := -c -Ilibopencm3/include -O2 -ffunction-sections -D$(PLATFORM)=1
TEST_LDFLAGS := -Llibopencm3/lib -T$(LDSCRIPT) -gc-sections -nostartfiles

jtagknocker: build/$(TARGET)/jtagknocker.bin

test: build/$(TARGET)/test.bin

-include $(SOURCE_OBJS:.o=.d)

build/$(TARGET)/jtagknocker.elf: $(SOURCE_OBJS) $(LDSCRIPT)
	@echo "      LD $@"
	@$(CC) -o $@ $(CFLAGS) $(SOURCE_LDFLAGS) $(SOURCE_OBJS) $(LDFLAGS)

build/$(TARGET)/source/%.o: source/%.c
	@echo "      CC $@"
	@mkdir -p $(dir $@)
	@$(CC) $(SOURCE_CFLAGS) $(CFLAGS) -c -MMD -MP -o $@ $<

clean:
	@rm -rf build

build/$(TARGET)/test.elf: $(TEST_OBJS) $(LDSCRIPT)
	@echo "      LD $@"
	@$(CC) -o $@ $(CFLAGS) $(TEST_LDFLAGS) $(TEST_OBJS) $(LDFLAGS)

build/$(TARGET)/test/%.o: test/%.c
	@echo "      CC $@"
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) $(CFLAGS) -c -MMD -MP -o $@ $<

%.bin: %.elf
	@echo " OBJCOPY $@"
	@$(OBJCOPY) -O binary -j .text $< $@

docs:
	@echo "    DOCS"
	@cd doc && doxygen Doxyfile

# Assumes st-flash is installed.
upload: build/$(TARGET)/jtagknocker.bin
	@echo "  UPLOAD $<"
	@st-flash write $< $(ROM_BASE)

test-upload: build/$(TARGET)/test.bin
	@echo "  UPLOAD $<"
	@st-flash write $< $(ROM_BASE)

