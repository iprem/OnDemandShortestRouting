CC = gcc
CFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g  -D_REENTRANT -Wall -L /users/cse533/Stevens/unpv13e/ -o 
LIBS = -lunp -lpthread -lrt
CLEANFILES = client server odr odr_test odrt   *~ *.o ud_* 


all:	client odr server

server:
	$(CC) $(CFLAGS) server server.c get_hw_addrs.c sockaddr_util.c send.c recv.c a3_utils.c $(LIBS)

otest: odr odrt

client : client.c get_hw_addrs.c sockaddr_util.c send.c a3_utils.c
	$(CC) $(CFLAGS) client client.c get_hw_addrs.c sockaddr_util.c send.c recv.c a3_utils.c $(LIBS)

prhw: prhwaddrs.c get_hw_addrs.c
	$(CC) $(CFLAGS) prhw prhwaddrs.c get_hw_addrs.c $(LIBS)

odrt: odr_test.c get_hw_addrs.c
	$(CC) $(CFLAGS) odrt odr_test.c get_hw_addrs.c $(LIBS)

odr : odr.c get_hw_addrs.c sockaddr_util.c a3_utils.c
	$(CC) $(CFLAGS) odr odr.c  get_hw_addrs.c sockaddr_util.c a3_utils.c $(LIBS)

clean:
	rm -f $(CLEANFILES)
