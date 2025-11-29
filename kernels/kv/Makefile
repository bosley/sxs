CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared -L../../build/pkg -L/opt/homebrew/opt/rocksdb/lib -lpkg_kvds -lpkg_slp -lrocksdb

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

