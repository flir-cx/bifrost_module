
PREFIX?=/usr/local
MODS=cmemk animal-i2c bifrost
DOCS=bifrost
MAKE:=$(MAKE) --no-print-directory

# Enable nice make
ifdef V
  E:=@true
  R:=
else
  E:=@echo
  R:=@
  MAKE:=@$(MAKE) --no-print-directory
endif

ifeq ($(DESTDIR), )
 DESTDIR=$(EXTDIR)
 PREFIX=
endif

KMFLAGS:=$(KMFLAGS) -C $(KERNELDIR)
ifneq ($(ARCH), $(BLDARCH))
  KMFLAGS:=$(KMFLAGS) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
endif

# Kernel header files to install

EXT_FILES:=\
	cmemk/cmemk.conf:/etc/modprobe.d \
	animal-i2c/animal-i2c.conf:/etc/modprobe.d \
	bifrost/bifrost.conf:/etc/modprobe.d \
	cmemk/cmemk.pools:/etc \
	cmemk/cmemk_modopts:/sbin \
	flir_chardev_mod:/sbin

all: $(MODS)
	@true

clean:: $(MODS)
	@true

distclean::
	$(MAKE) -C . clean

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

BIFROST_HDR_FILES:=\
	bifrost/bifrost_api.h  \
	$(shell find bifrost -name 'fpga_*.h' -printf 'bifrost/%P ') \
	$(shell find bifrost -name 'valhalla_*.h' -printf 'bifrost/%P ')

.PHONY:$(MODS)
$(MODS):
	$E "  MOD : $@ $(MAKECMDGOALS)"
	$R $(MAKE) $(KMFLAGS) M=$(shell pwd)/$@ $(MAKECMDGOALS)

%-inst:
	$E " INST : $*"
	$R $(MAKE) $(KMFLAGS) M=$(shell pwd)/$* INSTALL_MOD_PATH=$(DESTDIR) modules_install

install: install-rel install-dev
	@true

install-rel: $(foreach v,$(MODS),$v-inst)
	$E " INST : mod"
	$R install -C -d $(DESTDIR)/etc/modprobe.d
	$R install -C -t $(DESTDIR)/etc/modprobe.d cmemk/cmemk.conf
	$R install -C -t $(DESTDIR)/etc/modprobe.d animal-i2c/animal-i2c.conf
	$R install -C -t $(DESTDIR)/etc/modprobe.d bifrost/bifrost.conf
	$R install -C -d $(DESTDIR)/etc
	$R install -C -t $(DESTDIR)/etc cmemk/cmemk.pools
	$R install -C -d $(DESTDIR)/sbin
	$R install -C -t $(DESTDIR)/sbin cmemk/cmemk_modopts
	$R install -C -t $(DESTDIR)/sbin flir_chardev_mod

install-dev: 
	$E " INST : mod dev"
	$R install -C -d $(DESTDIR)/$(PREFIX)/include/cmemk
	$R install -C -t $(DESTDIR)/$(PREFIX)/include/cmemk cmemk/cmemk.h
	$R install -C -d $(DESTDIR)/$(PREFIX)/include/animal-i2c
	$R install -C -t $(DESTDIR)/$(PREFIX)/include/animal-i2c animal-i2c/i2c_usim.h animal-i2c/animal-i2c_api.h
	$R install -C -d $(DESTDIR)/$(PREFIX)/include/bifrost
	$R install -C -t $(DESTDIR)/$(PREFIX)/include/bifrost $(BIFROST_HDR_FILES) 




#	@echo " INST : modules dev"
#	@mkdir -p $(sort $(dir $(addprefix $(DESTDIR)/$(PREFIX)/usr/include/,$(HDR_FILES))))
#	@cp -u $(HDR_FILES) $(DESTDIR)/$(PREFIX)/usr/include

