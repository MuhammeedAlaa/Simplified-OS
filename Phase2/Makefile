build:
	gcc hashmap.c -o hashmap.o -c

	gcc queue.c -o queue.o -c

	gcc sortedLinkedList.c -o sortedLinkedList.o -c
	gcc buddy.c -o buddy.o -c -lm
	# gcc -o buddy.out buddy.o sortedLinkedList.o -lm

	gcc priorityQueue.c -o priorityQueue.o -c
	
	gcc process_generator.c -o process_generator.out
	
	gcc clk.c -o clk.out
	
	gcc scheduler.c -o scheduler.o -c
	gcc -o scheduler.out scheduler.o hashmap.o priorityQueue.o queue.o buddy.o sortedLinkedList.o -lm
	
	gcc process.c -o process.out
	
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out *.o processes.txt

all: clean build

run:
	./process_generator.out
