# Compiler
CXX = g++

# Compiler flags
# -g: adds debugging information
# -Wall: enables all compiler's warning messages
CXXFLAGS = -g -Wall -std=c++11

# The target executable
TARGET = db

# Source files
SRCS = main.cpp pager.cpp btree.cpp table.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link the object files to create the executable
# We add "TEMP=./tmp" here as well to fix permission errors during the linking stage on Windows.
$(TARGET): $(OBJS)
	@mkdir -p tmp
	TEMP=./tmp $(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
# We create a local ./tmp directory and set the TEMP environment variable for the g++
# command. This forces the compiler to use our local, writable directory for its
# temporary files, avoiding permission errors on Windows with MinGW.
%.o: %.cpp
	@mkdir -p tmp
	TEMP=./tmp $(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf tmp
