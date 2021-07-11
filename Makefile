CC = gcc
CPPFLAGS=
CFLAGS = -Wall -Wextra -MMD
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

.PHONY: all run test clean

all: $(BUILD_DIR)/$(TARGET)

run: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET) $(PROG)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
