CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2

# Add arguments
ifeq (run, $(firstword $(MAKECMDGOALS)))
  ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(ARGS):;@:)
endif

SRCS = main.cpp event.cpp process.cpp scheduler.cpp core.cpp workload.cpp
HDRS = event.h process.h scheduler.h core.h workload.h

# Test settings, override on the command line
# e.g. make test MODES="0 1 2" SEEDS="0 1 2"
MODES ?= 0 1 2
SEEDS ?= 0
TIMEOUT ?= 60

.PHONY: run test clean

# Rebuilds only when a source or header changes
sim: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o sim

run: sim
	./sim $(ARGS)

test: sim
	@echo "Running every input in tests/debug and tests/inputs (modes: $(MODES), seeds: $(SEEDS))"
	@MODES="$(MODES)" SEEDS="$(SEEDS)" TIMEOUT="$(TIMEOUT)" bash tests/run_tests.sh; \
	status=$$?; \
	echo "See tests/README.md for the input format, what each check means, and how to add tests."; \
	exit $$status

clean:
	rm -f sim tests/results.csv