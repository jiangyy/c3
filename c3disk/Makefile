obj-m := c3disk.o
c3disk-objs := mod.o diskops.o rob.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

play: all
	./run
