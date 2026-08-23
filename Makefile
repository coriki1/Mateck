# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror -Wextra -Iinclude

# Források
SRC = src/types.cpp src/node.cpp src/token.cpp src/lexer.cpp main.cpp

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
	del /F /Q src\*.o mateck.exe

# Újrafordítás teljesen
rebuild: clean all