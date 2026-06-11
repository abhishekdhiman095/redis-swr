# Redis SWR Module Makefile

MODULE_NAME=swr.so

SRC_DIR=src
SRC=$(SRC_DIR)/swr.c

REDIS_SRC=redis/src

CC=gcc

CFLAGS=-fPIC -Wall -Wextra -O2
LDFLAGS=-shared

TARGET=$(MODULE_NAME)


all: $(TARGET)


$(TARGET): $(SRC)
	$(CC) $(CFLAGS) \
		-I$(REDIS_SRC) \
		$(LDFLAGS) \
		-o $@ $^


clean:
	rm -f $(TARGET)


.PHONY: all clean