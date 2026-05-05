NAME := build/minishell
CC := cc
CFLAGS := -Wall -Wextra -Werror -g3 -Wno-sign-compare
INCLUDES := -Iinclude -I libc_mini/include
BUILD_DIR := build

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))
MINISRC := $(wildcard libc_mini/src/stdio/*.c) $(wildcard libc_mini/src/syscalls/*.c)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) $(MINISRC) -o $(NAME)


$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: all
	./$(NAME)

clean:
	rm -rf $(BUILD_DIR)/*

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all test clean fclean re
