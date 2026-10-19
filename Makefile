# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Werror -Wextra -Iinclude -MMD -MP

# Források (a program és a tesztek közösen használják)
LIB_SRC = src/types.cpp src/token.cpp src/lexer.cpp \
          src/node_number.cpp src/node_var.cpp src/node_constant.cpp \
          src/node_binaryop.cpp src/node_unaryop.cpp src/node_func.cpp \
          src/simplify.cpp src/compare.cpp src/format.cpp src/parser.cpp src/repl.cpp

# Tesztek (GoogleTest, telepítés: sudo dnf install gtest-devel)
TEST_SRC = tests/test_lexer.cpp tests/test_nodes.cpp tests/test_simplify.cpp tests/test_parser.cpp \
           tests/test_substitute.cpp tests/test_repl.cpp tests/test_compare.cpp tests/test_format.cpp

GTEST_CFLAGS = $(shell pkg-config --cflags gtest_main)
GTEST_LIBS   = $(shell pkg-config --libs gtest_main)

# Objektumok
LIB_OBJ  = $(LIB_SRC:.cpp=.o)
MAIN_OBJ = main.o
TEST_OBJ = $(TEST_SRC:.cpp=.o)
DEP      = $(LIB_OBJ:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

# Célnevek
TARGET      = mateck
TEST_TARGET = mateck_tests

# Alapértelmezett cél
all: $(TARGET)

# Linkelés
$(TARGET): $(LIB_OBJ) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET): $(LIB_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LIBS)

# Tesztek fordítása és futtatása
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Fordítás .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tests/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -c $< -o $@

# Tisztítás
clean:
	rm -f $(LIB_OBJ) $(MAIN_OBJ) $(TEST_OBJ) $(DEP) $(TARGET) $(TEST_TARGET)

# Újrafordítás teljesen
rebuild: clean all

.PHONY: all test clean rebuild

# Header-függőségek (-MMD generálja)
-include $(DEP)
