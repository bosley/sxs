CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I../../root -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_forge.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_forge.so
endif

all: $(TARGET)

$(TARGET): forge.cpp forge.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ forge.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
