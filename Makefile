CFLAGS=-std=gnu99 -Wall -O2
chavrprog: main.c ch341a.c ch341a.h
	

	gcc $(CFLAGS)  ch341a.c main.c ihex_copy.c ihex_parse.c ihex_record.c -o chavrprog -lusb-1.0

clean:
	rm *.o chavrprog -f
.PHONY: clean
