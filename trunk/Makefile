CC = gcc

.PHONY: all
all: tuio_tcp_proxy tuio_dump toxy_dump toxy toxy_gtk

tuio_tcp_proxy: tuio_tcp_proxy.c
	gcc -llo $< -o $@

toxy: toxy.o libtoxy.o 
	gcc -llo $^ -o $@

toxy_dump: toxy_dump.c
	gcc -llo $< -o $@

tuio_dump: tuio_dump.c
	gcc -llo $< -o $@

toxy_gtk: toxy_gtk.c
	${CC} `pkg-config --cflags --libs gtk+-2.0` -o $@ $<

%.o : %.c
	gcc -c $< 

.PHONY clean:
	rm -f tuio_dump toxy_dump tuio_tcp_proxy toxy
	rm -f *.o
