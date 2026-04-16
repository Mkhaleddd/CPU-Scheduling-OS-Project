#pragma once
#include <vector>

struct Process {
    int pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int priority;
    int completionTime = 0;
    int waitingTime = 0;
    int turnaroundTime = 0;
};

extern std::vector<Process> processes;
