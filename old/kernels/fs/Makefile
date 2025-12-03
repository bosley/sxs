CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I../../root -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_fs.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_fs.so
endif

all: $(TARGET)

$(TARGET): fs.cpp fs.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ fs.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

