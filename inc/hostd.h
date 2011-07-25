/*
    hostd.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the main functions for the host dispatcher.
*/
#ifndef HOSTD_H_
#define HOSTD_H_

#include "PCB.h"
#include "MAB.h"
#include "RAS.h"
#include "boolean.h"

#define AVAILABLE_MEMORY                1024            /* total available memory for all processes (in megabytes) */
#define RESERVED_MEMORY                 64              /* memory reserved for real time processes (in megabytes) */
#define AVAILABLE_PRINTERS              2               /* number of printer resources that the host dispatcher can allocate */
#define AVAILABLE_SCANNERS              1               /* number of scanner resources that the host dispatcher can allocate */
#define AVAILABLE_MODEMS                1               /* number of modem resources that the host dispatcher can allocate */
#define AVAILABLE_CDS                   2               /* number of CD resources that the host dispatcher can allocate */
#define NUM_FEEDBACK_QUEUES             LOWEST_PRIORITY /* number of feedback queues */

/* Global variables */
extern PCB *input_queue;
extern PCB *real_time_queue;
extern PCB *user_job_queue;
extern PCB *feedback_queue[NUM_FEEDBACK_QUEUES];
extern MAB *memory;
extern RAS *resources;
extern PCB *active;
extern unsigned int clock;

void tick(void);

void unload_pending_input_processes(void);
void unload_pending_user_processes(void);

boolean check_memory_and_resources(PCB *pcb);
boolean allocate_memory_and_resources(PCB *pcb);

PCB **next_queued_PCB(unsigned int min_priority);
boolean finished(void);

void print_help(void);
void print_status(void);
#endif