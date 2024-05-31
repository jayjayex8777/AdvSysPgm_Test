obj-m := chardev.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Some tutorials use M= instead of SUBDIRS= You may need to be root to
# compile the module. You must be root to insert it.
default:
        $(MAKE) -C $(KDIR) M=$(PWD) modules
        gcc -Wall -o quiz1 quiz1.c
        gcc -Wall -o quiz2 quiz2.c
        gcc -Wall -o quiz3 quiz3.c
clean:
        rm -f quiz1 quiz2 quiz3
        $(MAKE) -C $(KDIR) M=$(PWD) clean
