default: FCFS.out

FCFS.out : FCFS.o linked_list.o
	gcc -pthread $^ -o $@

FCFS.o : FCFS.c linked_list.h
	gcc -Wall -c $< -o $@

linked_list.o : linked_list.c linked_list.h
	gcc -Wall -c $< -o $@

clean :
	rm -f *.out
	rm -f *.o