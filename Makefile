CC = cc
	
all:	fat.c fat.h
	$(CC) fat.c fat.h -o fatout

clean:
	rm -f *.o

