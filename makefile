all : main Application.o arrsum Node.o 
main : main.cpp
	g++ main.cpp -pthread -o main
arrsum : arrsum.cpp
	g++ arrsum.cpp -o arrsum
Application.o : Application.cpp
	g++ -c Application.cpp -pthread 
Node.o : Node.cpp
	g++ -c Node.cpp -pthread 
clean :
	rm main Application.o arrsum Node.o


