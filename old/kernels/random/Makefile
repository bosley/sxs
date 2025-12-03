CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I../../root -I../../integrants -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -L../../build/integrants -lpkg_slp -lpkg_random

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_random.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_random.so
endif

all: $(TARGET)

$(TARGET): random.cpp random.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ random.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

