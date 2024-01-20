# Allow the module to be built with an arbitrary name
MOD ?= blanko
OUT = ../../../build/blanko/$(KERNEL_VER)/$(MOD).ko

# Bogus signature
CONFIG_MODULE_SIG = monstars

# KBuild
ifneq ($(KERNELRELEASE),)
obj-m := $(MOD).o

# Make
else

# Cross-compiler toolchain with older binutils version
PREFIX = /opt/cross/gcc-4.9.0-nolibc/x86_64-linux
PATH := $(PREFIX)/bin:$(PATH)
export ARCH = x86_64
export CROSS_COMPILE = x86_64-linux-

KDIR ?= /lib/modules/$(KERNEL_VER)/build

release:
	$(MAKE) CFLAGS_MODULE='-Os -gdwarf-2' -C $(KDIR) M=$$PWD
	strip --strip-debug $(MOD).ko
	mv $(MOD).ko $(OUT)

debug:
	$(MAKE) CFLAGS_MODULE='-DDEBUG -Og -g -gdwarf-2' -C $(KDIR) M=$$PWD
	mv $(MOD).ko $(OUT)

clean:
	rm -r *.order *.symvers *.mod* *.o .*.cmd .cache.mk .tmp_versions

endif