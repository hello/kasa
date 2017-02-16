
dwsim.out: ./XTMPsim.o ./diagwind.o
	gcc \
	-g \
	-o $@ \
	$^ \
	$(XTENSA_TOOLS_ROOT)/lib/iss/libsimxtcore.so \
	$(XTENSA_TOOLS_ROOT)/lib/iss/libxtmp.so \
	$(XTENSA_TOOLS_ROOT)/lib/iss/libsimctl.so \
	$(XTENSA_TOOLS_ROOT)/lib/iss/qt_driver.so \
	$(XTENSA_TOOLS_ROOT)/Tools/lib/libqt.a \
	-ldl


diagwind.o: diagwind.c
	gcc -g -Wall -Iinclude -I../include -I../../include -c -o $@ $<

#
# Compile as an AR6000 application which happens to know some special
# AR6002 addresses.
#
XTMPsim.o: XTMPsim.c
	gcc -g -DAR6000 -Iinclude -I../include -I../../include -I../../include/AR6000 -I$(XTENSA_TOOLS_ROOT)/include -c -o $@ $<

clean:
	rm -f XTMPsim.o diagwind.o

clobber: clean
	rm -f dwsim.out
