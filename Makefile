objects = main.o Node.o Application.o

all: $(objects) arrsum.cpp
	g++ -g $(objects) -pthread
	g++ -o arrsum arrsum.cpp

$(objects): Structure.h Application.h

main.o: main.cpp Node.h
	g++ -g -c main.cpp

Application.o: Application.cpp md5.h
	g++ -g -c Application.cpp

Node.o: Node.cpp Node.h md5.h
	g++ -g -c Node.cpp

clean:
	rm *.o *.out arrsum out* part* */out* */part*
