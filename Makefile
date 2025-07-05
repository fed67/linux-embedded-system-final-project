#obj-m += onewire_dev.o

#Define variable if it is built outside of yocto
# KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

GCC = gcc
CFLAGS = 
LDFLAGS = -lrt -lgpiod

all:
	gcc main.c -g -o my-one $(LDFLAGS)


# clean:
# 	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean


#SRC := $(shell pwd)

#all:
#	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

#modules_install:
#	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

#clean:
#	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
