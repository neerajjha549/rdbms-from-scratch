# Compiler
CXX = g++

# Compiler flags
# -g: adds debugging information
# -Wall: enables all compiler's warning messages
# Updated to C++14 to use std::make_unique
CXXFLAGS = -g -Wall -std=c++14

# The target executable
TARGET = db

# Source files
SRCS = main.cpp pager.cpp btree.cpp table.cpp tokenizer.cpp parser.cpp query_planner.cpp vm.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	@mkdir -p tmp
	TEMP=./tmp $(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.cpp
	@mkdir -p tmp
	TEMP=./tmp $(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf tmp
