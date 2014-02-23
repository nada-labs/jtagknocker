.PHONY: all clean

all: $(BINARY)

SRCLIBDIR = $(TOOLCHAIN_DIR)
include $(TOOLCHAIN_DIR)/ld/Makefile.linker

OBJS = jtag.o main.o 

$(BINARY): $(OBJS) $(LDSCRIPT)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) 
clean:
	@rm -rf *.o $(BINARY)
