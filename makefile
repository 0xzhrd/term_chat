# term_chat — portable hardened build (Linux + macOS)

CC      = cc
UNAME_S := $(shell uname -s)

# Shared warnings + standards
CFLAGS_BASE = -std=c11 -Wall -Wextra -Wconversion -Wshadow -Wformat=2 \
              -Wformat-security -Wnull-dereference -Wstrict-prototypes \
              -pthread -O2 -fstack-protector-strong

# Platform-specific compile + link hardening
ifeq ($(UNAME_S),Darwin)
    # macOS / Apple clang + ld64
    # - No -z,relro / -z,now / -z,noexecstack (GNU ld only); ld64 rejects them.
    # - PIE is the default on macOS; -Wl,-bind_at_load is the closest -z,now analogue.
    # - _DEFAULT_SOURCE is a glibc macro (no-op on macOS but harmless).
    CFLAGS  = $(CFLAGS_BASE) -D_DARWIN_C_SOURCE -D_FORTIFY_SOURCE=2
    LDFLAGS = -pthread -Wl,-bind_at_load
else
    # Linux / GCC + GNU ld
    CFLAGS  = $(CFLAGS_BASE) -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
              -D_FORTIFY_SOURCE=2
    LDFLAGS = -pthread -Wl,-z,relro,-z,now -Wl,-z,noexecstack
endif

BUILD_DIR   = build
SRC_DIR     = src
INCLUDE_DIR = include

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c \
          $(SRC_DIR)/transfers.c $(SRC_DIR)/terminal.c $(SRC_DIR)/logic.c
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET  = chat

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/headers.h
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean