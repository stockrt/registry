
all:
	@#gcc -fPIC -rdynamic -g -c -Wall ioctl_preload.c
	#gcc -shared -Wl,-soname,libmacspoof.so.1 -o libmacspoof.so.1.0.1 ioctl_preload.c -lc -ldl
	gcc -shared -Wl,-soname,libmacspoof.so.1 -o libmacspoof.so.1.0.1 ioctl_preload.64b.c -lc -ldl
	@echo ""
	@echo "-- To test: (with tcsh shell)"
	@echo ""
	@echo "ifconfig | grep -i hwaddr"
	@echo setenv LD_PRELOAD `pwd`/libmacspoof.so.1.0.1
	@echo "ifconfig | grep -i hwaddr"
	@echo setenv MAC_ADDRESS "42:fe:ed:be:ef:99"
	@echo "ifconfig | grep -i hwaddr"
	@echo unsetenv MAC_ADDRESS
	@echo "ifconfig | grep -i hwaddr"
	@echo unsetenv LD_PRELOAD
	@echo "ifconfig | grep -i hwaddr"


clean:
		rm -f libmacspoof.so.1.0.1
