CFLAGS := -Wall -pedantic
EXES := putsmain putsmainsub brkmain
OBJS := puts_noarg.o puts_alternative.o puts_data.o
TARGETS := $(EXES) $(OBJS)


all: $(TARGETS)


putsmain: putsmain.c
	gcc $(CFLAGS) $^ -o $@

putsmainsub: putsmainsub.c
	gcc $(CFLAGS) $^ -o $@

.c.o:
	gcc $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -f $(TARGETS)
