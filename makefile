CC=gcc
CFLAGS=-g -Wall -D__USE_LARGEFILE64
INCLUDES=-I/usr/local/include
LDFLAGS=-L/usr/local/lib -L.
LIBS=-lsp -lskiplist

YACC=bison -y
LEX=flex

OBJS=spreadlogd.o lex.yy.o y.tab.o config.o
LSLOBJS=skiplist.o

all:	spreadlogd

parser:	lex.yy.c y.tab.c y.tab.h

lex.yy.c:	config_gram.l
	$(LEX) config_gram.l
y.tab.c y.tab.h:	config_gram.y
	$(YACC) -d config_gram.y
lex.yy.o: lex.yy.c y.tab.h
	$(CC) $(CFLAGS) $(INCLUDES) -c lex.yy.c
y.tab.o: y.tab.c config.h
	$(CC) $(CFLAGS) $(INCLUDES) -c y.tab.c

.c.o:	$*.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

libskiplist.a:	$(LSLOBJS)
	$(AR) cq libskiplist.a $(LSLOBJS)

spreadlogd:	libskiplist.a $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f *~ *.o spreadlogd libskiplist.a y.tab.h y.tab.c lex.yy.c
