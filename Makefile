CC = gcc
CFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g  -D_REENTRANT -Wall -L /users/cse533/Stevens/unpv13e/ -o 
LIBS = -lunp -lpthread
CLEANFILES = client server odr odr_test  *~ *.o ud_* 


all:	client odr

otest: odr odrt

client : client.c
	$(CC) $(CFLAGS) client client.c $(LIBS)

prhw: prhwaddrs.c get_hw_addrs.c
	$(CC) $(CFLAGS) prhw prhwaddrs.c get_hw_addrs.c $(LIBS)

unixdg : 
	$(CC) $(CFLAGS) dg unixdgcli01.c  $(LIBS)

odrt: odr_test.c get_hw_addrs.c
	$(CC) $(CFLAGS) odrt odr_test.c get_hw_addrs.c $(LIBS)

odr : odr.c get_hw_addrs.c
	$(CC) $(CFLAGS) odr odr.c  get_hw_addrs.c $(LIBS)

clean:
	rm -f $(CLEANFILES)
