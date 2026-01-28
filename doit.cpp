#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <vector>
#include <sstream>

using namespace std;

// to track each process' data
struct Job {
    int pid, id;
    string command;
    timeval start;
    rusage before;
};

vector<Job> backgroundJobs;
int numJobs = 1;


// subtracts after - before time value to calculate the elapsed time.
timeval tv_sub(timeval a , timeval b ){
    timeval out ;
    out.tv_sec  = a.tv_sec  - b.tv_sec ;
    out.tv_usec = a.tv_usec - b.tv_usec ;
    if (out.tv_usec < 0){ // handles carry over
        out.tv_usec += 1000000 ;
        out.tv_sec  -= 1 ;
    }
    return out ;
}

// takes CPU snapshots before and after from rusage and uses timeval math.
rusage ru_delta(const rusage& after , const rusage& before){
    rusage d{} ;

    d.ru_utime = tv_sub(after.ru_utime , before.ru_utime) ;
    d.ru_stime = tv_sub(after.ru_stime , before.ru_stime) ;

    d.ru_nvcsw  = after.ru_nvcsw  - before.ru_nvcsw ;
    d.ru_nivcsw = after.ru_nivcsw - before.ru_nivcsw ;
    d.ru_majflt = after.ru_majflt - before.ru_majflt ;
    d.ru_minflt = after.ru_minflt - before.ru_minflt ;

    d.ru_maxrss = after.ru_maxrss ;
    return d ;
}

/* printStatus function: helper function that measures and prints the statistics for a finished process.

*/
void printStatus(timeval start, timeval end, rusage usage) {
    long seconds = end.tv_sec - start.tv_sec;
    long miliseconds = end.tv_usec - start.tv_usec;
    
    // the carry over math is handled 
    if (miliseconds < 0) {
        miliseconds += 1000000;
        seconds -= 1;
    }

    long elapsed_miliseconds = (seconds * 1000) + (miliseconds / 1000);

    long userMiliseconds = usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000;
    long systemMiliseconds = usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000;

    cout << endl << "-- Statistics --" << endl;
    cout << "1. CPU time: " << userMiliseconds + systemMiliseconds << " ms" << endl;
    cout << "2. Elapsed time: " << elapsed_miliseconds << " ms" << endl;
    cout << "3. Involuntary preemptions: " << usage.ru_nivcsw << endl;
    cout << "4. Voluntary context switches: " << usage.ru_nvcsw << endl;
    cout << "5. Major page faults: " << usage.ru_majflt << endl;
    cout << "6. Minor page faults: " << usage.ru_minflt << endl;
    cout << "7. Maximum resident size used: " << usage.ru_maxrss << " kB" << endl;
}

/*
  executeCommand function : run one command based on the shell rules.
 this function handles the fork/execvp/wait pattern
 - Calls fork() to create a new child process
 - The child command uses execvp() to run the command 
 - Parent process decides on what to do based on the "isBackground" flag:
    if isBackground true, then the job info is saved into the backgroundJobs vector and prints the PID.
    if isBackground false, then waitpid() is used to wait till the process finishes then the resources get printed.
*/
void executeCommand(char** args, bool isBackground, string commandName) {
    timeval start; 
    gettimeofday(&start, nullptr); // start wall clock

    int pid = fork(); // split the process

    if (pid < 0) {
        cerr << "Fork error" << endl;
    } 
    else if (pid == 0) { // child
        execvp(args[0], args); // replace itself with the command
        cerr << "Execvp error" << endl; // if execvp fails
        exit(1);
    } 
    else { // parent= decide background or foreground
        if (isBackground) { // saves the PID to check back later with waitpid
            Job newJob;
            newJob.pid = pid;
            newJob.id = numJobs++;
            newJob.command = commandName;
            newJob.start = start;
            
            // Capture usage specifically for this background job path
            getrusage(RUSAGE_CHILDREN, &newJob.before);

            backgroundJobs.push_back(newJob);
            cout << "[" << newJob.id << "] " << pid << endl;
        } 
        else
        {
            // Capture usage right before waiting for foreground
            rusage before{};
            getrusage(RUSAGE_CHILDREN, &before);

            int status;
            waitpid(pid, &status, 0); // wait until it finishes.
            
            rusage after{};
            getrusage(RUSAGE_CHILDREN, &after);

            timeval end;
            gettimeofday(&end, nullptr);

            rusage delta = ru_delta(after, before);
            printStatus(start, end, delta);
        }
     }
 }

/* - backgroundJobChecker function is designed to loop through the the list of running background
* jobs to detect if any has finished.
* -  WOHANG flag is used as it is essential for checking the status without blocking the entire shell.
*/
void backgroundJobChecker() {
    for (int i = 0; i < backgroundJobs.size() ;i++){
        int status;

        int done = waitpid(backgroundJobs[i].pid , &status , WNOHANG) ;
        if (done > 0) {
            timeval end;
            gettimeofday(&end, nullptr);

            rusage after{} ;
            getrusage(RUSAGE_CHILDREN , &after) ;
            rusage delta = ru_delta(after , backgroundJobs[i].before) ;

            cout << endl << "[" << backgroundJobs[i].id << "] "
                 << backgroundJobs[i].pid << " Completed" << endl;

            printStatus(backgroundJobs[i].start, end, delta);
            
            backgroundJobs.erase(backgroundJobs.begin() + i);
            i--; 
        }
        
    }
}

int main(int argc, char **argv) {

    if (argc > 1) {
        executeCommand(&argv[1], false, argv[1]);
        return 0;
    }

    string prompt = "==> ";
    
    while (true) {
        backgroundJobChecker();
        cout << prompt;

        string input_line;
        if (!getline(cin, input_line) || input_line == "exit") {
            if (!backgroundJobs.empty()) {
                cout << "Waiting for background jobs..." << endl;
                for (int i = 0; i < backgroundJobs.size(); i++) {
                    waitpid(backgroundJobs[i].pid, NULL, 0);
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

        if (tokens.empty()) {
            continue;
        }

        bool isBackground = false;

        if (tokens.back() == "&") {
            isBackground = true;
            tokens.pop_back();
        }

        if (tokens[0] == "cd") {
            if (tokens.size() < 2) {
                cerr << "usage: cd <dir>" << endl;
            } 
            else if (chdir(tokens[1].c_str()) < 0) {
                cout << "cd failed." << endl;
            }
            continue;
        }

        if (tokens[0] == "set" && tokens.size() > 3 && tokens[1] == "prompt") {
            prompt = tokens[3] + " ";
            continue;
        }

        if (tokens[0] == "jobs") {
            for (int i = 0; i < backgroundJobs.size(); i++) {
                cout << "[" << backgroundJobs[i].id << "] " << backgroundJobs[i].pid << " " << backgroundJobs[i].command << endl;
            }
            continue;
        }

        char *args[128]; // makes the exact format execvp expects
        for (int i = 0; i < tokens.size(); i++) {
            args[i] = (char *)tokens[i].c_str();
        }
        args[tokens.size()] = NULL; // make the last argument as NULL for termination

        executeCommand(args, isBackground, tokens[0]);
    }

    return 0;
}


/*

Observation of comparing to Linux Shell:

- My shell works like a simplified version a standard Linux shell. It handles the read-parse-execute loop and it can run 
background tasks using "&" without freezing.

Feature Comparison:
- NO "glue": my shell cannot chain commands with pipe (|) or cannot redirect output to files. 
My shell just sees them as regular text. 

- My shell requires typing the full command whereas linux shell has command history.

- Error handling: My shell still prints a message but keeps running if a command that doesn't exist is entered.
However, it is not as robust as bash which Linus has.

- My shell prints detailed stats after every command while Linux doesn't, adding a convenient side to my shell for testing how
heavy a program is.


*/
