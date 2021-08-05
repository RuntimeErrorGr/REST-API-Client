CC=g++
CFLAGS=-I.

client: client.cpp helpers.cpp buffer.cpp
	$(CC) -o client client.cpp buffer.cpp helpers.cpp -Wall -Wextra

run: client
	./client

clean:
	rm -f *.o client
