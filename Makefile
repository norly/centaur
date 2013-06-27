LIBNAME := elfu
EXENAME := elfucli

BUILDDIR   := build
INCLUDEDIR := include
SRCDIR     := src

EXE       := $(BUILDDIR)/$(EXENAME)
STATICLIB := $(BUILDDIR)/lib$(LIBNAME).a

SHARED_VERMAJ := 0
SHARED_VERMIN := 0
SHARED_VERREV := 0
SHAREDLIB     := $(BUILDDIR)/lib$(LIBNAME).so.$(SHARED_VERMAJ).$(SHARED_VERMIN).$(SHARED_VERREV)

HEADERS := $(shell find $(INCLUDEDIR)/ -iname "*.h")
HEADERS += $(shell find $(SRCDIR)/ -iname "*.h")

ALLSRCS := $(shell find $(SRCDIR)/ -iname "*.c")
LIBSRCS := $(filter $(SRCDIR)/lib$(LIBNAME)/%.c, $(ALLSRCS))
EXESRCS := $(filter-out $(SRCDIR)/lib$(LIBNAME)/%.c, $(ALLSRCS))
LIBOBJS := $(patsubst %.c, $(BUILDDIR)/%.o, $(LIBSRCS))
EXEOBJS := $(patsubst %.c, $(BUILDDIR)/%.o, $(EXESRCS))


ifeq ($(shell pkg-config --exists libelf > /dev/null 2> /dev/null ; echo $$?),0)
	LIBELF_INCLUDES := $(shell pkg-config --cflags-only-I libelf)
	LIBELF_CFLAGS   := $(shell pkg-config --cflags-only-other libelf)
	LIBELF_LDFLAGS  := $(shell pkg-config --libs libelf)
else
	LIBELF_INCLUDES :=
	LIBELF_CFLAGS   :=
	LIBELF_LDFLAGS  := -lelf
endif


INCLUDES := $(patsubst %, -I%, $(INCLUDEDIR) $(SRCDIR)) $(LIBELF_INCLUDES)
CFLAGS   := -g -Wall -std=gnu99 -pedantic $(LIBELF_CFLAGS)
LDFLAGS  := $(LIBELF_LDFLAGS)




.PHONY: all
all: $(STATICLIB) $(SHAREDLIB) $(EXE)


.PHONY: check
check: $(EXE)
	make -C tests check


.PHONY: debug
debug: $(EXE)
	gdb $(EXE) $(shell ps -e | sed "s/^ *\([0-9]\+\) .*$(EXENAME).*$$/\1/g;te;d;:e")


$(EXE): $(EXEOBJS) $(STATICLIB)
	gcc -o $@ $^ $(LDFLAGS)

$(SHAREDLIB): $(LIBSRCS)
	gcc $(INCLUDES) $(CFLAGS) -shared -fPIC -Wl,-soname,lib$(LIBNAME).so.$(SHARED_VERMAJ) -o $@ $^ $(LDFLAGS)

$(STATICLIB): $(LIBOBJS)
	ar rcs $@ $^

$(BUILDDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(INCLUDES) $(CFLAGS) -c -o $@ $<


.PHONY: docs
docs: doxygen-doc

.PHONY: doxygen-doc
doxygen-doc:
	mkdir -p docs
	doxygen


.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/
	make -C tests clean


.PHONY: distclean
distclean: clean
	find . -xdev -name "*~" -exec rm {} \;
	find . -xdev -name "core" -exec rm {} \;
	rm -rf docs/doxygen/
