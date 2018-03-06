#include <semaphore.h>

struct PrintJob{
    int size;
};

sem_t empty, full;

void pushJob(struct PrintJob *job);
struct PrintJob* popJob();
int count();