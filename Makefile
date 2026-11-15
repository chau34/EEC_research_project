CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2

run:
	$(CXX) $(CXXFLAGS) main.cpp event.cpp process.cpp scheduler.cpp core.cpp -o sim
	./sim 10

clean:
	rm -f sim