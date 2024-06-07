.PHONY: all run clean

CXX = clang++
CXXFLAGS = -std=c++23 -g -DDEBUG -Wno-experimental-header-units -fprebuilt-module-path=build $(addsuffix .pcm,$(addprefix -fmodule-file=build/,print cstdint))

all: build/test

build/test: src/main/test.cpp build/cc.o
	$(CXX) $(CXXFLAGS) $^ -o $@

build/cc.pcm:
build/cc.o: src/cc/cc.cppm
	$(CXX) $(CXXFLAGS) -fmodule-output -c $^ -o $@

run: build/test
	./build/test

clean:
	rm -f build/test build/cc.o build/cc.pcm
