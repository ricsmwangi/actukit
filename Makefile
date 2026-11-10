CXX      ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -pedantic
CFLAGS   ?= -O2 -Wall -Wextra

all: src/actukit-mack

src/actukit-mack: src/actukit_mack.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f src/actukit src/actukit-sum src/actukit-mack

.PHONY: all clean
