Introduction
-------------
	This document will show you how to compile and test our hardware encryption engine.
	If you want to know the details about encryption architecture,please see /linux/documentation/crypto.

Hardware encryption engine driver
----------------------------------
Compile:
	Hardware encryption engine driver is locate at:
	make menuconfig_public_linux
	-*- Cryptographic API  --->
		[*]   Hardware crypto devices  --->
			<M>   Driver for Ambarella HW CRYPTO
	After compile, you will get the driver module -- ambarella_crypto
usage:
	Load the module into kernel:
		modprobe ambarella_crypto config_polling_mode=1
Note: 
        At the end of the test log, you will see an error :
	modprobe: can't load module tcrypt (kernel/crypto/tcrypt.ko): Resource temporarily unavailabl
	This is not an really error.Because the test function run in the module_init function, after testing,
	we do not need to modprobe this module into kernel,so the function return an error.
	Just ignore this error.

speed test module
------------------
summary :
	The speed test module is provided by kernel. It is compiled as a module.
	It will test the speed of algorithm when loading into the kernel. And this module doesn't
	care about the accuracy of the algorithm.
compile:
	make menuconfig_public_linux
	-*- Cryptographic API  --->
		 <M>   Testing module
	After compile, you will get the test module -- tcrypt.
usage:
	aes test :
		modprobe tcrypt mode=200 sec=1
	this module support many kinds algorithm, for example:
	mode = 200 : aes algorithm test;
	mode = 204 : des algorithm test;
	...
	Take a look in tcrypt.c, function  do_test. You can find all kinds of test mode.

old accuracy test module
-------------------------
summary :
	The old accuracy test module is used for kernel before 2.6.38 version. Because there is no standard interface
	before 2.6.38 version. It depends on our private module -- ambac.ko, which provide a interface for old accuracy test
	module. We recommend you to use new accuracy test module, not this one.
compile:
	1¡¢compile ambac.ko
		make menuconfig
		[*] Build Ambarella Linux Kernel -->
			 Ambarella Private Linux Configuration  --->
				 [*] Build Ambarella private crypto module
	2¡¢compile old accuracy test module
		make menuconfig
		[*] Build Unit Tests -->
			Ambarella Public Linux Unit test configs  --->
				[*]   Build Ambarella Public Linux Crypto unit tests
usage:
	modprobe ambac
	test_crypto -r

new accuracy test module
-------------------------
summary:
	The new accuracy test module is used on kernel 2.6.38 and later version. It depend on standard
	CryptoAPI in the kernel. So we recommend you to use this one as a accuracy test tool.
compile:
	make menuconfig_public_linux
	-*- Cryptographic API  --->
		 <*>   User-space interface for hash algorithms
		 <*>   User-space interface for symmetric key cipher algorithms
	make menuconfig
	[*] Build Unit Tests -->
		 Ambarella Public Linux Unit test configs  --->
			[*]   Build Crypto Test With Socket Interface
	after compile,you will get the module -- test_crypto_socket

usage:
	test_crypto_socket -q
	or
	test_crypto_socket   for detail information .








