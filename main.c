#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linked_list.h"

// Helper function to generate a random number between min and max
int randBetween(int min, int max){
    return rand() % (max + 1 - min) + min;
}

// Helper function to get microsecond value from a timeval struct
long getMicroTime(struct timeval *t){
    return (t->tv_sec * 1000000) + t->tv_usec;
}

uint32_t getMicroSecondsDiff(struct timeval *start, struct timeval *end){
    return (uint32_t)(getMicroTime(start) - getMicroTime(end));
}

// Helper function to store two integers in one uint64
uint64_t compress2x32to64(uint32_t x, uint32_t y){
    return (x & 0x0000FFFF) | (y << 16);
}
uint32_t getTopCompressed(uint64_t compressed){
    return (uint32_t)(compressed & 0x0000FFFF);
}
uint32_t getBottomCompressed(uint64_t compressed){
    return (uint32_t)(compressed >> 16);
}

sem_t mutex; // Lock for accessing the job queue
pthread_t *producers, *consumers;  // Arrays of producers and consumers
int numProducers, numConsumers, counter; // Counter keeps track of how many producers have finished

void shutdown(){
    printf("SHUTTING DOWN PROGRAM\n");

    // Destroy semaphores
    printf("\tDESTROYING SEMAPHORES\n");
    if(sem_destroy(&full) == -1)
        printf("\tError destroying semaphore 'full'");
    if(sem_destroy(&empty) == -1)
        printf("\tError destroying semaphore 'empty'");
    if(sem_destroy(&mutex) == -1)
        printf("\tError destroying semaphore 'mutex'");
    
    // Deallocate the producer and consumer arrays
    // All other memory should be freeing itself when its job is
    // done as the program goes, I think I did pretty good on that
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
    for(i = 0; i <= numJobs; i++){ // For each job (plus one, last loop inserts a -1 sized job as a flag)
        struct PrintJob *job = malloc(sizeof(struct PrintJob)); // Malloc a PrintJob
        if(job == NULL){
            printf("Error allocating memory for print job\n");
            shutdown();
            exit(-1);
        }
        if(i < numJobs){ // If for loop is NOT on its last iteration (i.e. still producing jobs)
            job->size = randBetween(100, 1000); // Generate random job size
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
        double seconds = randBetween(1, 10) / 10; // 0.1-1 seconds
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

    // Initial size of results (+1 is because the first item is not a job, but the total number of jobs)
    int resultsSize = numProducers+1;
    uint64_t *results = malloc(sizeof(uint64_t) * resultsSize); // Malloc initial size of results
    uint64_t jobsConsumed = 0;

    struct timeval currentTime;
    while(counter < numProducers){
        sem_wait(&empty);
        sem_wait(&mutex);
        struct PrintJob *job = popJob();
        sem_post(&mutex);
        if(job == NULL){
            continue;
        }
        // Get current time
        gettimeofday(&currentTime, NULL);
        // Store the job's properties and free it
        int size = job->size;
        uint32_t wait = getMicroSecondsDiff(&currentTime, &(job->timeAdded));
        free(job);

        printf("Consumer %d consumed %d\n", thread_num, size);
        if(size < 0){ // If this is a flag job, increment counter to show a producer thread has finished
            counter++;
        }
        else{ // Else it was an actual job, so add its data to results
            jobsConsumed++;
            if(jobsConsumed > resultsSize-1){ // Realloc results to double size if it's too small
                resultsSize *= 2;
                results = realloc(results, sizeof(uint64_t) * resultsSize);
                if(results == NULL){
                    printf("Error reallocating memory");
                    shutdown();
                    exit(-1);
                }
            }
            // Add size and wait to results in one element by compressing into one uint64
            results[jobsConsumed] = compress2x32to64(size, wait);
            // Sleep based on job size (size/100 seconds)
            usleep((size / 100)*1000000);
        }
    }
    printf("Consumer %d exiting\n", thread_num);
    sem_post(&empty);
    results[0] = jobsConsumed; // Now that we have # of jobs consumed, put it in first spot of results
    free(args);
    // Exit and pass the results back to the main thread to be displayed at the end
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
    uint64_t* consumerResults[numConsumers]; // Array holding results from all consumer threads
    for(i = 0; i < numConsumers; i++){
        pthread_join(consumers[i], (void**)&consumerResults[i]);
    }
    printf("ALL CONSUMERS FINISHED\n");

    // Print out producer results
    int totalNumJobs = 0;
    for(i = 0; i < numProducers; i++){
        int numJobs = producerResults[i][0]; // First item of results is number of results
        printf("Producer %d produced %d jobs:\n", i+1, numJobs);
        int j = 0;
        for(j = 1; j <= numJobs; j++){
            printf("\tJob %d: %d bytes\n", j, producerResults[i][j]);
            totalNumJobs++;
        }
        free(producerResults[i]);
    }

    // Print out consumer results
    double averageWaitTime = 0;
    for(i = 0; i < numConsumers; i++){
        uint64_t numJobs = consumerResults[i][0]; // First item of results is number of results
        printf("Consumer %d consumed %ld jobs:\n", i+1, numJobs);
        int j = 0;
        for(j = 1; j <= numJobs; j++){
            // Retrieve size and wait time from compressed uint64
            uint32_t size = getTopCompressed(consumerResults[i][j]);
            uint32_t waitTime = getBottomCompressed(consumerResults[i][j]);
            printf("\tJob %d: %d bytes, waited for %d microseconds\n", j, size, waitTime);
            averageWaitTime += waitTime;
        }
        free(consumerResults[i]);
    }
    averageWaitTime /= (double)totalNumJobs;
    printf("Average wait time: %f microseconds => %f seconds\n", averageWaitTime, averageWaitTime / 1000000.0);
    
    shutdown();
    return 0;
}
