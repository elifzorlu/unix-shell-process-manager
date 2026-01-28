#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <sstream>

using namespace std; 

Shell::Shell() : numJobs(1), prompt("==> ") {}

void Shell::run() {
    while (true) {
        checkBackgroundJobs();
        cout << prompt;

        string input_line;
        if (!getline(cin, input_line) || input_line == "exit") {
            if (!backgroundJobs.empty()) {
                cout << "Waiting for background jobs..." << endl;
                for (const auto& job : backgroundJobs) {
                    waitpid(job.pid, NULL, 0);
                }
            }
            break;
        }

        vector<string> tokens;
        stringstream ss(input_line);
        string word;
        while (ss >> word) {
            tokens.push_back(word);
        }

        if (tokens.empty()) continue;

        bool isBackground = false;
        if (tokens.back() == "&") {
            isBackground = true;
            tokens.pop_back();
        }

        // Built-in: cd
        if (tokens[0] == "cd") {
            if (tokens.size() < 2) cerr << "usage: cd <dir>" << endl;
            else if (chdir(tokens[1].c_str()) < 0) cout << "cd failed." << endl;
            continue;
        }

        // Built-in: set prompt
        if (tokens[0] == "set" && tokens.size() > 3 && tokens[1] == "prompt") {
            prompt = tokens[3] + " ";
            continue;
        }

        // Built-in: jobs
        if (tokens[0] == "jobs") {
            for (const auto& job : backgroundJobs) {
                cout << "[" << job.id << "] " << job.pid << " " << job.command << endl;
            }
            continue;
        }

        // Prepare args for execvp
        char *args[128];
        for (size_t i = 0; i < tokens.size(); i++) {
            args[i] = (char *)tokens[i].c_str();
        }
        args[tokens.size()] = NULL;

        executeCommand(args, isBackground, tokens[0]);
    }
}

void Shell::executeCommand(char** args, bool isBackground, string commandName) {
    timeval start;
    gettimeofday(&start, nullptr);

    int pid = fork();

    if (pid < 0) {
        cerr << "Fork error" << endl;
    } 
    else if (pid == 0) { // Child
        execvp(args[0], args);
        cerr << "Execvp error" << endl;
        exit(1);
    } 
    else { // Parent
        if (isBackground) {
            Job newJob;
            newJob.pid = pid;
            newJob.id = numJobs++;
            newJob.command = commandName;
            newJob.start = start;
            getrusage(RUSAGE_CHILDREN, &newJob.before);
            
            backgroundJobs.push_back(newJob);
            cout << "[" << newJob.id << "] " << pid << endl;
        } 
        else {
            rusage before{};
            getrusage(RUSAGE_CHILDREN, &before);

            int status;
            waitpid(pid, &status, 0);
            
            rusage after{};
            getrusage(RUSAGE_CHILDREN, &after);
            
            timeval end;
            gettimeofday(&end, nullptr);

            rusage delta = ru_delta(after, before);
            printStatus(start, end, delta);
        }
    }
}

void Shell::checkBackgroundJobs() {
    for (size_t i = 0; i < backgroundJobs.size(); i++) {
        int status;
        if (waitpid(backgroundJobs[i].pid, &status, WNOHANG) > 0) {
            timeval end;
            gettimeofday(&end, nullptr);

            rusage after{};
            getrusage(RUSAGE_CHILDREN, &after);
            rusage delta = ru_delta(after, backgroundJobs[i].before);

            cout << endl << "[" << backgroundJobs[i].id << "] " 
                 << backgroundJobs[i].pid << " Completed" << endl;
            
            printStatus(backgroundJobs[i].start, end, delta);

            backgroundJobs.erase(backgroundJobs.begin() + i);
            i--;
        }
    }
}

// === Utility Functions ===

void Shell::printStatus(timeval start, timeval end, rusage usage) {
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    if (microseconds < 0) {
        microseconds += 1000000;
        seconds -= 1;
    }
    long elapsed_ms = (seconds * 1000) + (microseconds / 1000);
    
    // Note: rusage time is already in seconds/microseconds
    long user_ms = (usage.ru_utime.tv_sec * 1000) + (usage.ru_utime.tv_usec / 1000);
    long sys_ms = (usage.ru_stime.tv_sec * 1000) + (usage.ru_stime.tv_usec / 1000);

    cout << "\n-- Statistics --" << endl;
    cout << "1. CPU time: " << user_ms + sys_ms << " ms" << endl;
    cout << "2. Elapsed time: " << elapsed_ms << " ms" << endl;
    cout << "3. Involuntary preemptions: " << usage.ru_nivcsw << endl;
    cout << "4. Voluntary context switches: " << usage.ru_nvcsw << endl;
    cout << "5. Major page faults: " << usage.ru_majflt << endl;
    cout << "6. Minor page faults: " << usage.ru_minflt << endl;
    cout << "7. Maximum resident size: " << usage.ru_maxrss << " kB" << endl;
}

timeval Shell::tv_sub(timeval a, timeval b) {
    timeval out;
    out.tv_sec = a.tv_sec - b.tv_sec;
    out.tv_usec = a.tv_usec - b.tv_usec;
    if (out.tv_usec < 0) {
        out.tv_usec += 1000000;
        out.tv_sec -= 1;
    }
    return out;
}

rusage Shell::ru_delta(const rusage& after, const rusage& before) {
    rusage d{};
    d.ru_utime = tv_sub(after.ru_utime, before.ru_utime);
    d.ru_stime = tv_sub(after.ru_stime, before.ru_stime);
    d.ru_nvcsw = after.ru_nvcsw - before.ru_nvcsw;
    d.ru_nivcsw = after.ru_nivcsw - before.ru_nivcsw;
    d.ru_majflt = after.ru_majflt - before.ru_majflt;
    d.ru_minflt = after.ru_minflt - before.ru_minflt;
    d.ru_maxrss = after.ru_maxrss; 
    return d;
}
