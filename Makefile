# PIC32 device number
DEVICE		= 32MX340F512H

# UART settings for programmer
TTYDEV		?=/dev/ttyUSB0
TTYBAUD		?=115200

# Name of the project
PROGNAME = kernel

# Linkscript
# LINKSCRIPT	:= p$(shell echo "$(DEVICE)" | tr '[:upper:]' '[:lower:]').ld

# Resources
LINKSCRIPT := src/p32mx340f512h.ld
INCLUDEDIR := src/include
SOURCEDIR := src/kernel
BUILDDIR := build

# File transfer program
PICOCOM_FTP ?= sendelf_crc32
# PICOCOM_FTP ?= sendtar_crc32

# Compiler and linker flags
CFLAGS += -ffreestanding -march=mips32r2 -msoft-float -Wa,-msoft-float -I$(INCLUDEDIR) -Wall -Wextra -Winline
ASFLAGS += -msoft-float
LDFLAGS += -T $(LINKSCRIPT) -lc

# Filenames
ELFFILE = $(PROGNAME).elf
HEXFILE = $(PROGNAME).hex

CFILES = $(wildcard $(SOURCEDIR)/*.c)
ASFILES = $(wildcard $(SOURCEDIR)/*.S)
SYMSFILES = $(wildcard *.syms)

# Object file names
OBJFILES =  $(patsubst $(SOURCEDIR)/%.c,%.c.o,$(CFILES))
OBJFILES += $(patsubst $(SOURCEDIR)/%.S,%.S.o,$(ASFILES))
OBJFILES += $(SYMSFILES:.syms=.syms.o)

# Hidden directory for dependency files
DEPDIR = .deps
df = $(DEPDIR)/$(*F)

.PHONY: all clean install envcheck picocom sendelf_crc32 sendtar_crc32 disasm
.SUFFIXES:

all: $(HEXFILE)

clean:
	$(RM) helpers/sendelf_crc32
	$(RM) -rf $(HEXFILE) $(ELFFILE)
	$(RM) -rf $(BUILDDIR)/*
	$(RM) -rf $(DEPDIR)

picocom:
	picocom -b 19200 -y n --send-cmd "./helpers/$(PICOCOM_FTP)" $(TTYDEV)

sendelf_crc32:
	@echo "$(TARGET)" | grep mcb32 > /dev/null && { \
		echo "This should be compiled in the native environment!"; \
		exit 1; \
	} || { \
		gcc -Wall -Wextra -o helpers/sendelf_crc32 helpers/sendelf_crc32.c; \
	}

sendtar_crc32:
	@echo "$(TARGET)" | grep mcb32 > /dev/null && { \
		echo "This should be compiled in the native environment!"; \
		exit 1; \
	} || { \
		gcc -Wall -Wextra -o helpers/sendtar_crc32 helpers/sendtar_crc32.c; \
	}

disasm: envcheck
	$(TARGET)objdump -d $(PROGNAME).elf > untracked/$(PROGNAME).elf.S

envcheck:
	@echo "$(TARGET)" | grep mcb32 > /dev/null || (\
		echo ""; \
		echo " **************************************************************"; \
		echo " * Make sure you have sourced the cross compiling environment *"; \
		echo " * Do this by issuing:                                        *"; \
		echo " * . /path/to/crosscompiler/environment                       *"; \
		echo " **************************************************************"; \
		echo ""; \
		exit 1)

install: envcheck
	$(TARGET)avrdude -v -p $(shell echo "$(DEVICE)" | tr '[:lower:]' '[:upper:]') -c stk500v2 -P "$(TTYDEV)" -b $(TTYBAUD) -U "flash:w:$(HEXFILE)"

$(ELFFILE): $(OBJFILES) envcheck
	$(CC) $(CFLAGS) -o $@ $(OBJFILES:%=$(BUILDDIR)/%) $(LDFLAGS)

$(HEXFILE): $(ELFFILE) envcheck
	$(TARGET)bin2hex -a $(ELFFILE)

$(DEPDIR):
	@mkdir -p $@
	@mkdir -p build

# Compile C files
%.c.o: $(SOURCEDIR)/%.c envcheck | $(DEPDIR)
	echo $*.c.d $(df).c.P $$
	$(CC) $(CFLAGS) -c -MD -o $(BUILDDIR)/$@ $<
	@cp $(BUILDDIR)/$*.c.d $(df).c.P; sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(BUILDDIR)/$*.c.d >> $(df).c.P

# Compile ASM files with C pre-processor directives
%.S.o: $(SOURCEDIR)/%.S envcheck | $(DEPDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c -MD -o $(BUILDDIR)/$@ $<
	@cp $(BUILDDIR)/$*.S.d $(df).S.P; sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(BUILDDIR)/$*.S.d >> $(df).S.P;

# Link symbol lists to object files
%.syms.o: %.syms
	$(LD) -o $@ -r --just-symbols=$<

# Check dependencies
-include $(CFILES:%.c=$(DEPDIR)/%.c.P)
-include $(ASFILES:%.S=$(DEPDIR)/%.S.P)

