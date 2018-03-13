#include <semaphore.h>
#include <sys/time.h>

struct PrintJob{
    int size;
    struct timeval timeAdded;
};

sem_t empty, full;

void pushJob(struct PrintJob *job);
struct PrintJob* popJob();
int count();