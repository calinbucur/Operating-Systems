CFLAGS = -Wall -Wextra -g

.PHONY: build clean

build: libscheduler

libscheduler:
	gcc -fPIC $(CFLAGS) -o so_scheduler.o -c so_scheduler.c -lpthread
	gcc -fPIC $(CFLAGS) -o queue.o -c queue.c -lpthread
	gcc -fPIC $(CFLAGS) -shared -o libscheduler.so so_scheduler.o queue.o

clean:
	rm -f so_scheduler.o libscheduler.so