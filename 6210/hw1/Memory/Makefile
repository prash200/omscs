CC=gcc
CFLAGS=-Wall -I. -lvirt -std=c99

release: Memory_coordinator.c
	$(CC) -o ./Memory_coordinator ./Memory_coordinator.c $(CFLAGS)

debug: Memory_coordinator.c
	$(CC) -o ./Memory_coordinator ./Memory_coordinator.c $(CFLAGS) -DDEBUG
	
clean:
	rm -f ./Memory_coordinator
