CXX := g++

SRC_DIR := src
INCLUDE_DIR := include
LIB_DIR := lib
BIN_DIR := bin

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)

ifeq ($(OS),Windows_NT)
    TARGET := $(BIN_DIR)/ftp.exe
else
    TARGET := $(BIN_DIR)/ftp
endif

CXXFLAGS := -I$(INCLUDE_DIR) -std=c++20 -Wall -Wextra -Werror -pedantic

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJS)

fclean:
	rm -f $(OBJS)
	rm -f $(TARGET)

re: fclean all clean

.PHONY: all clean fclean re
