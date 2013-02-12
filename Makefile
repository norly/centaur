PROJ := elfedit

BUILDDIR   := build
INCLUDEDIR := include
SRCDIR     := src

EXE     := $(BUILDDIR)/$(PROJ)
HEADERS := $(shell find $(INCLUDEDIR)/ -iname "*.h")
HEADERS += $(shell find $(SRCDIR)/ -iname "*.h")

SOURCES := $(shell find $(SRCDIR)/ -iname "*.c")
OBJS    := $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))

INCLUDES := $(patsubst %, -I%, $(INCLUDEDIR) $(SRCDIR)) -I /usr/include/libelf
CFLAGS   := -g -Wall
LDFLAGS  := -lelf



.PHONY: default
default: $(EXE)


.PHONY: debug
debug: $(EXE)
	gdb $(EXE) $(shell ps -e | sed "s/^ *\([0-9]\+\) .*$(PROJ).*$$/\1/g;te;d;:e")


$(EXE): $(OBJS)
	@if [ ! -d $(BUILDDIR) ] ; then echo "Error: Build dir '$(BUILDDIR)' does not exist." ; false ; fi
	gcc -o $@ $^ $(LDFLAGS)


$(BUILDDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	gcc $(INCLUDES) $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -f $(STATICLIB)
	rm -f $(OBJS)
	rm -f $(TESTEXES)
	rm -rf $(BUILDDIR)/


.PHONY: distclean
distclean: clean
	find . -xdev -name "*~" -exec rm {} \;
	find . -xdev -name "core" -exec rm {} \;
