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
enum SchedulerType {
    SCHED_FCFS,
    SCHED_SJF_PREEMPTIVE,
    SCHED_SJF_NON_PREEMPTIVE,
    SCHED_PRIORITY_PREEMPTIVE,
    SCHED_PRIORITY_NON_PREEMPTIVE,
    SCHED_ROUND_ROBIN
};

extern std::vector<Process> processes;
extern SchedulerType currentScheduler;
extern int timeQuantum;
