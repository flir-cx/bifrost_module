
MODS=cmemk animal-i2c bifrost
DOCS=bifrost

KMFLAGS:=$(KMFLAGS) -C $(KERNELDIR)
ifneq ($(ARCH), $(BLDARCH))
  KMFLAGS:=$(KMFLAGS) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
endif


# Kernel header files to install
HDR_FILES:=$(HDR_FILES) cmemk/cmemk.h \
	animal-i2c/i2c_usim.h animal-i2c/animal-i2c_api.h \
	bifrost/bifrost_api.h  \
	$(shell find bifrost -name 'fpga_*.h' -printf 'bifrost/%P ') \
	$(shell find bifrost -name 'valhalla_*.h' -printf 'bifrost/%P ')

EXT_FILES:=\
	cmemk/cmemk.conf:/etc/modprobe.d \
	animal-i2c/animal-i2c.conf:/etc/modprobe.d \
	bifrost/bifrost.conf:/etc/modprobe.d \
	cmemk/cmemk.pools:/etc \
	cmemk/cmemk_modopts:/sbin \
	flir_chardev_mod:/sbin

all: $(MODS)
	@true

.PHONY:$(MODS)
$(MODS):
	+$_ exec $(MAKE) $(KMFLAGS) M=$(shell pwd)/$@ $(MAKECMDGOALS)
	

