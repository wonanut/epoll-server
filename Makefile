all:
	g++ MyThreadPool.cpp MyThreadPool.h server.cpp -o server -g -std=c++11 -lpthread
	g++ client.cpp -o client -std=c++11 -lpthread
clean:
	rm -f server
	rm -f client
.PHONY : clean