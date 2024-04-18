SRC_DIR := src
OBJ_DIR := obj

CFLAGS := -Wall -Wextra -pedantic

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(subst $(SRC_DIR),$(OBJ_DIR),$(SOURCES:.c=.o))

.PHONY: all
all: $(OBJ_DIR)/simple-dhcp

$(OBJ_DIR):
	mkdir -p -- "$(OBJ_DIR)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/simple-dhcp: $(OBJECTS) | $(OBJ_DIR)
	cc -o $@ $(OBJECTS)
