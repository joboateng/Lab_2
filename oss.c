#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define SHMKEY_CLOCK 1234
#define SHMKEY_PROCESS_TABLE 5678
#define MAX_PROCESSES 20
#define DEFAULT_TIME_LIMIT 5
#define DEFAULT_INTERVAL 100000 // 100 ms in nanoseconds

struct PCB {
    int occupied;
    pid_t pid;
    int startSeconds;
    int startNano;
};

struct SimulatedClock {
    int seconds;
    int nanoseconds;
};

struct PCB processTable[MAX_PROCESSES];
struct SimulatedClock *simulatedClock;

int shmClockID, shmProcessTableID;

void initializeProcessTable() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processTable[i].occupied = 0;
    }
}

void initializeSharedMemory() {
    // Create shared memory for the simulated system clock
    shmClockID = shmget(SHMKEY_CLOCK, sizeof(struct SimulatedClock), IPC_CREAT | 0666);
    if (shmClockID == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    simulatedClock = (struct SimulatedClock *)shmat(shmClockID, NULL, 0);
    if ((void *)simulatedClock == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Create shared memory for the process table
    shmProcessTableID = shmget(SHMKEY_PROCESS_TABLE, sizeof(struct PCB) * MAX_PROCESSES, IPC_CREAT | 0666);
    if (shmProcessTableID == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    struct PCB *sharedProcessTable = (struct PCB *)shmat(shmProcessTableID, NULL, 0);
    if ((void *)sharedProcessTable == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize process table
    memcpy(processTable, sharedProcessTable, sizeof(struct PCB) * MAX_PROCESSES);

    // Detach shared memory for the process table
    shmdt(sharedProcessTable);
}

void cleanupSharedMemory() {
    // Detach and remove shared memory for the clock
    shmdt(simulatedClock);
    shmctl(shmClockID, IPC_RMID, NULL);

    // Attach shared memory for the process table
    struct PCB *sharedProcessTable = (struct PCB *)shmat(shmProcessTableID, NULL, 0);
    if ((void *)sharedProcessTable == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Update shared process table with the current state
    memcpy(sharedProcessTable, processTable, sizeof(struct PCB) * MAX_PROCESSES);

    // Detach shared memory for the process table
    shmdt(sharedProcessTable);

    // Remove shared memory for the process table
    shmctl(shmProcessTableID, IPC_RMID, NULL);
}

void terminateHandler(int signum) {
    cleanupSharedMemory();
    exit(EXIT_SUCCESS);
}

void incrementClock() {
    simulatedClock->nanoseconds += DEFAULT_INTERVAL;
    if (simulatedClock->nanoseconds >= 1000000000) {
        simulatedClock->seconds++;
        simulatedClock->nanoseconds -= 1000000000;
    }
}

void checkIfChildHasTerminated() {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (processTable[i].occupied && processTable[i].pid == pid) {
                processTable[i].occupied = 0;
                processTable[i].pid = 0;
                processTable[i].startSeconds = 0;
                processTable[i].startNano = 0;
                break;
            }
        }
    }
}

void possiblyLaunchNewChild(int maxSimultaneousProcesses, int timeLimit, int interval) {
    int emptyEntry = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processTable[i].occupied) {
            emptyEntry = i;
            break;
        }
    }

    if (emptyEntry != -1 && maxSimultaneousProcesses > 0) {
        int randomInterval = (rand() % (timeLimit * 1000000000)) + 1;
        if (simulatedClock->nanoseconds + randomInterval >= 1000000000) {
            simulatedClock->seconds++;
            simulatedClock->nanoseconds = simulatedClock->nanoseconds + randomInterval - 1000000000;
        } else {
            simulatedClock->nanoseconds += randomInterval;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            char intervalStr[20];
            sprintf(intervalStr, "%d", interval);
            execl("./worker", "./worker", intervalStr, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        } else {
            processTable[emptyEntry].occupied = 1;
            processTable[emptyEntry].pid = pid;
            processTable[emptyEntry].startSeconds = simulatedClock->seconds;
            processTable[emptyEntry].startNano = simulatedClock->nanoseconds;

            maxSimultaneousProcesses--;
        }
    }
}

void outputProcessTable() {
    printf("OSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), simulatedClock->seconds, simulatedClock->nanoseconds);
    printf("Process Table:\n");
    printf("Entry  Occupied  PID  StartS  StartN\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        printf("%-6d %-9d %-4d %-6d %-6d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 9) {
        fprintf(stderr, "Usage: %s -n <proc> -s <simul> -t <timelimitForChildren> -i <intervalInMsToLaunchChildren>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int proc = 0, simul = 0, timeLimit = DEFAULT_TIME_LIMIT, interval = DEFAULT_INTERVAL;
    int opt;

    while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s -n <proc> -s <simul> -t <timelimitForChildren> -i <intervalInMsToLaunchChildren>\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'n':
                proc = atoi(optarg);
                break;
            case 's':
                simul = atoi(optarg);
                break;
            case 't':
                timeLimit = atoi(optarg);
                break;
            case 'i':
                interval = atoi(optarg) * 1000000; // Convert from milliseconds to nanoseconds
                break;
            default:
                fprintf(stderr, "Invalid option\n");
                exit(EXIT_FAILURE);
        }
    }

    if (proc <= 0 || simul <= 0 || timeLimit <= 0 || interval <= 0) {
        fprintf(stderr, "Invalid parameters\n");
        exit(EXIT_FAILURE);
    }

    // Set up signal handlers for cleanup
    signal(SIGINT, terminateHandler);

    // Initialize shared memory
    initializeSharedMemory();

    int maxSimultaneousProcesses = simul;
    int processesLaunched = 0;

    while (1) {
        incrementClock();

        if (processesLaunched < proc) {
            possiblyLaunchNewChild(maxSimultaneousProcesses, timeLimit, interval);
            processesLaunched++;
        }

        if (processesLaunched == proc && maxSimultaneousProcesses == 0) {
            break;
        }

        checkIfChildHasTerminated();

        if (simulatedClock->nanoseconds % 500000000 == 0) {
            // Output the process table every half second
            outputProcessTable();
        }
    }

    while (wait(NULL) > 0);

    terminateHandler(SIGINT);
}
