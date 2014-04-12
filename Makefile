all:
	make -C /lib/modules/`uname -r`/build M=$(PWD)/ubertooth modules

clean:
	

