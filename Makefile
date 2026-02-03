TARGET=i686-elf

TOOLS_PREFIX=$(HOME)/kfs-tools

LD=$(TOOLS_PREFIX)/bin/$(TARGET)-ld
GCC=$(TOOLS_PREFIX)/bin/$(TARGET)-gcc
GDB=$(TOOLS_PREFIX)/bin/$(TARGET)-gdb

all:

toolchain: | $(LD) $(GCC) $(GDB)

BINUTILS_VERSION=2.45
GCC_VERSION=15.2.0
GDB_VERSION=17.1

BINUTILS_SRC=$(TOOLS_PREFIX)/src/binutils-$(BINUTILS_VERSION)
GCC_SRC=$(TOOLS_PREFIX)/src/gcc-$(GCC_VERSION)
GDB_SRC=$(TOOLS_PREFIX)/src/gdb-$(GDB_VERSION)

BINUTILS_BUILD=.build/binutils-$(BINUTILS_VERSION)
GCC_BUILD=.build/gcc-$(GCC_VERSION)
GDB_BUILD=.build/gdb-$(GDB_VERSION)

NUM_JOBS?=1

$(BINUTILS_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/binutils-$(BINUTILS_VERSION).tar.xz
	mv binutils-$(BINUTILS_VERSION) $(TOOLS_PREFIX)/src

$(GCC_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/gcc-$(GCC_VERSION).tar.xz
	mv gcc-$(GCC_VERSION) $(TOOLS_PREFIX)/src

$(GDB_SRC):
	mkdir -p $(TOOLS_PREFIX)
	wget -P $(TOOLS_PREFIX)/src https://ftp.gnu.org/gnu/gdb/gdb-$(GDB_VERSION).tar.xz
	tar -xf $(TOOLS_PREFIX)/src/gdb-$(GDB_VERSION).tar.xz
	mv gdb-$(GDB_VERSION) $(TOOLS_PREFIX)/src

$(LD): $(BINUTILS_SRC)
	mkdir -p $(BINUTILS_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(BINUTILS_BUILD); $(BINUTILS_SRC)/configure --target=$(TARGET) --prefix=$(TOOLS_PREFIX) --with-sysroot --disable-nls --disable-werror

	make -C $(BINUTILS_BUILD) -j $(NUM_JOBS)
	make -C $(BINUTILS_BUILD) install

$(GCC): $(GCC_SRC)
	mkdir -p $(GCC_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(GCC_BUILD); $(GCC_SRC)/configure --target=$(TARGET) --prefix=$(TOOLS_PREFIX) --disable-nls --enable-languages=c,c++ --without-headers

	make -C $(GCC_BUILD) -j $(NUM_JOBS) all-gcc
	make -C $(GCC_BUILD) -j $(NUM_JOBS) all-target-libgcc
	make -C $(GCC_BUILD) install-gcc
	make -C $(GCC_BUILD) install-target-libgcc

$(GDB): $(GDB_SRC)
	mkdir -p $(GDB_BUILD)
	mkdir -p $(TOOLS_PREFIX)

	cd $(GDB_BUILD); $(GDB_SRC)/configure --target=$(TARGET) --prefix=$(TOOLS_PREFIX) --disable-werror

	make -C $(GDB_BUILD) -j $(NUM_JOBS) all-gdb
	make -C $(GDB_BUILD) install-gdb

.PHONY: all toolchain clean fclean re
