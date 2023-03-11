#fisier folosit pentru compilarea serverului&clientului TCP iterativ

all:
	g++ client.cpp -o client
	g++ server.cpp -o server
clean:
	rm -f client server