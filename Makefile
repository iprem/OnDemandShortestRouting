CC = gcc
CFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g  -D_REENTRANT -Wall -L /users/cse533/Stevens/unpv13e/ -o 
LIBS = -lunp -lpthread
CLEANFILES = client server odr *~ *.o ud_*


all:	client odr

client : client.c
	$(CC) $(CFLAGS) client client.c $(LIBS)

unixdg : 
	$(CC) $(CFLAGS) dg unixdgcli01.c  $(LIBS)

odr : odr.c
	$(CC) $(CFLAGS) odr odr.c $(LIBS)

clean:
	rm -f $(CLEANFILES)
