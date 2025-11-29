CC = gcc
CFLAGS = -g -Wall -pedantic
LDFLAGS = -lmicrohttpd

BIN = search
SRC = main.c config.c

$(BIN): $(SRC)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(BIN)

