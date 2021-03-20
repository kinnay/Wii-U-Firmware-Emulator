
FLAGS = -std=c++14 -O2 -g -flto -Wno-invalid-offsetof

LDFLAGS = $(FLAGS) -pthread
SRCFLAGS = $(FLAGS) -Isrc -MMD

SRC = $(shell find src/ -type f -name "*.cpp")
SRC += $(shell find src/ -type f -name "*.c")
OBJS = $(patsubst src/%,build/%.o,$(SRC))

main: $(OBJS)
	g++-8 -o $@ $(LDFLAGS) $(OBJS) -lcrypto

build/%.o: src/%
	mkdir -p $(dir $@)
	g++-8 $(SRCFLAGS) -c -o $@ $<
	
clean:
	rm -rf build
	rm -f main
	
-include $(OBJS:.o=.d)
