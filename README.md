# How to Compile and Run

Clone this repository:

```bash
git clone https://github.com/joboateng/Lab_2
```

Navigate to the project directory:

```bash
cd Lab_2
```

Compile the project using the provided makefile:

```bash
make
```

## Running the Simulation

### OSS

Run the OSS program with the desired parameters:

```bash
./oss -n <total_processes> -s <simultaneous_processes> -t <time_limit> -i <interval_to_launch>
```

Example:

```bash
./oss -n 5 -s 2 -t 5 -i 100
```

### Worker

Open a new terminal and run the worker program:

```bash
./worker <interval>
```

Example:

```bash
./worker 100
```

## Clean Up

To remove compiled files:

```bash
make clean
```

## Program Details

### OSS

- Manages a simulated system clock and a process table.
- Launches worker processes with specified time intervals.
- Outputs the process table every half second.

### Worker

- Takes a time interval as a command-line argument.
- Simulates work by printing system clock information at regular intervals.
- Terminates when the specified time interval has elapsed.
