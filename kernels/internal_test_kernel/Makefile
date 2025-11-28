CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_test.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_test.so
endif

all: $(TARGET)

$(TARGET): test.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

