#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <semaphore.h>

#include "linked_list.h"

#define QUEUE_MAX 15

struct Node {
    struct PrintJob *job;
    struct timeval timeAdded;
    struct Node *next;
} *head;

void pushJob(struct PrintJob *job){
    // Malloc and set up a new linked list Node
    struct Node *new = (struct Node *)malloc(sizeof(struct Node));
    if(new == NULL){
        printf("Error allocating memory\n");
        exit(-1);
    }
    new->job = job;
    gettimeofday(&(new->job->timeAdded), NULL);
    new->next = NULL;

    if(head == NULL) // If the list is empty, make this the head
        head = new;
    else{ // Else navigate to the end of the list and append this on
        struct Node *last = (struct Node *)head;
        while(last->next != NULL)
            last = last->next;
        last->next = new;
    }

    int c = count();
    if(c != QUEUE_MAX) // If queue is not full, full should be unlocked
        sem_post(&full);
    if(c == 1) // If queue just left empty state, unlock empty semaphore
        sem_post(&empty);
}

struct PrintJob* popJob(){
    if(head == NULL){
        return NULL;
    }

    struct Node *smallest = head; // The Node with the smallest job size
    struct Node *beforeSmallest = NULL; // The Node immediately preceding the smallest one
    
    struct Node *current = head; // Used to iterate through the list
    struct Node *prev = NULL; // The Node from the previous iteration
    while(current->next != NULL){ // Iterate through list and find smallest job
        if(current->job->size < smallest->job->size){
            beforeSmallest = prev;
            smallest = current;
        }
        prev = current;
        current = current->next;
    }

    // Remove smallest from the list and return its job
    if(smallest == head){
        head = head->next;
    }
    else {
        beforeSmallest->next = smallest->next;
        smallest->next = NULL;
    }
    struct PrintJob *job = smallest->job;
    free(smallest);
    
    int c = count();
    if(c == QUEUE_MAX-1) // If queue just left full state, unlock the full semaphore
        sem_post(&full);
    if(c != 0) // If queue is not empty, empty should be unlocked
        sem_post(&empty);

    return job;
}

int count(){
    struct Node *n = head;
    int c = 0;
    while(n != NULL){
        n = n->next;
        c++;
    }
    return c;
}