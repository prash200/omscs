CC=gcc
CFLAGS=-Wall -I. -lvirt -std=c99

release: vcpu_scheduler.c
	$(CC) -o ./vcpu_scheduler ./vcpu_scheduler.c $(CFLAGS)

debug:
	$(CC) -o ./vcpu_scheduler ./vcpu_scheduler.c $(CFLAGS) -DDEBUG
	
clean:
	rm -f ./vcpu_scheduler
