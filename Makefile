LIBNAME := elfu
EXENAME := elfucli

LIBRARIES := libelf

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

INCLUDES := $(patsubst %, -I%, $(INCLUDEDIR) $(SRCDIR)) $(shell pkg-config --cflags-only-I $(LIBRARIES))
CFLAGS   := -g -Wall -pedantic -Wno-variadic-macros -fPIC $(shell pkg-config --cflags-only-other $(LIBRARIES))
LDFLAGS  := $(shell pkg-config --libs $(LIBRARIES))



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

$(SHAREDLIB): $(LIBOBJS)
	gcc -shared -Wl,-soname,lib$(LIBNAME).so.$(SHARED_VERMAJ) -o $@ $^ $(LDFLAGS)

$(STATICLIB): $(LIBOBJS)
	ar rcs $@ $^

$(BUILDDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(INCLUDES) $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/
	make -C tests clean


.PHONY: distclean
distclean: clean
	find . -xdev -name "*~" -exec rm {} \;
	find . -xdev -name "core" -exec rm {} \;
