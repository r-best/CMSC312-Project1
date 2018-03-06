default: FCFS

FCFS : FCFS.o linked_list.o
	gcc $^ -o $@

FCFS.o : FCFS.c main.h linked_list.h
	gcc -Wall -lpthread -c $< -o $@

linked_list.o : linked_list.c main.h linked_list.h
	gcc -Wall -lpthread -c $< -o $@

clean :
	rm -f FCFS
	rm -f *.o