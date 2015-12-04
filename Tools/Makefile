
CC=g++
CFLAGS=-g

BINS=yaffaLoad

all:	$(BINS)

yaffaLoad.o:	yaffaLoad.c
	$(CC) -c $(CFLAGS) $? -o $@

yaffaLoad:	yaffaLoad.o serialComms.o serialComms.h
	$(CC) $(CFLAGS) yaffaLoad.o serialComms.o -o $@


serialComms.o:	serialComms.c
	$(CC) -c $(CLAGS) $? -o $@

clean:
	rm -f $(BINS) *.o cscope.out

backup:	clean
	./backup.sh


