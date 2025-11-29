CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared
LIBS = ../../build/pkg/libpkg_slp.a ../../build/pkg/libpkg_random.a

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_random.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_random.so
endif

all: $(TARGET)

$(TARGET): random.cpp random.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ random.cpp $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

