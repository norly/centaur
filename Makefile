PROJ := elfucli

LIBRARIES := libelf

BUILDDIR   := build
INCLUDEDIR := include
SRCDIR     := src

EXE     := $(BUILDDIR)/$(PROJ)
HEADERS := $(shell find $(INCLUDEDIR)/ -iname "*.h")
HEADERS += $(shell find $(SRCDIR)/ -iname "*.h")

SOURCES := $(shell find $(SRCDIR)/ -iname "*.c")
OBJS    := $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))

INCLUDES := $(patsubst %, -I%, $(INCLUDEDIR) $(SRCDIR)) $(shell pkg-config --cflags-only-I $(LIBRARIES))
CFLAGS   := -g -Wall -pedantic -Wno-variadic-macros $(shell pkg-config --cflags-only-other $(LIBRARIES))
LDFLAGS  := $(shell pkg-config --libs $(LIBRARIES))



.PHONY: default
default: $(EXE)


.PHONY: check
check: $(EXE)
	$(error the re-layouting has broken make check for now, sorry.)
	$(EXE) $(EXE) -o testexe
	@cmp $(EXE) testexe
	@rm testexe
	@echo "Check successful."


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
