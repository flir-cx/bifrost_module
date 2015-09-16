
# This is the kernel based makefile for the module

ifeq ($(ARCH), arm)
  ccflags-y := -Wall -Wstrict-prototypes -DUSE_UDEV=0 -DMAX_POOLS=4 $(call cc-disable-warning, date-time)
else
  ccflags-y := -Wall -Wstrict-prototypes -DUSE_UDEV=0 -DMAX_POOLS=4 -D__SIM
endif

obj-m := cmemk.o



