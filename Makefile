
CFLAGS=-std=gnu99 -Wall -O2
chavrprog: 
	

	gcc $(CFLAGS)  ch341a.c main.c chavrprog.c config.c  ihex_copy.c ihex_parse.c ihex_record.c -o chavrprog -lusb-1.0 -I ./

clean:
	rm *.o chavrprog -f
.PHONY: clean
