# Part executed when called from kernel build system
ifneq ($(KERNELRELEASE),)

obj-m += fan_pwm_driver.o ## name of the generated module
mymodule-objs := fan_pwm_driver.o ## list of objets needed by that module
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

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	
install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

endif
