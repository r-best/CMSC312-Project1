#include <semaphore.h>

sem_t empty, full;

void pushJob(struct PrintJob *job);
struct PrintJob* popJob();
int count();