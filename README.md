# C++ Micro-Shell (Systems Programming)

A lightweight, Unix-like command-line interpreter implemented in C++. This project demonstrates low-level process control, resource accounting, and signal handling without relying on standard shell libraries.

The codebase follows a modular, object-oriented design that encapsulates shell state and job management for improved maintainability and memory safety.

## ⚡ Core Systems Concepts

- **Process Management:** Uses the fork() + execvp() pattern to spawn and replace child processes.
- **Resource Accounting:** Tracks CPU time, context switches, and page faults via getrusage().
- **Asynchronous Execution:** Supports background jobs (&) using non-blocking waitpid(..., WNOHANG).
- **Signal Handling:** Prevents zombie processes through active monitoring of child process state.

## 📂 Project Structure

    /Micro-Shell-Project
    ├── include/       # Header files (interfaces)
    ├── src/           # Source code (implementation)
    ├── bin/           # Compiled executables
    └── Makefile       # Automated build script

## 🛠 Build & Run

### Prerequisites

- GCC/G++ (C++17 recommended)
- Linux/Unix environment (or WSL on Windows)

### Build & Execute

    make
    ./bin/microshell

## 🚀 Features

| Feature | Command | Description |
|------|------|------|
| Foreground execution | ls -la | Blocks until the process completes |
| Background execution | sleep 10 & | Runs process asynchronously |
| Process statistics | Automatic | Reports CPU and memory statistics |
| Job listing | jobs | Displays active background jobs |
| Built-in commands | cd, exit | Handled internally by the shell |

## 💻 Sample Usage

    ==> sleep 2 &
    [1] 14052
    ==> jobs
    [1] 14052 sleep
    ==>
    [1] 14052 Completed

    -- Statistics --
    CPU time: 0 ms
    Elapsed time: 2004 ms
    Involuntary context switches: 2
    Voluntary context switches: 11

## 🚧 Optimization Roadmap

- Pipe support using | and kernel pipes
- I/O redirection with > and < via dup2()
- Job control commands (fg, bg)
