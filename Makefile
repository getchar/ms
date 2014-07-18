CC = cc

all: ms

ms: src/main.c src/parsec.h src/msio.h src/msdefs.h src/msflags.h
	$(CC) -o ms src/main.c -lm

install:
	cp ms /usr/bin
	cp man/ms.6 /usr/share/man/man6

clean:
	/bin/rm -f ms
	/bin/rm -f src/*~
	/bin/rm -f man/*~
