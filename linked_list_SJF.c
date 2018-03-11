#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#include "linked_list.h"

#define QUEUE_MAX 15

struct Node {
    struct PrintJob *job;
    struct Node *next;
} *head;

void pushJob(struct PrintJob *job){
    printf("PUSHING\n");
    struct Node *new = (struct Node *)malloc(sizeof(struct Node));
    if(new == NULL){
        printf("Error allocating memory\n");
        exit(-1);
    }
    new->job = job;
    new->next = NULL;

    if(head == NULL)
        head = new;
    else{
        struct Node *last = (struct Node *)head;
        while(last->next != NULL)
            last = last->next;
        last->next = new;
    }

    int c = count();
    if(c != QUEUE_MAX) // If queue is now full, lock full semaphore
        sem_post(&full);
    if(c == 1) // If queue is no longer empty, free the empty semaphore
        sem_post(&empty);
    printf("PUSHING DONE\n");
}

struct PrintJob* popJob(){
    printf("POPPING\n");
    if(head == NULL){
        return NULL;
    }
    
    struct Node *current = head; // Used to iterate through the list
    struct Node *prev = NULL; // The Node from the previous iteration
    struct Node *smallest = head; // The Node with the smallest job size
    struct Node *beforeSmallest = NULL; // The Node immediately preceding the smallest one
    while(current->next != NULL){
        if(current->job->size < smallest->job->size){
            beforeSmallest = prev;
            smallest = current;
        }
        prev = current;
        current = current->next;
    }
    // Remove smallest from the list and return its job
    beforeSmallest->next = smallest->next;
    smallest->next = NULL;
    struct PrintJob *job = smallest->job;
    free(smallest);

    int c = count();
    if(c == QUEUE_MAX-1) // If queue is no longer full, free the full semaphore
        sem_post(&full);
    if(c != 0)
        sem_post(&empty);

    printf("POPPING DONE\n");
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