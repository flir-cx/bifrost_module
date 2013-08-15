
# Kernel header files to install
HDR_FILES:=$(HDR_FILES) cmemk/cmemk.h

# This is the kernel based makefile for the module

ifeq ($(ARCH), arm)
  ccflags-y := -Wall -Wstrict-prototypes -DUSE_UDEV=0 -DMAX_POOLS=4
else
  ccflags-y := -Wall -Wstrict-prototypes -DUSE_UDEV=0 -DMAX_POOLS=4 -D__SIM
endif

obj-m := cmemk.o



