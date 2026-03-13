# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

# Források
SRC = src/node.cpp src/token.cpp main.cpp

# Objektumok
OBJ = $(SRC:.cpp=.o)

# Célnév
TARGET = mateck

# Alapértelmezett cél
all: $(TARGET)

# Linkelés
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Fordítás .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tisztítás
clean:
	rm -f $(OBJ) $(TARGET)

# Újrafordítás teljesen
rebuild: clean all