build:
	gcc hashmap.c -g -o hashmap.o -c

	gcc queue.c -g -o queue.o -c

	gcc sortedLinkedList.c -g -o  sortedLinkedList.o -c
	gcc buddy.c -g -o buddy.o -c -lm
	# gcc -o buddy.out buddy.o sortedLinkedList.o -lm

	gcc priorityQueue.c -g -o  priorityQueue.o -c
	
	gcc process_generator.c -g -o process_generator.out
	
	gcc clk.c -g -o clk.out
	
	gcc scheduler.c -g -o scheduler.o -c
	gcc -g -o scheduler.out scheduler.o hashmap.o priorityQueue.o queue.o buddy.o sortedLinkedList.o -lm
	
	gcc process.c -g -o process.out
	
	gcc test_generator.c -g -o test_generator.out

clean:
	rm -f *.out *.o processes.txt

all: clean build

run:
	./process_generator.out
