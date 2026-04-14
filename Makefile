CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS  :=
TARGET   := memmanager
SRCDIR   := src
SRCS     := $(wildcard $(SRCDIR)/*.cpp)
OBJS     := $(SRCS:$(SRCDIR)/%.cpp=build/%.o)
.PHONY: all clean run debug
all: build/$(TARGET)
build/$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "  Built: $@"
build/%.o: $(SRCDIR)/%.cpp | build/
	$(CXX) $(CXXFLAGS) -c $< -o $@
build/:
	mkdir -p build
debug: CXXFLAGS += -g -DDEBUG -fsanitize=address,undefined
debug: LDFLAGS  += -fsanitize=address,undefined
debug: all
run: all
	./build/$(TARGET)
clean:
	rm -rf build/
