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
    // Store the head in a temp variable and make the next node the new head
    struct Node *temp = head;
    head = head->next;
    struct PrintJob *job = temp->job; // Free the old head and return its job
    free(temp);
    
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