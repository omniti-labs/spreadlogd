CC=gcc

#### BEGIN ARCH DEPENDANT SECTION ####
# For Linux
#LDFLAGS=-L/usr/local/lib -Wl,-rpath /usr/local/lib -L. -L/opt/spread/lib -Wl,-rpath /opt/spread/lib `perl -MExtUtils::Embed -e ldopts`
#LIBS=-lspread -levent
#CFLAGS=-g -D__USE_LARGEFILE64 -Wall
#MAIN=spreadlogd.o

# For FreeBSD
#LDFLAGS=-L/usr/local/lib -L. `perl -MExtUtils::Embed -e ldopts`
#LIBS=-lspread -lgnuregex -levent
#CFLAGS=-g -Wall -DHAVE_GNUREGEX_H
#MAIN=spreadlogd-kqueue.o

#MACOSX
LDFLAGS=-L/usr/local/lib -L. `perl -MExtUtils::Embed -e ldopts`
LIBS=-lspread -levent
CFLAGS=-g -D__USE_LARGEFILE64 -Wall
MAIN=spreadlogd.o

# For Solaris
#LIBS=-lspread -lnsl -lsocket -lucb -levent
#LDFLAGS=-L/usr/local/lib -L/usr/ucblib -R/usr/ucblib -L.
#BSDINCLUDES=-I/usr/ucbinclude
#CFLAGS=-g -D__USE_LARGEFILE64 -Wall
#MAIN=spreadlogd.o

PERL_OBJS=perl.o perlxsi.o
INCLUDES=-I/usr/local/include -I/opt/spread/include `perl -MExtUtils::Embed -e ccopts` -DPERL 


#### END ARCH DEPENDANT SECTION ####

YACC=bison -y
LEX=flex
AR=ar
RANLIB=ranlib

OBJS=lex.sld_.o y.tab.o config.o hash.o timefuncs.o module.o nethelp.o \
	$(PERL_OBJS)
LSLOBJS=skiplist.o echash.o

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

spreadlogd:	libskiplist.a $(MAIN) $(OBJS) $(LSLOBJS)
	@if test -z "$(MAIN)"; then \
		echo "Uncomment the correct section the makefile"; \
		exit; \
	else \
		$(CC) -g -o $@ $(MAIN) $(OBJS) $(LSLOBJS) $(LDFLAGS) $(LIBS); \
	fi

clean:
	rm -f *~ *.o spreadlogd libskiplist.a y.tab.h y.tab.c lex.sld_.c \
		perlxsi.c
