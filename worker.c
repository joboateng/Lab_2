#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define SHMKEY_CLOCK 1234

struct SimulatedClock {
    int seconds;
    int nanoseconds;
};

void terminateHandler(int signum) {
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <interval>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int interval = atoi(argv[1]);

    int shmClockID = shmget(SHMKEY_CLOCK, sizeof(struct SimulatedClock), 0666);
    struct SimulatedClock *simulatedClock = (struct SimulatedClock *)shmat(shmClockID, NULL, 0);

    // Set up signal handler for cleanup
    signal(SIGINT, terminateHandler);

    while (1) {
        printf("WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d\n",
               getpid(), getppid(), simulatedClock->seconds, simulatedClock->nanoseconds);
        sleep(1);  // Simulate work
    }

    shmdt(simulatedClock);

    return EXIT_SUCCESS;
}

