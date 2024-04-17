#
# Makefile to build Bifrost driver for Linux using `kbuild' build system
#
# Control build by adding one or many to ccflags-y:
#  -DBIFROST_DEBUG: for debug outputs using printk()
#

obj-m := bifrost.o

bifrost-objs := bifrost_main.o
bifrost-objs += bifrost_cdev.o
bifrost-objs += bifrost_pci.o
bifrost-objs += bifrost_dma.o
bifrost-objs += bifrost_membus.o
bifrost-objs += bifrost_platform.o

ccflags-y := -Wall -Wstrict-prototypes



