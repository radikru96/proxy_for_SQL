CC=g++ -std=c++17 -g -Wall
RC=g++ -std=c++17 -Wall

debug: bin/main_debug
release: bin/main
clean:
	rm -rf obj/*o bin/* *.xml

bin/main_debug: obj/main_debug.o
	$(CC) obj/main_debug.o -o bin/main_debug

bin/main: obj/main.o
	$(RC) obj/main.o -o bin/main

obj/main.o: src/main.cpp src/skel.h
	$(RC) -c src/main.cpp -o obj/main.o

obj/main_debug.o: src/main.cpp src/skel.h
	$(CC) -c src/main.cpp -o obj/main_debug.o -D DEBUG