# Allow the module to be built with an arbitrary name
MOD ?= monstars_nf

# KBuild
ifneq ($(KERNELRELEASE),)
obj-m := $(MOD).o

# Make
else
KERNEL_VER ?= `uname -r`
KDIR ?= /lib/modules/$(KERNEL_VER)/build

release:
	$(MAKE) CFLAGS_MODULE='-Os' -C $(KDIR) M=$$PWD
	strip --strip-debug $(MOD).ko
	mv $(MOD).ko ../build/$(MOD)-$(KERNEL_VER).ko

debug:
	$(MAKE) CFLAGS_MODULE='-DDEBUG -Og -g' -C $(KDIR) M=$$PWD
	mv $(MOD).ko ../build/$(MOD)-$(KERNEL_VER).ko

clean:
	rm *.order *.symvers *.mod* $(MOD).o

endif