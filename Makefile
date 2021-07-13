CC=g++ -g -Wall -lmysqlclient
RC=g++ -Wall -lmysqlclient

debug: bin/main_debug
release: bin/main

bin/main_debug: obj/main.o
	$(CC) obj/main.o -o bin/main_debug

bin/main: obj/main.o
	$(RC) obj/main.o -o bin/main

obj/main.o: src/main.cpp
	$(CC) -c src/main.cpp -o obj/main.o

