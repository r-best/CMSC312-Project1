#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linked_list.h"

// Helper function to generate a random number between min and max
int randBetween(int min, int max){
    return rand() % (max + 1 - min) + min;
}

sem_t mutex; // Lock for accessing the job queue
pthread_t *producers, *consumers; 
int numProducers, numConsumers, counter; // Counter keeps track of how many producers have finished

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
}

void signalHandler(int signal){
	if(signal == SIGINT){
        // Wait on mutex so we don't destroy stuff while threads are working
        sem_wait(&mutex);
        shutdown();
        exit(0);
    }
}

void* producerFn(void *args){
    int thread_num = *((int*)args); // Get the thread's number from args
    printf("Starting producer %d\n", thread_num);
    int numJobs = (rand() % 20) + 1; // Randomly generate number of jobs

    // Malloc array of results to be returned from thread on completion,
    // first item is number of items, the rest are the sizes of all jobs produced
    int *results = malloc(sizeof(int) * (numJobs+1));
    results[0] = numJobs;

    int i = 0;
    for(i = 0; i <= numJobs; i++){ // For each job (plus one)
        struct PrintJob *job = malloc(sizeof(struct PrintJob)); // Malloc a PrintJob
        if(job == NULL){
            printf("Error allocating memory for print job\n");
            shutdown();
            exit(-1);
        }
        if(i < numJobs){ // If for loop is NOT on its last loop (i.e. still producing jobs)
            job->size = randBetween(100, 1000); // Generate job size
            results[i+1] = job->size; // Record this job in results
        }
        else // If all jobs done, insert a flag job instead (-1 size to tell consumers you're done)
            job->size = -1;
        sem_wait(&full);
        sem_wait(&mutex);
        sem_post(&full);
        pushJob(job); // Push job onto queue
        printf("Producer %d produced %d\n", thread_num, job->size);
        sem_post(&mutex);

        // Randomly wait 0.1-1 seconds
        int seconds = randBetween(1, 10) / 10; // 0.1-1 seconds
        usleep(seconds * 1000000);
    }
    printf("Producer %d exiting\n", thread_num);
    free(args);
    // Exit and pass the results back to the main thread to be displayed at the end
    pthread_exit(results);
}

void* consumerFn(void *args){
    int thread_num = *((int*)args); // Get the thread's number from args
    printf("Starting consumer %d\n", thread_num);

    // Initial size of results (+1 is because the first item is not a job size, but the number of jobs)
    int resultsSize = numProducers+1;
    int *results = malloc(sizeof(int) * resultsSize); // Malloc initial size of results
    int jobsConsumed = 0;

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

        if(size < 0){ // If this is a flag job, increment counter to show a producer thread has finished
            counter++;
        }
        else{ // Else it was an actual job, so add its size to results
            jobsConsumed++;
            if(jobsConsumed > resultsSize-1){ // Realloc results to double size if it's too small
                resultsSize *= 2;
                results = realloc(results, sizeof(int) * resultsSize);
                if(results == NULL){
                    printf("Error reallocating memory");
                    shutdown();
                    exit(-1);
                }
            }
            results[jobsConsumed] = size;
        }
        printf("Consumer %d consumed %d\n", thread_num, size);
        usleep((size / 1000)*1000000);
    }
    printf("Consumer %d exiting\n", thread_num);
    sem_post(&empty);
    results[0] = jobsConsumed; // Now that we have # of jobs consumed, put it in first spot of results
    free(args);
    pthread_exit(results);
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
    int i = 0; // For loop iterator to make it compile on the server
    for(i = 1; i <= numProducers; i++){
        int *thread_num = malloc(sizeof(int));
        if(thread_num == NULL){
            printf("Error allocating memory\n");
            shutdown();
            exit(-1);
        }
        *thread_num = i;
        int ret = pthread_create(&producers[i-1], NULL, producerFn, thread_num);
        if(ret < 0){
            printf("Error spawning producer thread %d\n", i);
            shutdown();
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
    for(i = 1; i <= numConsumers; i++){
        int *thread_num = malloc(sizeof(int));
        if(thread_num == NULL){
            printf("Error allocating memory\n");
            shutdown();
            exit(-1);
        }
        *thread_num = i;
        int ret = pthread_create(&consumers[i-1], NULL, consumerFn, thread_num);
        if(ret < 0){
            printf("Error spawning consumer thread %d\n", i);
            shutdown();
            exit(-1);
        }
    }

    // Join producers
    int* producerResults[numProducers]; // Array holding results from all producer threads
    for(i = 0; i < numProducers; i++){
        pthread_join(producers[i], (void**)&producerResults[i]);
    }
    printf("ALL PRODUCERS FINISHED\n");

    // Join consumers
    int* consumerResults[numConsumers]; // Array holding results from all consumer threads
    for(i = 0; i < numConsumers; i++){
        pthread_join(consumers[i], (void**)&consumerResults[i]);
    }
    printf("ALL CONSUMERS FINISHED\n");

    // Print out producer results
    for(i = 0; i < numProducers; i++){
        int numJobs = producerResults[i][0];
        printf("Producer %d produced %d jobs:\n", i+1, numJobs);
        int j = 0;
        for(j = 1; j <= numJobs; j++){
            printf("\tJob %d: %d bytes\n", j, producerResults[i][j]);
        }
        free(producerResults[i]);
    }

    // Print out consumer results
    for(i = 0; i < numConsumers; i++){
        int numJobs = consumerResults[i][0];
        printf("Consumer %d consumed %d jobs:\n", i+1, numJobs);
        int j = 0;
        for(j = 1; j <= numJobs; j++){
            printf("\tJob %d: %d bytes\n", j, consumerResults[i][j]);
        }
        free(consumerResults[i]);
    }
    
    shutdown();
    return 0;
}
