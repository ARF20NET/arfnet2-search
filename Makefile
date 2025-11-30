CC = gcc
CFLAGS = -g -Wall -pedantic
LDFLAGS = -lmicrohttpd -lmagic

BIN = search
SRC = main.c config.c index.c

$(BIN): $(SRC)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(BIN)

