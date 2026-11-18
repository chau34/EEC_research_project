CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2

# Add arguments
ifeq (run, $(firstword $(MAKECMDGOALS)))
  ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(ARGS):;@:)
endif

SRCS = main.cpp event.cpp process.cpp scheduler.cpp core.cpp

.PHONY: run clean

run:
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim
	./sim $(ARGS)

test:
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim
	./sim 10 0

clean:
	rm -f sim