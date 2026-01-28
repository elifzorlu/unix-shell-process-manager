#ifndef SHELL_H
#define SHELL_H

#include <vector>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>

// Structure to track background processes
struct Job {
    int pid;
    int id;
    std::string command;
    timeval start;
    rusage before;
};

class Shell {
public:
    Shell();
    void run(); // Main loop

private:
    std::vector<Job> backgroundJobs;
    int numJobs;
    std::string prompt;

    // Core Helpers
    void executeCommand(char** args, bool isBackground, std::string commandName);
    void checkBackgroundJobs();
    
    // Statistics & Utilities
    void printStatus(timeval start, timeval end, rusage usage);
    timeval tv_sub(timeval a, timeval b);
    rusage ru_delta(const rusage& after, const rusage& before);
};

#endif
