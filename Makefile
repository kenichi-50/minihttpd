CC = gcc

CFLAGS = -Wall -Wextra -Werror -Iinclude

SRC = src/main.c src/server.c src/http.c src/files.c src/mime.c

TARGET = minihttpd

all: $(TARGET)

$(TARGET): $(SRC) include/http.h include/server.h include/files.h include/mime.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean