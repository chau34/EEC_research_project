run:
	g++ main.cpp event.cpp process.cpp scheduler.cpp -o sim
	./sim 10

clean:
	rm -f sim