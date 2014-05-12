#
# Makefile to build Bifrost driver for Linux using `kbuild' build system
#
# Control build by adding one or many to ccflags-y:
#  -DBIFROST_DEBUG: for debug outputs using printk()
#

obj-m := bifrost.o

bifrost-objs := bifrost_main.o bifrost_cdev.o bifrost_pci.o bifrost_sim.o bifrost_dma.o bifrost_membus.o

ccflags-y := -Wall -Wstrict-prototypes



