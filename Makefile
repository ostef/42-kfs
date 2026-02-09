TARGET_ISO=pantheon.iso
TARGET=kernel.bin
TARGET_ARCH=i686-elf
SOURCE_FILES=boot.asm \
	kernel.c \
	com.c \
	ioport.c \
	interrupts.c \
	vga.c \
	tty.c \
	interrupt_handlers.asm \
	keyboard.c \
	LibKernel/memory.c \
	LibKernel/string.c \
	LibKernel/print.c

OBJECT_FILES=$(addsuffix .o,$(SOURCE_FILES))
DEP_FILES=$(addsuffix .d,$(SOURCE_FILES))

SOURCE_DIR=Source
INCLUDE_DIRS=$(SOURCE_DIR)
SYSTEM_DIR=System
BUILD_DIR=.build

TOOLS_PREFIX=$(HOME)/kfs-tools

LD=$(TOOLS_PREFIX)/bin/$(TARGET_ARCH)-ld
GCC=$(TOOLS_PREFIX)/bin/$(TARGET_ARCH)-gcc
GDB=$(TOOLS_PREFIX)/bin/$(TARGET_ARCH)-gdb

GRUB_COMPRESS?=xz

C_FLAGS?=-O0 -g
C_FLAGS:=$(C_FLAGS) -ffreestanding -Wall -Wextra
C_INCLUDE_DIRS=$(addprefix -I,$(INCLUDE_DIRS))
LIBS=gcc
LINK_FLAGS=-ffreestanding -nostdlib

all: $(TARGET_ISO)

runk: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -no-reboot -d cpu_reset

run: $(TARGET_ISO)
	qemu-system-i386 -cdrom $(TARGET_ISO) -serial stdio -no-reboot -d cpu_reset

$(TARGET_ISO): $(TARGET)
	cp $(TARGET) $(SYSTEM_DIR)/boot/pantheon
	grub-mkrescue --compress=$(GRUB_COMPRESS) $(SYSTEM_DIR) -o $@

$(TARGET): $(addprefix $(BUILD_DIR)/,$(OBJECT_FILES)) $(SOURCE_DIR)/linker.ld
	$(GCC) -T $(SOURCE_DIR)/linker.ld $(LINK_FLAGS) $(addprefix $(BUILD_DIR)/,$(OBJECT_FILES)) $(addprefix -l,$(LIBS)) -o $@
	grub-file --is-x86-multiboot $@

$(BUILD_DIR)/%.asm.o: $(SOURCE_DIR)/%.asm Makefile
	@ mkdir -p $(@D)
	nasm -felf32 -MP -MD $(BUILD_DIR)/$*.asm.d $< -o $@

$(BUILD_DIR)/%.c.o: $(SOURCE_DIR)/%.c Makefile
	@ mkdir -p $(@D)
	$(GCC) $(C_FLAGS) $(C_INCLUDE_DIRS) -MMD -MP -MF$(BUILD_DIR)/$*.c.d -c $< -o $@

-include $(addprefix $(BUILD_DIR)/,$(DEP_FILES))

clean:
	rm -rf .build

fclean: clean
	rm $(TARGET)

re: fclean all

toolchain: $(LD) $(GCC) $(GDB)

BINUTILS_VERSION=2.45
GCC_VERSION=15.2.0
GDB_VERSION=17.1

BINUTILS_SRC=$(TOOLS_PREFIX)/src/binutils-$(BINUTILS_VERSION)
GCC_SRC=$(TOOLS_PREFIX)/src/gcc-$(GCC_VERSION)
GDB_SRC=$(TOOLS_PREFIX)/src/gdb-$(GDB_VERSION)

BINUTILS_BUILD=$(BUILD_DIR)/binutils-$(BINUTILS_VERSION)
GCC_BUILD=$(BUILD_DIR)/gcc-$(GCC_VERSION)
GDB_BUILD=$(BUILD_DIR)/gdb-$(GDB_VERSION)

NUM_JOBS?=1

$(BINUTILS_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://mirror.ibcp.fr/pub/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/binutils-$(BINUTILS_VERSION).tar.xz
	mv binutils-$(BINUTILS_VERSION) $(TOOLS_PREFIX)/src

$(GCC_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://mirror.ibcp.fr/pub/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/gcc-$(GCC_VERSION).tar.xz
	mv gcc-$(GCC_VERSION) $(TOOLS_PREFIX)/src

$(GDB_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://mirror.ibcp.fr/pub/gnu/gdb/gdb-$(GDB_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/gdb-$(GDB_VERSION).tar.xz
	mv gdb-$(GDB_VERSION) $(TOOLS_PREFIX)/src

$(LD): $(BINUTILS_SRC)
	mkdir -p $(BINUTILS_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(BINUTILS_BUILD); $(BINUTILS_SRC)/configure --target=$(TARGET_ARCH) --prefix=$(TOOLS_PREFIX) --with-sysroot --disable-nls --disable-werror

	make -C $(BINUTILS_BUILD) -j $(NUM_JOBS)
	make -C $(BINUTILS_BUILD) install

$(GCC): $(GCC_SRC)
	mkdir -p $(GCC_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(GCC_BUILD); $(GCC_SRC)/configure --target=$(TARGET_ARCH) --prefix=$(TOOLS_PREFIX) --disable-nls --enable-languages=c,c++ --without-headers

	make -C $(GCC_BUILD) -j $(NUM_JOBS) all-gcc
	make -C $(GCC_BUILD) -j $(NUM_JOBS) all-target-libgcc
	make -C $(GCC_BUILD) install-gcc
	make -C $(GCC_BUILD) install-target-libgcc

$(GDB): $(GDB_SRC)
	mkdir -p $(GDB_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(GDB_BUILD); $(GDB_SRC)/configure --target=$(TARGET_ARCH) --prefix=$(TOOLS_PREFIX) --disable-werror

	make -C $(GDB_BUILD) -j $(NUM_JOBS) all-gdb
	make -C $(GDB_BUILD) install-gdb

.PHONY: all toolchain clean fclean re
