CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I/opt/homebrew/opt/ncurses/include
LDFLAGS = -L/opt/homebrew/opt/sqlite/lib -L/opt/homebrew/opt/ncurses/lib -lsqlite3 -lncurses -lcurl
CPPFLAGS = -I/opt/homebrew/opt/sqlite/include -I/opt/homebrew/opt/ncurses/include

SRCS = src/main.cpp src/radix_tree.cpp src/database.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = radix_dict

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
