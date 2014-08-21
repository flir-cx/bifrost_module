
PREFIX?=/usr/local
MODS=cmemk animal-i2c bifrost
DOCS=bifrost
MAKE:=$(MAKE) --no-print-directory

ifeq ($(DESTDIR), )
 DESTDIR=$(EXTDIR)
 PREFIX=
endif

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

distclean clean:: $(MODS)
	@true


prepare:
	set -e ; if [ "$$(cat prepare.arch)" != "$(ARCH)" ] ; then \
		echo "Prepare"; \
		if $(MAKE) -C $(KERNELDIR) prepare modules_prepare -j10 ; then \
			echo "$(ARCH)" > prepare.arch; \
		else \
			echo "** COULD NOT STAMP PREPARE **"; \
			echo "** You probably need to prepare your arch with sudo..."; \
			exit 1; \
		fi; \
	fi


.PHONY:$(MODS)
$(MODS):
	@echo "  MOD : $@ $(MAKECMDGOALS)"
	@$(MAKE) $(KMFLAGS) M=$(shell pwd)/$@ $(MAKECMDGOALS)

%-inst:
	@echo " INST : $*"
	@$(MAKE) $(KMFLAGS) M=$(shell pwd)/$* INSTALL_MOD_PATH=$(DESTDIR) modules_install

install: $(foreach v,$(MODS),$v-inst)
	@true

install-dev: install
	@echo " INST : modules dev"
	@mkdir -p $(sort $(dir $(addprefix $(DESTDIR)/$(PREFIX)/usr/include/,$(HDR_FILES))))
	@cp -u $(HDR_FILES) $(DESTDIR)/$(PREFIX)/usr/include

