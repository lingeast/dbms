CC=g++
LDFLAGS=
all: libbpt.a

libbpt.a: sub bptree.o bptscanitr.o
	ar rcs libbpt.a bptree.o bptscanitr.o ./file/file_helper.o ./key/intkey.o ./key/varcharkey.o ./key/floatkey.o ./page/page.o
sub:
	cd file; make
	cd key; make
	cd page; make
	
test.o : test.cpp

bptree.o: bptree.cpp

bpscanitr.o: bpscanitr.cpp

.PHONY: clean
clean:
	-rm *.o *.a ./file/*.a ./file/*.o ./key/*.a ./key/*.o  ./page/*.a ./page/*.o 
