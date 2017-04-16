all: proxy.c thread.c proc.c csapp.c csapp.h
	gcc proxy.c csapp.c -o proxy -lpthread
	gcc proc.c csapp.c csapp.h -o proc -lpthread
	gcc thread.c csapp.c csapp.h -o thread -lpthread

debug:
	gcc proxy.c csapp.c -o proxy -ggdb -lpthread
	gcc proc.c csapp.c csapp.h -o proc -ggdb -lpthread
	gcc thread.c csapp.c csapp.h -o thread -ggdb -lpthread

log:
	cat log.txt

clean:
	rm proxy
	rm proc
	rm thread
	rm log.txt
