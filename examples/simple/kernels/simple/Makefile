CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I$(SXS_HOME)/include
LDFLAGS = -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_simple.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_simple.so
endif

all: $(TARGET)

$(TARGET): simple.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ simple.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
