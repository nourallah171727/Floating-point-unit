# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

C_SRCS = src/main.c
CPP_SRCS = src/modules.cpp
HEADERS = include/modules.hpp

C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

TARGET = project
SCPATH = $(SYSTEMC_HOME)

CFLAGS = -std=c17 -Iinclude
CXXFLAGS = -std=c++14 -Iinclude -I"$(SCPATH)/include"
LDFLAGS = -L"$(SCPATH)/lib" -lsystemc -lm

UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S), Darwin)
    LDFLAGS += -Wl,-rpath="$(SCPATH)/lib"
endif

CXX := $(shell command -v g++ || command -v clang++)
CC  := $(shell command -v gcc || command -v clang)

# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

.DEFAULT_GOAL := $(TARGET)
.PHONY: all debug release clean

all: $(TARGET)

# Debug build
project: CXXFLAGS += -g
project: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# clean up
clean:
	rm -f $(TARGET)
	rm -rf *.o

