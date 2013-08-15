#
# Makefile to build Bifrost driver for Linux using `kbuild' build system
#
# Control build by adding one or many to ccflags-y:
#  -DBIFROST_DEBUG: for debug outputs using printk()
#
HDR_FILES:=$(HDR_FILES) bifrost/bifrost_api.h  \
	$(shell find bifrost -name 'fpga_*.h' -printf 'bifrost/%P ') \
	$(shell find bifrost -name 'valhalla_*.h' -printf 'bifrost/%P ')

obj-m := bifrost.o

bifrost-objs := bifrost_main.o bifrost_fb.o bifrost_cdev.o bifrost_pci.o bifrost_sim.o bifrost_dma.o

ccflags-y := -Wall -Wstrict-prototypes

EXT_FILES=\
	bifrost.conf:/etc/modprobe.d


