# Source files
C_SRCS   := $(wildcard src/*.c)
CPP_SRCS := $(wildcard src/*.cpp)
# As modules.hpp and structs.h are included in main.c and modules.cpp
# they will also be automatically compiled

# Object files derived from source files
C_OBJS   := $(C_SRCS:.c=.o)
CPP_OBJS := $(CPP_SRCS:.cpp=.o)

# Final executable name
TARGET = project

# SystemC installation path (must be set externally)
SCPATH = $(SYSTEMC_HOME)

# Compiler commands
CC  := $(shell command -v gcc || command -v clang)
CXX := $(shell command -v g++ || command -v clang++)

# Compiler flags
CFLAGS = -std=c17 -Iinclude
CXXFLAGS = -std=c++14 -Iinclude -I"$(SCPATH)/include"

# Linker flags including SystemC library and math library
LDFLAGS = -L"$(SCPATH)/lib" -lsystemc -lm

# Add rpath linker flag on non-Darwin systems to find SystemC libs at runtime
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S), Darwin)
    LDFLAGS += -Wl,-rpath="$(SCPATH)/lib"
endif

# Default make target
.DEFAULT_GOAL := $(TARGET)

# Declare phony targets to avoid conflicts with files of the same name
.PHONY: all debug release clean

# Default build target
all: $(TARGET)

# Debug build with debug symbols enabled
debug: CXXFLAGS += -g
debug: $(TARGET)

# Release build with optimization enabled
release: CXXFLAGS += -O2
release: $(TARGET)

# Compile C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link all object files into the final executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# Remove generated files
clean:
	rm -f $(TARGET) $(C_OBJS) $(CPP_OBJS)
