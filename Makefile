all: rrobin

rrobin: rrobin.o queue.o
	gcc -o rrobin rrobin.o queue.o

%.o: %.c $(wildcard %.h)
	gcc -c $<

clean:
	rm -rf rrobin *.o
