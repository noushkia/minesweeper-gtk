CC=gcc -Wall -w
CFLAGS=`pkg-config gtk+-2.0 --cflags`
LDFLAGS=`pkg-config gtk+-2.0 --libs`
EXEC=minesweeper

compile: *.c
	$(CC) $(CFLAGS) $^ -o $(EXEC)  $(LDFLAGS)
clean:
	rm -rf *.o $(EXEC)
