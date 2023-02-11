# Allow the module to be built with an arbitrary name
MOD ?= monstars_nf
OUT = ../build/$(KERNEL_VER)/$(MOD).ko

# Bogus signature
CONFIG_MODULE_SIG = monstars

# KBuild
ifneq ($(KERNELRELEASE),)
obj-m := $(MOD).o

# Make
else
KDIR ?= /lib/modules/$(KERNEL_VER)/build

release:
	$(MAKE) CFLAGS_MODULE='-Os -gdwarf-2 $(KFLAGS)' -C $(KDIR) M=$$PWD
	strip --strip-debug $(MOD).ko
	mv $(MOD).ko $(OUT)

debug:
	$(MAKE) CFLAGS_MODULE='-DDEBUG -Og -g -gdwarf-2 $(KFLAGS)' -C $(KDIR) M=$$PWD
	mv $(MOD).ko $(OUT)

clean:
	rm *.order *.symvers *.mod* $(MOD).o .*.cmd

endif