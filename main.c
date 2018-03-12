#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linked_list.h"

int randBetween(int min, int max){
    return rand() % (max + 1 - min) + min;
}

sem_t mutex;
pthread_t *producers, *consumers;
int numProducers, numConsumers, counter;

void shutdown(){
    printf("SHUTTING DOWN PROGRAM\n");

    // Destroy semaphores
    printf("\tDESTROYING SEMAPHORES\n");
    if(sem_destroy(&full) == -1)
        printf("\tError destroying semaphore");
    if(sem_destroy(&empty) == -1)
        printf("\tError destroying semaphore");
    if(sem_destroy(&mutex) == -1)
        printf("\tError destroying semaphore");
    
    // Shutting down threads
    // printf("\tSHUTTING DOWN REAMINING THREADS\n");
    // for(int i = 0; i < numProducers; i++){
    //     pthread_kill(producers[i], 9);
    // }
    // for(int i = 0; i < numConsumers; i++){
    //     pthread_kill(consumers[i], 9);
    // }
    
    // Deallocate the producer and consumer arrays
    printf("\tDEALLOCATING MEMORY\n");
    free(producers);
    free(consumers);

    printf("\tEXITING\n");
    exit(0);
}

void signalHandler(int signal){
	if(signal == SIGINT){
        // Wait on mutex so we don't destroy stuff while threads are working
        sem_wait(&mutex);
        shutdown();
    }
}

void* producerFn(void *args){
    printf("Starting producer thread\n");
    int numJobs = (rand() % 20) + 1;
    for(int i = 0; i <= numJobs; i++){
        struct PrintJob *job = malloc(sizeof(struct PrintJob));
        if(job == NULL){
            printf("Error allocating memory for print job\n");
            exit(-1);
        }
        if(i < numJobs)
            job->size = randBetween(100, 1000);
        else // If all jobs done, insert a flag job instead (-1 size to tell consumers you're done)
            job->size = -1;
        sem_wait(&full);
        sem_wait(&mutex);
        sem_post(&full);
        pushJob(job);
        printf("Produced %d\n", job->size);
        sem_post(&mutex);

        float seconds = randBetween(1, 10) / 10;
        struct timespec ts;
        ts.tv_sec = seconds;
        ts.tv_nsec = seconds * 1000000000;
        nanosleep(&ts, NULL);
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
        sem_post(&mutex);
        if(job == NULL){
            continue;
        }
        int size = job->size;
        free(job);

        // If this is a flag job, increment counter
        if(size < 0){
            counter++;
        }
        printf("Consumed %d\n", size);
        sleep(size / 1000);
    }
    printf("Consumer thread exiting\n");
    sem_post(&empty);
    pthread_exit(0);
}

int main(int argc, char *argv[]){
    // Set up signal handler
    struct sigaction action;
	action.sa_handler = signalHandler;
	action.sa_flags = SA_NODEFER | SA_ONSTACK;
	sigaction(SIGINT, &action, NULL);

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
    producers = malloc(sizeof(pthread_t) * numProducers);
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

    // Create consumers
    printf("Creating consumer threads...\n");
    counter = 0;
    consumers = malloc(sizeof(pthread_t) * numConsumers);
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

    for(int i = 0; i < numProducers; i++){
        pthread_join(producers[i], NULL);
    }
    printf("ALL PRODUCERS FINISHED\n");

    for(int i = 0; i < numConsumers; i++){
        pthread_join(consumers[i], NULL);
    }
    printf("ALL CONSUMERS FINISHED\n");

    shutdown();
}