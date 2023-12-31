BINDIR =	/usr/local/bin
MANDIR =	/usr/local/man/man1
CC =		cc
CFLAGS =	-O -I/usr/include/libxml2 -I/usr/local/include/libxml2 -I/usr/local/include -ansi -pedantic -U__STRICT_ANSI__ -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long
LDFLAGS =	-s -L/usr/local/lib
LIBS =		-lxml2

all:		xml2c

xml2c:		xml2c.c
	$(CC) $(CFLAGS) xml2c.c $(LDFLAGS) $(LIBS) -o xml2c


install:	all
	rm -f $(BINDIR)/xml2c
	cp xml2c $(BINDIR)
	rm -f $(MANDIR)/xml2c.1
	cp xml2c.1 $(MANDIR)


clean:
	rm -f xml2c *.o core core.* *.core
