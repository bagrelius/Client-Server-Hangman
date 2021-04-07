
all: game_server game_client

game_server: server.cpp
	g++ -g -Wall -Werror -pedantic -pthread -std=c++11 -o $@ $^

game_client: client.cpp
	g++ -g -Wall -Werror -pedantic -std=c++11 -o $@ $^

clean:
	rm -f *.o game_server game_client
