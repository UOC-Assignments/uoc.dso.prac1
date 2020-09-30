
ifneq ($(KERNELRELEASE),)
	obj-m := traceexit.o 
else
	include ../Makefile
	KERNELDIR := ../../$(KERNEL_VERSION)
	PWD := $(shell pwd)

all: modules exit_code

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

exit_code: extras/exit_code.c
	gcc -static -D__KERNEL__ -I../../$(KERNEL_VERSION)/include exit_code.c -o exit_code

clean:
	rm -rf *.[oas] .*.flags *.ko .*.cmd .*.d .*.tmp *.mod.c .tmp_versions Module.symvers
