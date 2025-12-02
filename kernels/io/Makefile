CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I../../root -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_io.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_io.so
endif

all: $(TARGET)

$(TARGET): io.cpp io.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ io.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

