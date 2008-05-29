.PHONY: all
all: tuio_tcp_proxy tuio_dump toxy_dump toxy

tuio_tcp_proxy: tuio_tcp_proxy.c
	gcc -llo $< -o $@

toxy: toxy.c
	gcc -llo $< -o $@

toxy_dump: toxy_dump.c
	gcc -llo $< -o $@

tuio_dump: tuio_dump.c
	gcc -llo $< -o $@

.PHONY clean:
	rm -f tuio_dump toxy_dump tuio_tcp_proxy toxy
