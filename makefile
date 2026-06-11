CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c $(SRC_DIR)/transfers.c $(SRC_DIR)/terminal.c $(SRC_DIR)/logic.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/server.o $(BUILD_DIR)/client.o $(BUILD_DIR)/transfers.o $(BUILD_DIR)/terminal.o $(BUILD_DIR)/logic.o
TARGET = chat

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/server.o: $(SRC_DIR)/server.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/client.o: $(SRC_DIR)/client.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/transfers.o: $(SRC_DIR)/transfers.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/terminal.o: $(SRC_DIR)/terminal.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/logic.o: $(SRC_DIR)/logic.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
