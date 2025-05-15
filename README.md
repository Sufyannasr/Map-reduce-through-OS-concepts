# Map-reduce-through-OS-concepts
This project implements a simplified MapReduce framework by leveraging core Operating System (OS) concepts such as processes, threads, inter-process communication (IPC), synchronization, and scheduling. The goal is to simulate how large-scale data processing tasks are performed in distributed systems using MapReduce, but within a single machine using OS-level mechanisms.

The system splits a large input dataset into smaller chunks, assigns "Map" tasks to worker processes or threads to perform data transformation, and then groups and sorts intermediate results before assigning "Reduce" tasks to aggregate the data. Synchronization primitives like mutexes, semaphores, or barriers ensure safe concurrent execution.

Key features:

Process-based or Thread-based parallelism

Named pipes (FIFOs), shared memory, or message queues for communication

Critical section handling using mutexes/semaphores

Scheduling and load balancing using OS-level controls

Custom input-output handling and intermediate storage

This project demonstrates how foundational OS mechanisms can be orchestrated to mimic the behavior of a distributed data processing model, offering insights into both parallel computing and systems programming.

