obj-m += onewire_dev.o

#Define variable if it is built outside of yocto
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

# all:
# 	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

# clean:
# 	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean