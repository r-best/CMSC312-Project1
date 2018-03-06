#include <stdlib.h>
#include <semaphore.h>

#include "main.h"
#include "linked_list.h"

#define QUEUE_MAX 15

struct Node {
    struct PrintJob *job;
    struct Node *next;
} *head;

void pushJob(struct PrintJob *job){
    struct Node *new = (struct Node *)malloc(sizeof(struct Node));
    new->job = job;
    new->next = NULL;

    struct Node *last = (struct Node *)head;
    while(last->next != NULL)
        last = last->next;
    last->next = new;

    int c = count();
    if(c == QUEUE_MAX) // If queue is now full, lock full semaphore
        sem_wait(&full);
    else if(c == 1) // If queue is no longer empty, free the empty semaphore
        sem_post(&empty);
}

struct PrintJob* popJob(){
    struct Node *temp = head;
    head = head->next;
    struct PrintJob *job = temp->job;
    free(temp);
    
    int c = count();
    if(c == QUEUE_MAX-1) // If queue is no longer full, free the full semaphore
        sem_post(&full);
    else if(c == 0) // If queue is empty, lock the empty semaphore
        sem_wait(&empty);

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