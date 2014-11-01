# Building a fake serial driver

## Introduction

Linux special files and devices.

- major/minor numbers
- system calls
- /dev and /sys

## Modules vs built in

Emulators (qemu <3) are always a good idea.

### The bootstrap environment

http://github.com/fakedrake/xilinx-zynq-bootstrap

- Compiler (ARM)
- Kernel (Xilinx fork)
- Filesystem (directory/ramdisk/sdcard)
- Emulator (qemu)
- *Some other stuff not relevant here (JTAG/SSL/android/DirectFB/...)*

\* *Nothing too specific*

### Module

- Makefile
- Kernel headers
- Cross compiler
- .ko object (module)

### Built in module

- Kconfig (make menuconfig/make xconfig /...)
- Makefiles

## Dirty hands

### Objects

- Driver object
- Device object (mostly character devs)
- Operations and registration

### Loading and unloading

- Modprobe: the careful (symbol & dependency checks)
- Insmod: the brave
- Rmmod: should be unused
