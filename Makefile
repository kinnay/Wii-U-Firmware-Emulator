
CXX ?= g++

FLAGS = -std=c++14 -O2 -g -flto -Wno-invalid-offsetof

READLINE_CFLAGS = $(shell pkg-config --cflags readline)
READLINE_LIBS = $(shell pkg-config --libs readline)

LDFLAGS = $(FLAGS) $(READLINE_LIBS) -pthread
SRCFLAGS = $(FLAGS) $(READLINE_CFLAGS) -Isrc -MMD

SRC = $(shell find src/ -type f -name "*.cpp")
SRC += $(shell find src/ -type f -name "*.c")
OBJS = $(patsubst src/%,build/%.o,$(SRC))

main: $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) -lhistory $(OBJS) -lcrypto

build/%.o: src/%
	mkdir -p $(dir $@)
	$(CXX) $(SRCFLAGS) -c -o $@ $<
	
clean:
	rm -rf build
	rm -f main
	
-include $(OBJS:.o=.d)
