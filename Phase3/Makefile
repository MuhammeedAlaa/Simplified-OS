build:
	gcc producer.c -o producer.o
	gcc consumer.c -o consumer.o
	gcc initializer.c -o initializer.o
	gcc destructor.c -o destructor.o
	./initializer.o
clean:
	./destructor.o
	rm -f *.out *.o processes.txt

all: clean build

runc:
	./consumer.o
runp:
	./producer.o
