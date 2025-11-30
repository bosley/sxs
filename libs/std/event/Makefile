CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared -L../../build/pkg -lpkg_events -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_event.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_event.so
endif

all: $(TARGET)

$(TARGET): event.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ event.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

