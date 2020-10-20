# Part executed when called from kernel build system
ifneq ($(KERNELRELEASE),)

obj-m += mymodule.o ## name of the generated module
mymodule-objs := skeleton.o ## list of objets needed by that module
# CFLAGS_skeleton.o
CFLAGS_skeleton := -DDEBUG ## to enable printing of debugging messages

# Part executed when called from standard make in the module source directory
else
CPU := arm64
KDIR := /home/lmi/workspace/nano/buildroot/output/build/linux-5.8.6/
TOOLS := /home/lmi/workspace/nano/buildroot/output/host/usr/bin/aarch64-none-linux-gnu-
PWD := $(shell pwd)

#MODPATH := $(HOME)/workspace/buildroot/output/target # production mode
MODPATH := /home/lmi/workspace/nano/rootfs

DTB = mydtb.dtb
DTS = $(DTB:.dtb=.dts)
INCL =-I. -I$(KDIR)/include -I$(KDIR)/arch/arm64/boot/dts

dtb: $(DTB)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

	
install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

$(DTB) : $(DTS)
	ln -s $(KDIR)/arch/arm/boot/dts arm
	-cpp $(INCL) -E -P -x assembler-with-cpp $(DTS) | dtc -I dts -O dtb -o $(DTB) -
	rm arm

endif
