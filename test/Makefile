include $(TOP_DIR)/makefile.rules

.PHONY: all clean
BINARY := test-$(BINARY)

all: $(BINARY)

SRCLIBDIR = $(TOOLCHAIN_DIR)
include $(TOOLCHAIN_DIR)/ld/Makefile.linker

CFLAGS += -DTEST

OBJS = test.o tjtag.o tjtagtap.o tchain.o tmessage.o

$(BINARY): $(OBJS) $(LDSCRIPT)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) 
clean:
	@rm -rf *.o $(BINARY)
