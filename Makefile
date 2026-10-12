# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror -Wextra -Iinclude -MMD -MP

# Források
SRC = src/types.cpp src/token.cpp src/lexer.cpp \
      src/node_number.cpp src/node_var.cpp \
      src/node_binaryop.cpp src/node_unaryop.cpp src/node_func.cpp \
      main.cpp
	  
# Objektumok
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)

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
	rm -f $(OBJ) $(DEP) $(TARGET)

# Újrafordítás teljesen
rebuild: clean all

.PHONY: all clean rebuild

# Header-függőségek (-MMD generálja)
-include $(DEP)
