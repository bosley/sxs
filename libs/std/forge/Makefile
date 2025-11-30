CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg -I$(HOME)/.sxs/include
LDFLAGS = -shared -L$(HOME)/.sxs/lib -lpkg_slp

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
