obj-m += GeneralModule.o


KERNEL_DIR=/lib/modules/$(shell uname -r)/build/


#  If we were cross-compiling we would add: 	ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \  to each of these
all:
	make -C $(KERNEL_DIR) M=$(shell pwd) modules
	/usr/src/linux-headers-$(shell uname -r)/scripts/sign-file sha256 \
	    ~/.../<key>.priv \
	    ~/.../<key>.der \
	    ./GeneralModule.ko 

clean:
	make -C $(KERNEL_DIR) M=$(shell pwd) clean 



