CC=gcc

#### BEGIN ARCH DEPENDANT SECTION ####
# For Linux
#LDFLAGS=-L/usr/local/lib -L.
#LIBS=-lsp -lskiplist
#CFLAGS=-g -D__USE_LARGEFILE64 -Wall
#MAIN=spreadlogd.o

# For FreeBSD
#LDFLAGS=-L/usr/local/lib -L. `perl -MExtUtils::Embed -e ldopts`
#LIBS=-lsp -lskiplist -lgnuregex
#INCLUDES=-I/usr/local/include `perl -MExtUtils::Embed -e ccopts` -DPERL
#CFLAGS=-g -Wall -DHAVE_GNUREGEX_H
#MAIN=spreadlogd-kqueue.o
#PERL_OBJS=perl.o perlxsi.o

#MACOSX
#LDFLAGS=-L/usr/local/lib -L.
#LIBS=-lsp -lskiplist
#CFLAGS=-g -Wall
#MAIN=spreadlogd.o

# For Solaris
#LIBS=-lsp -lskiplist -lnsl -lsocket -lucb 
#LDFLAGS=-L/usr/local/lib -L/usr/ucblib -R/usr/ucblib -L.
#BSDINCLUDES=-I/usr/ucbinclude
#CFLAGS=-g -D__USE_LARGEFILE64 -Wall
#MAIN=spreadlogd.o
#### END ARCH DEPENDANT SECTION ####

YACC=bison -y
LEX=flex
AR=ar
RANLIB=ranlib

OBJS=lex.sld_.o y.tab.o config.o hash.o timefuncs.o $(PERL_OBJS)
LSLOBJS=skiplist.o

all:	spreadlogd

parser:	lex.sld_.c y.tab.c y.tab.h

lex.sld_.c:	config_gram.l
	$(LEX) -Psld_ config_gram.l
y.tab.c y.tab.h:	config_gram.y
	$(YACC) -p sld_ -d config_gram.y
lex.sld_.o: lex.sld_.c y.tab.h
	$(CC) $(CFLAGS) $(INCLUDES) -c lex.sld_.c
y.tab.o: y.tab.c config.h
	$(CC) $(CFLAGS) $(INCLUDES) -c y.tab.c

perlxsi.c:
	perl -MExtUtils::Embed -e xsinit -- -o perlxsi.c

test.o:	test.c
	$(CC) $(CFLAGS) $(BSDINCLUDES) $(INCLUDES) -c $<

config.o:	config.c
	$(CC) $(CFLAGS) $(BSDINCLUDES) $(INCLUDES) -c $<
hash.o:		hash.c
	$(CC) $(CFLAGS) $(BSDINCLUDES) $(INCLUDES) -c $<
.c.o:	$*.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

libskiplist.a:	$(LSLOBJS)
	$(AR) cq libskiplist.a $(LSLOBJS)
	$(RANLIB) libskiplist.a

spreadlogd:	libskiplist.a $(MAIN) $(OBJS)
	@if test -z "$(MAIN)"; then \
		echo "Uncomment the correct section the makefile"; \
		exit; \
	else \
		$(CC) -g -o $@ $(MAIN) $(OBJS) $(LDFLAGS) $(LIBS); \
	fi

clean:
	rm -f *~ *.o spreadlogd libskiplist.a y.tab.h y.tab.c lex.sld_.c
