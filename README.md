# 42 Kernel From Scratch project

## KFS-1

### References

- https://wiki.osdev.org/GCC_Cross-Compiler
- https://wiki.osdev.org/Bare_Bones

### Setting up

Prerequisites (Debian based systems):

```Shell
sudo apt install build-essential nasm bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev qemu-system-x86 xorriso mtools grub-pc-bin
```

Build cross compiling toolchain:
`make toolchain`

Build the iso file with `make` then you can launch the VM with:

```Shell
qemu-system-i386 -cdrom pantheon.iso
```
