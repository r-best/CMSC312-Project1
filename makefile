default: FCFS

FCFS: FCFS.out

FCFS.out : main.o linked_list_FCFS.o
	gcc -pthread $^ -o $@

main.o : main.c linked_list.h
	gcc -Wall -c $< -o $@

linked_list_FCFS.o : linked_list_FCFS.c linked_list.h
	gcc -Wall -c $< -o $@

clean :
	rm -f *.out
	rm -f *.o