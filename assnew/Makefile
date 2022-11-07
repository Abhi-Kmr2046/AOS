obj-m+=a2_19CS10004.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean	