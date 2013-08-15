#
# Makefile to build animal-i2c driver for Linux using `kbuild' build system
#

# Kernel header files to install
HDR_FILES:=$(HDR_FILES) animal-i2c/i2c_usim.h animal-i2c/animal-i2c_api.h

EXT_FILES=\
	animal-i2c.conf:/etc/modprobe.d

obj-m := animal-i2c.o animal-i2c-usim.o

animal-i2c-objs := i2c_main.o
animal-i2c-usim-objs := i2c_usim.o

ccflags-y := -Wall -Wstrict-prototypes


