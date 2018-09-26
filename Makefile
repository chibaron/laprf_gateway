
CC            = gcc
CFLAGS        = -Wall -I./
LDFLAGS       = 
LIBS          = -lrt -lpthread
OBJS          = main.o laprf.o monitor.o
PROGRAM       = laprf_gw

all:	$(PROGRAM)

$(PROGRAM):	$(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGRAM)

clean:;	rm -f *.o *~ $(PROGRAM)

install:    $(PROGRAM)
    install -s $(PROGRAM) /usr/local/bin



