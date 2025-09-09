# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g3 -I includes

# Project name
NAME := webserver

# Directories
SRC_DIR := srcs
OBJ_DIR := obj

# Source and object files
SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/methods.cpp $(SRC_DIR)/requestHandler.cpp $(SRC_DIR)/conf.cpp $(SRC_DIR)/request.cpp $(SRC_DIR)/utils.cpp $(SRC_DIR)/httpException.cpp
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Default rule
all: $(NAME)

# Link
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create obj directory if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Cleaning rules
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
