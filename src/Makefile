#
#
# Warning: make depend overwrites this file.

.PHONY: depend clean backup setup

MAIN=sorttest

MINIBASE=..

CC=g++

CFLAGS= -DUNIX -w -g

INCLUDES = -I. -I$(MINIBASE)/include

#LFLAGS=-L${MINIBASE}/lib -liberty -lwithout -lm

SRCS =  main.C \
	scan.C page.C sort.C db.C system_defs.C new_error.C \
	#buf.C heapfile.C hfpage.C \

$(info $(SRCS))

OBJS = $(SRCS:.C=.o)

$(info $(OBJS))

$(MAIN):  $(OBJS)
	 $(CC) $(CFLAGS) $(INCLUDES) $(OBJS) buf.o heapfile.o hfpage.o -no-pie -o $(MAIN) $(LFLAGS) 

%.o : %.C
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

depend: $(SRCS)
	makedepend $(INCLUDES) $^

clean:
	mv buf.o buf.obak
	mv heapfile.o heapfile.obak
	mv hfpage.o hfpage.obak
	rm -f *.o *~ $(MAIN)
	mv buf.obak buf.o
	mv heapfile.obak heapfile.o
	mv hfpage.obak hfpage.o
backup:
	-mkdir bak
	cp Makefile *.cpp *.h bak

# Grab the sources for a user who has only the makefile
setup:
	/bin/cp -i $(MINIBASE)/src/*.h .
	/bin/cp -i $(MINIBASE)/src/*.cpp .
	/bin/cp -i $(MINIBASE)/src/*sample* .

# DO NOT DELETE THIS LINE -- make depend needs it 
