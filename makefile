default: FCFS SJF

FCFS: FCFS.out

SJF: SJF.out

FCFS.out : main.o linked_list_FCFS.o
	gcc -pthread $^ -o $@

SJF.out : main.o linked_list_SJF.o
	gcc -pthread $^ -o $@

main.o : main.c linked_list.h
	gcc -Wall -c $< -o $@

linked_list_FCFS.o : linked_list_FCFS.c linked_list.h
	gcc -Wall -c $< -o $@

linked_list_SJF.o : linked_list_SJF.c linked_list.h
	gcc -Wall -c $< -o $@

clean :
	rm -f *.out
	rm -f *.o