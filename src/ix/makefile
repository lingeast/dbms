
include ../makefile.inc

all: libix.a ixtest1 ixtest2

# lib file dependencies
libix.a: libix.a(ix.o)  # and possibly other .o files

# c file dependencies
ix.o: ix.h

ixtest1.o: ixtest_util.h

ixtest2.o: ixtest_util.h

# binary dependencies
ixtest1: ixtest1.o libix.a $(CODEROOT)/rbf/librbf.a  $(CODEROOT)/bpt/libbpt.a

ixtest2: ixtest2.o libix.a $(CODEROOT)/rbf/librbf.a  $(CODEROOT)/bpt/libbpt.a

# dependencies to compile used libraries
.PHONY: $(CODEROOT)/rbf/librbf.a $(CODEROOT)/bpt/libbpt.a
$(CODEROOT)/rbf/librbf.a:
	$(MAKE) -C $(CODEROOT)/rbf
$(CODEROOT)/bpt/libbpt.a:
	$(MAKE) -C $(CODEROOT)/bpt

.PHONY: clean
clean:
	-rm ixtest1 ixtest2 *.a *.o
	$(MAKE) -C $(CODEROOT)/rbf clean
	$(MAKE) -C $(CODEROOT)/bpt clean
