# Allow the module to be built with an arbitrary name
MOD ?= monstars_nf

# Bogus signature
CONFIG_MODULE_SIG = monstars

# KBuild
ifneq ($(KERNELRELEASE),)
obj-m := $(MOD).o

# Make
else
KDIR ?= /lib/modules/$(KERNEL_VER)/build

release:
	$(MAKE) CFLAGS_MODULE='-Os $(FLAGS)' -C $(KDIR) M=$$PWD
	strip --strip-debug $(MOD).ko
	mv $(MOD).ko ../build/$(MOD)-$(KERNEL_VER).ko

debug:
	$(MAKE) CFLAGS_MODULE='-DDEBUG -Og -g $(FLAGS)' -C $(KDIR) M=$$PWD
	mv $(MOD).ko ../build/$(MOD)-$(KERNEL_VER).ko

clean:
	rm *.order *.symvers *.mod* $(MOD).o .*.cmd

endif