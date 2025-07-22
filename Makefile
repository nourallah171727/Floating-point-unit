# Source files
C_SRCS   := $(wildcard src/*.c)
CPP_SRCS := $(wildcard src/*.cpp)

# Executable name
TARGET = project

# SystemC path (must be set in the environment)
SCPATH = $(SYSTEMC_HOME)

# Compiler commands
CC  := $(shell command -v gcc || command -v clang)
CXX := $(shell command -v g++ || command -v clang++)

# Compiler flags
CFLAGS   = -std=c17 -Iinclude
CXXFLAGS = -std=c++14 -Iinclude -I"$(SCPATH)/include"

# Linker flags
LDFLAGS  = -L"$(SCPATH)/lib" -lsystemc -lm

# Add rpath for SystemC libraries at runtime (non-MacOS)
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S), Darwin)
    LDFLAGS += -Wl,-rpath="$(SCPATH)/lib"
endif

# Default target
.DEFAULT_GOAL := project

# Phony targets
.PHONY: project all clean debug release

# Build the project
project:
	$(CC)  $(CFLAGS)   -c $(C_SRCS)
	$(CXX) $(CXXFLAGS) -c $(CPP_SRCS)
	$(CXX) *.o $(LDFLAGS) -o $(TARGET)

# Alias for "make"
all: project

# Debug version
debug:
	$(CXX) $(CXXFLAGS) -g $(CFLAGS) $(C_SRCS) $(CPP_SRCS) $(LDFLAGS) -o $(TARGET)

# Optimized release version
release:
	$(CXX) $(CXXFLAGS) -O2 $(CFLAGS) $(C_SRCS) $(CPP_SRCS) $(LDFLAGS) -o $(TARGET)

# Cleanup
clean:
	rm *.o
	rm -f $(TARGET)
