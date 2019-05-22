HOW TO BUILD KERNEL 3.4.5 JBP FOR SM-T110_NA_XARd

1. How to Build
	- Get Toolchain (arm-eabi-4.6)
		- get Toolchain
	
	-	Extract kernel source	
	
	- edit Makefile
		edit "CROSS_COMPILE" to right toolchain path(You downloaded).
		  EX) export CROSS_COMPILE=/usr/local/toolchain/arm-eabi-4.6/bin/arm-eabi- 

	a. FOR SM-T110_NA_XARd
		$ export ARCH=arm
		$ make pxa986_goyawifi_rev02_defconfig
		$ make
	
2. Output files
	- Kernel : /arch/arm/boot/zImage
	
3. build clean
	- make clean
