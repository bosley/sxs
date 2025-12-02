CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I../../root -I../../integrants -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -L../../build/integrants -L/opt/homebrew/opt/rocksdb/lib -lpkg_slp -lpkg_kvds -lrocksdb

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_kv.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_kv.so
endif

all: $(TARGET)

$(TARGET): kv.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ kv.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

