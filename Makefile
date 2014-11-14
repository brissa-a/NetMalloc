ifneq ($(KERNELRELEASE),)
obj-m := file_trigger.o net.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
#KERNELDIR ?= /home/brissa-a/kernel/linux
KERNELDIR ?= /usr/src/linux-headers-3.13.0-35-generic
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules clean
endif
