#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linked_list.h"

void signalHandler(int signal){
	if(signal == SIGINT){
        return;
    }
}

sem_t mutex;
int numProducers, numConsumers, counter;

void initPrintJob(struct PrintJob *job){
    // printf("Initializing print job\n");
    job->size = 2;//rand() % (1000 + 1 - 100) + 100;
}

void* producerFn(void *args){
    printf("Starting producer thread\n");
    int numJobs = 1;//(rand() % 20) + 1;
    for(int i = 0; i <= numJobs; i++){
        struct PrintJob *job = malloc(sizeof(struct PrintJob));
        if(job == NULL){
            printf("Error allocating memory for print job\n");
            exit(-1);
        }
        if(i < numJobs)
            initPrintJob(job);
        else // If all jobs done, insert a flag job instead (-1 size to tell consumers you're done)
            job->size = -1;
        sem_wait(&full);
        sem_wait(&mutex);
        sem_post(&full);
        pushJob(job);
        printf("Produced %d\n", job->size);
        sem_post(&mutex);
    }
    printf("Producer thread exiting\n");
    pthread_exit(0);
}

void* consumerFn(void *args){
    printf("Starting consumer thread\n");
    while(counter < numProducers){
        sem_wait(&empty);
        sem_wait(&mutex);
        struct PrintJob *job = popJob();
        // sem_post(&empty);
        sem_post(&mutex);
        if(job == NULL){
            // sem_post(&mutex);
            continue;
        }
        int size = job->size;
        free(job);

        // If this is a flag job, increment counter
        if(size < 0){
            counter++;
            printf("COUNTER: %d %d\n", counter, numProducers);
            // If counter has reached numProducers, all producers are done so break
            if(counter == numProducers){
                printf("YES\n");
                // break;
            }
        }
        printf("Consumed %d\n", size);
        sleep(size / 1000);
    }
    printf("Consumer thread exiting\n");
    sem_post(&empty);
    printf("A\n");
    pthread_exit(0);
}

int main(int argc, char *argv[]){
    // Set up signal handler
    // struct sigaction action;
	// action.sa_handler = signalHandler;
	// action.sa_flags = SA_NODEFER | SA_ONSTACK;
	// sigaction(SIGINT, &action, NULL);

    // Process args
    if(argc < 3){
        printf("Program requires at least 2 arguments\n");
        exit(-1);
    }
    numProducers = atoi(argv[1]);
    numConsumers = atoi(argv[2]);

    printf("NUMPRODUCERS: %d\n", numProducers);
    printf("NUMCONSUMERS: %d\n", numConsumers);

    // Seed random
    srand(time(NULL));

    // Initialize semaphores
    printf("Initializing semaphores...\n");
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, 0);
    sem_init(&full, 0, 1);
    printf("\tDone\n");

    // Create producers
    printf("Creating producer threads...\n");
    pthread_t *producers = malloc(sizeof(pthread_t) * numProducers);
    if(producers == NULL){
        printf("Error allocating memory\n");
        exit(-1);
    }
    for(int i = 0; i < numProducers; i++){
        int ret = pthread_create(&producers[i], NULL, producerFn, NULL);
        if(ret < 0){
            printf("Error spawning producer thread %d\n", i+1);
            exit(-1);
        }
    }
    printf("\tDone\n");

    // Create consumers
    printf("Creating consumer threads...\n");
    counter = 0;
    pthread_t *consumers = malloc(sizeof(pthread_t) * numConsumers);
    if(consumers == NULL){
        printf("Error allocating memory\n");
        exit(-1);
    }
    for(int i = 0; i < numConsumers; i++){
        int ret = pthread_create(&consumers[i], NULL, consumerFn, NULL);
        if(ret < 0){
            printf("Error spawning consumer thread %d\n", i+1);
            exit(-1);
        }
    }
    printf("\tDone\n");

    for(int i = 0; i < numProducers; i++){
        pthread_join(producers[i], NULL);
    }
    printf("ALL PRODUCERS FINISHED\n");

    for(int i = 0; i < numConsumers; i++){
        pthread_join(consumers[i], NULL);
    }
    printf("ALL CONSUMERS FINISHED\n");
}