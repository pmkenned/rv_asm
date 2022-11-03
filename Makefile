CC = gcc
CPPFLAGS=
CFLAGS = -Wall -Wextra -ggdb -MMD
LDFLAGS =
LDLIBS =

# TODO: debug, release
# TODO: test
# NOTE: $(info SRC is $(SRC))

BUILD_DIR = ./build
SRC_DIR = ./src
TARGET = a.out

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)

ASM ?= insert_sort

.PHONY: all run test clean

all: $(BUILD_DIR)/$(TARGET)

run: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET) asm/$(ASM).s

test: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET) asm/$(ASM).s > out.hex
	diff out.hex hex/$(ASM).hex
	rm -f out.hex

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
