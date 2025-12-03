CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_example.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_example.so
endif

all: $(TARGET)

$(TARGET): example.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ example.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
