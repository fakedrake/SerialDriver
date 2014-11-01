obj-m := fake_serial.o

# Ubuntu
# LINUX_HEADERS=/usr/src/linux-headers-$(shell uname -r)x
LINUX_HEADERS=$(PWD)/kernel
BOOTCMD=-append "console=ttyPS0,115200 root=/dev/nfs rw nfsroot=10.0.2.2:$$HOME/Public,proto=tcp  2049,proto=tcp ip=:::::eth0:dhcp"

all:
	make -C $(LINUX_HEADERS) M=$(PWD) modules

clean:
	make -C $(LINUX_HEADERS) M=$(PWD) clean
