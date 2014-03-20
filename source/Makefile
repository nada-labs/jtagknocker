include $(TOP_DIR)/makefile.rules

.PHONY: all clean

all: $(BINARY)

SRCLIBDIR = $(TOOLCHAIN_DIR)
include $(TOOLCHAIN_DIR)/ld/Makefile.linker

OBJS = jtag.o jtagtap.o main.o serial.o knock.o chain.o commands.o message.o

$(BINARY): $(OBJS) $(LDSCRIPT)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) 
clean:
	@rm -rf *.o $(BINARY)
