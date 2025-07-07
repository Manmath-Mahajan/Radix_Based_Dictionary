CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I/opt/homebrew/opt/curl/include -I/opt/homebrew/opt/sqlite/include -I/opt/homebrew/opt/openssl@3/include -I/opt/homebrew/opt/ncurses/include
LDFLAGS = -L/opt/homebrew/opt/curl/lib -L/opt/homebrew/opt/sqlite/lib -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/ncurses/lib -lcurl -lsqlite3 -lcrypto -lssl -lncurses

SRCS = src/main.cpp src/radix_tree.cpp src/database.cpp src/user_manager.cpp
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
