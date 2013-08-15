# Link to latest build-system if available in top dir 
$(shell ( ! [ -d ../maxmake ] || [ -h maxmake ] ) || ln -s ../maxmake maxmake )

include maxmake/env.mk

NAME=modules
MODS=cmemk animal-i2c bifrost
GOALS=clean help modules 

KMFLAGS:=$(KMFLAGS) -C $(KERNELDIR)
ifneq ($(ARCH), $(BLDARCH))
  KMFLAGS:=$(KMFLAGS) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
endif

# Include kernel makefiles for headers and such
include $(foreach v,$(MODS),$(v)/Makefile)


all:modules
	@true

test:

distclean:
	@$(MAKE) -C . clean

include maxmake/inst_t.mk

.PHONY:$(GOALS)
$(GOALS): $(foreach v,$(MODS),mod-$(v))
	$(ECHO) "DONE : modules $(MAKECMDGOALS)"

.PHONY:install
install: $(foreach v,$(MODS),inst-$(v)) install-dev
	$(ECHO) "DONE : modules install"

uninstall:

install-dev:
	$_ $(call INSTALL,$(HDR_FILES),$(INCDIR))

inst-%::
	+$_ exec $(MAKE) $(KMFLAGS) M=$(shell pwd)/$* INSTALL_MOD_PATH=$(DESTDIR) modules_install


mod-%::
	$(ECHO) " MOD : $* $(MAKECMDGOALS)"
	+$_ exec $(MAKE) $(KMFLAGS) M=$(shell pwd)/$* $(MAKECMDGOALS)
