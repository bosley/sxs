CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_alu.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_alu.so
endif

all: $(TARGET)

$(TARGET): alu.cpp alu.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ alu.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

