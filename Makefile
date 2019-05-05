all: WTF WTFserver

WTF: WTF.c
	gcc -o WTF WTF.c -lssl -lcrypto

WTFserver: WTFserver.c
	gcc -lpthread -o WTFserver WTFserver.c -lssl -lcrypto

clean:
	rm -rf WTF
	rm -rf WTFserver
