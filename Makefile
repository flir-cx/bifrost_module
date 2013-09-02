# Link to latest build-system if available in top dir 
$(shell ( ! [ -d ../maxmake ] || [ -h maxmake ] ) || ln -s ../maxmake maxmake )


NAME:=modules
VER_MAJOR:=0
VER_MINOR:=0
VER_PATCH:=1

include maxmake/env.mk
include maxmake/defrules.mk
include maxmake/inst_t.mk

MODS=cmemk animal-i2c bifrost
GOALS=_clean modules 

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

_all::modules
	@true

prepare:
	-@$(MAKE) -C $(KERNELDIR) prepare modules_prepare -j10

test:

_distclean::
	@$(MAKE) -C . clean


$(GOALS):: $(foreach v,$(MODS),mod-$(v))
	$(ECHO) "DONE : modules $(MAKECMDGOALS)"

_installrel:: $(foreach v,$(MODS),inst-$(v))
	$(ECHO) "DONE : modules install"


_installdev::
	$_ $(call INSTALL,$(HDR_FILES),$(INCDIR))

uninstall:

inst-%::
	+$_ exec $(MAKE) $(KMFLAGS) M=$(shell pwd)/$* INSTALL_MOD_PATH=$(DESTDIR) modules_install


mod-%::
	$(ECHO) " MOD : $* $(MAKECMDGOALS)"
	+$_ exec $(MAKE) $(KMFLAGS) M=$(shell pwd)/$* $(MAKECMDGOALS)
