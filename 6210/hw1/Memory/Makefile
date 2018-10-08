CC=gcc
CFLAGS=-Wall -I. -lvirt -std=c99

release: memory_coordinator.c
	$(CC) -o ./memory_coordinator ./memory_coordinator.c $(CFLAGS)

debug: memory_coordinator.c
	$(CC) -o ./memory_coordinator ./memory_coordinator.c $(CFLAGS) -DDEBUG
	
clean:
	rm -f ./memory_coordinator
