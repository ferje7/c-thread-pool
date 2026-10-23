# C Thread Pool

A concurrent thread pool implemented in C using POSIX threads (`pthreads`).

This project demonstrates core operating systems concepts such as thread management, synchronization, producer-consumer design, mutexes, condition variables, and dynamic worker scaling.

## Features

- Multithreaded task processing
- Producer-consumer architecture
- Shared linked-list task queue
- Mutex protection for critical sections
- Condition variables to avoid busy-waiting
- Handling of spurious wakeups
- Clean shutdown with `pthread_cond_broadcast`
- Dynamic worker scaling under high load
- Proper thread joining and resource cleanup
- Safe concurrent execution

## Architecture

The main thread acts as the **producer** and reads tasks from standard input.

Worker threads act as **consumers**, removing tasks from a shared queue and processing them concurrently.

```text
             Main Thread
              Producer
                 |
                 v
          +---------------+
          |  Shared Queue |
          | Mutex + Cond  |
          +---------------+
            |     |     |
            v     v     v
         Worker Worker Worker
```

The shared queue is protected by a POSIX mutex to prevent race conditions.

Workers sleep on a condition variable when the queue is empty instead of continuously checking for new work.

## Synchronization

Workers wait using:

```c
pthread_cond_wait(&queue.cond, &queue.mutex);
```

The condition is checked inside a `while` loop to safely handle spurious wakeups.

When a new task is added, the producer wakes a worker using:

```c
pthread_cond_signal(&queue.cond);
```

The mutex is released before processing a task so multiple workers can execute concurrently.

## Clean Shutdown

When there are no more incoming tasks, the main thread marks the queue as complete and wakes all sleeping workers:

```c
queue.done = 1;
pthread_cond_broadcast(&queue.cond);
```

Workers finish any remaining tasks before exiting, and the main thread joins every worker before cleaning up synchronization resources.

## Dynamic Scaling

The thread pool can create additional workers when the task queue becomes heavily loaded.

Extra workers help process bursts of work faster and exit when demand decreases.

All dynamically created threads are tracked and joined during shutdown.

## Build

Compile with GCC:

```bash
gcc -o threadpool threadpool.c -pthread
```

## Usage

Run the program with the number of initial worker threads:

```bash
./threadpool 4
```

You can pipe tasks into the program:

```bash
echo -e "hello\nworld\nfoo" | ./threadpool 4
```

Each worker processes a task and prints its `djb2` hash together with the original input.

Because tasks run concurrently, output order may vary between executions.

## Concepts Demonstrated

- POSIX threads
- Concurrency
- Producer-consumer pattern
- Race-condition prevention
- Critical sections
- Mutexes
- Condition variables
- Spurious wakeups
- Dynamic thread pools
- Graceful shutdown
- Resource cleanup

## Technologies

- C
- POSIX Threads
- GCC
- Linux
