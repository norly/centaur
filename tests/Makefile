BUILDDIR = build
SRCDIR   = src

SOURCES  = $(shell find $(SRCDIR)/ -iname "*.c")
OBJS     = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))
EXENAMES = putsmain putsmainsub
EXES     = $(patsubst %, $(BUILDDIR)/%, $(EXENAMES))

CFLAGS = -Wall -pedantic



.PHONY: testbase
testbase: $(OBJS) $(EXES)


.PHONY: check
check: testbase
	./runtests.sh


$(BUILDDIR)/putsmain: $(SRCDIR)/putsmain.c
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(CFLAGS) -o $@ $^

$(BUILDDIR)/putsmainsub: $(SRCDIR)/putsmainsub.c
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/
