/*
 * PCB.h
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * This file contains the functions relating to process control blocks (PCBs).
 */
#ifndef PCB_H_
#define PCB_H_

#define MAX_ARGS                        3 // maximum number of arguments to args array
#define LOWEST_PRIORITY                 3 // lowest priority (largest integer) that a process can be set to. priority will not be decremented below this value

#define REAL_TIME_PROCESS_PRIORITY      0 // priority of a real time process
#define REAL_TIME_PROCESS_MAX_MBYTES    64 // memory required for real time processes

#include <sys/types.h>
#include "MAB.h"

typedef struct PCB {
    unsigned int id; // unique identifier

    pid_t pid; // system process ID (0 when uninitialised)
    char * args[MAX_ARGS]; // program name and args - null terminated array

    unsigned int arrival_time; // time at which this process should begin execution
    unsigned int remaining_cpu_time; // remaining CPU time
    unsigned int priority; // priority of the process

    unsigned int num_printers; // number of printer resources required by the process
    unsigned int num_scanners; // number of scanner resources required by the process
    unsigned int num_modems; // number of modem resources required by the process
    unsigned int num_cds; // number of CD resources required by the process

    unsigned int mbytes; // size of memory required for this process
    MAB * memory; // the MAB assigned to this process

    struct PCB * prev; // prev PCB in the queue
    struct PCB * next; // next PCB in the queue
} PCB;

// Declaration to prevent compilation warnings
int kill(pid_t pid, int sig);

// Queue operations
PCB * create_null_PCB(void);
PCB * enqueue_PCB(PCB ** head, PCB ** pcb);
PCB * dequeue_PCB(PCB ** head);

// PCB operations
PCB * start_PCB(PCB **pcb);
PCB * decrement_remaining_cpu_time(PCB ** pcb);
PCB * lower_priority(PCB ** pcb);
PCB * suspend_PCB(PCB ** pcb);
PCB * restart_PCB(PCB ** pcb);
PCB * terminate_PCB(PCB ** pcb);
void free_PCB(PCB ** pcb);

#ifdef DEBUG
void print_PCB_queue(PCB * head);
void print_PCB(PCB * pcb);
#endif // #ifdef DEBUG
#endif // #ifndef PCB_H_
