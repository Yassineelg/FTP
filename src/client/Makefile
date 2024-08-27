CXX := g++

SRC_DIR := src
INCLUDE_DIR := include
LIB_DIR := lib
BIN_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.cpp)

OBJS := $(SRCS:.cpp=.o)

ifeq ($(OS),Windows_NT)
    TARGET := $(BIN_DIR)/ftp.exe
else
    TARGET := $(BIN_DIR)/ftp
endif

CXXFLAGS := -I$(INCLUDE_DIR) -L$(LIB_DIR)

all: $(TARGET) clean

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

re: clean all

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)