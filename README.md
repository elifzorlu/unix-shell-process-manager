# Unix Shell with Process & Resource Management

Implemented a Unix-like shell in C++ supporting foreground and background execution, job tracking, and detailed resource usage reporting.

## Features
- Executes external programs using `fork` and `exec`
- Supports background jobs and job tracking
- Measures CPU time (user + system), wall-clock time, and memory usage
- Reports context switches and page faults using `getrusage`
- Handles process synchronization with `wait` / `waitpid`

## Technical Highlights
- Used low-level Linux system calls for process control
- Implemented precise time measurement using `gettimeofday`
- Tracked per-job resource consumption and execution statistics
- Managed background jobs using internal job tables

## Example Output
## Example Output

```text
Statistics:
CPU time: 15 ms
Elapsed time: 22 ms
Voluntary context switches: 3
Involuntary context switches: 1
Major page faults: 0
Minor page faults: 124



## Tech Stack
- C++
- Linux
- POSIX system calls (`fork`, `exec`, `wait`, `getrusage`)

## Motivation
This project was built to gain a deeper understanding of how operating systems manage processes, scheduling, and resource accounting at the system-call level.
