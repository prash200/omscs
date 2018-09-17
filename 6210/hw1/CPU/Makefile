CC=gcc
CFLAGS=-Wall -I. -lvirt -std=c99

make: vcpu_scheduler.c
	$(CC) -o ./vcpu_scheduler ./vcpu_scheduler.c $(CFLAGS)

clean:
	rm -f ./vcpu_scheduler
