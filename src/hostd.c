/*
    hostd.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the main functions for the host dispatcher.
*/
#include "../inc/hostd.h"
#include "../inc/input.h"
#include "../inc/PCB.h"
#include "../inc/MAB.h"
#include "../inc/RAS.h"
#include "../inc/output.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Global variables */
PCB *input_queue;                                       /* the input (dispatcher) queue */
PCB *real_time_queue;                                   /* the real time queue */
PCB *user_job_queue;                                    /* the user job queue */
PCB *feedback_queue[NUM_FEEDBACK_QUEUES];               /* the feedback queues - note feedback_queue[i] stores processes with priority (i + 1) */
MAB *memory;                                            /* system memory */
RAS *resources;                                         /* system resources */
PCB *active;                                            /* active process */
unsigned int clock;                                     /* the clock */

/*
    ===========================================================
    main
    ===========================================================
    The main function for the host dispatcher.
    -----------------------------------------------------------
    argc:       The number of arguments.
    argv:       The arguments.
    -----------------------------------------------------------
    return value:
                An integer representing the exit status of 
                the program. (0 = success)
    -----------------------------------------------------------
*/
int main(int argc, char *argv[]) {
    /* print help */
    print_help();
    fprintf(__STANDARD_OUTPUT, "\n");

    /* initialise */
    input_queue = NULL;
    real_time_queue = NULL;
    user_job_queue = NULL;
    for (unsigned int i = 0; i < NUM_FEEDBACK_QUEUES; i++) {
        feedback_queue[i] = NULL;
    }
    active = NULL;
    clock = 0;
    
    FILE *input;                                        /* file to read input from */
    
    /* initialise memory resources */
    memory = create_null_MAB();
    memory->size = AVAILABLE_MEMORY;
    
    /* initialise resources */
    resources = create_resources(AVAILABLE_PRINTERS, AVAILABLE_SCANNERS, AVAILABLE_MODEMS, AVAILABLE_CDS);
    
    /* open file */
    if (argc < 2) {
        fprintf(__ERROR_OUTPUT, "No input file specified.\n");
        exit(1);
    } else {
        if (!(input = fopen(argv[1], "r"))) {
            fprintf(__ERROR_OUTPUT, "Unable to open input file '%s' for reading.\n", argv[1]);
            exit(1);
        }
    }
    
    /* fill input queue from dispatch list file */
    input_queue = read_process_list(input);
    
    /* close the input file */
    if (input != NULL) {
        fclose(input);
        input = NULL;
    }
#ifdef DEBUG

    fprintf(__DEBUG_OUTPUT, "Finished parsing input file. Input queue: ");
    print_PCB_queue(input_queue);
    fprintf(__DEBUG_OUTPUT, ".\n");
#endif

    /* print initial status */
    print_status();

    /* start and run dispatch timer - this is the main host dispatcher action and won't return until the host dispatcher has completed */
    tick();
    
    /* Output the total elapsed time when the host dispatcher has finished */
    fprintf(__STANDARD_OUTPUT, "\nFinished processing. Total elapsed time is %d.\n", clock);

    /* Clean up */
    if (active != NULL) {
        terminate_PCB(&active);
        free_PCB(&active);
    }
    
    while (input_queue != NULL) {
        PCB *tmp = dequeue_PCB(&input_queue);
        terminate_PCB(&tmp);
        free_PCB(&tmp);
    }
    
    while (user_job_queue != NULL) {
        PCB *tmp = dequeue_PCB(&input_queue);
        terminate_PCB(&tmp);
        free_PCB(&tmp);
    }
    
    while (real_time_queue != NULL) {
        PCB *tmp = dequeue_PCB(&real_time_queue);
        terminate_PCB(&tmp);
        free_PCB(&tmp);
    }
    
    for (unsigned int i = 0; i < NUM_FEEDBACK_QUEUES; i++) {
        while (feedback_queue[i] != NULL) {
            PCB *tmp = dequeue_PCB(&feedback_queue[i]);
            terminate_PCB(&tmp);
            free_PCB(&tmp);
        }
    }
    
    MAB *m = memory;
    MAB *m_next = NULL;
    while (m != NULL) {
        m_next = m->next;
        free(memory);
        m = m_next;
        m_next = NULL;
       }
    
    RAS *r = resources;
    RAS *r_next = NULL;
    while (r != NULL) {
        r_next = r->next;
        free(r);
        r = r_next;
        r_next = NULL;
    }
}

/*
    ===========================================================
    tick
    ===========================================================
    The main functional component of the host dispatcher. This
    function "ticks" time and initiates the relevant process,
    memory and resource operation at each clock tick.
    
    This function uses global variables for the clock, memory
    resources, resources, input queue, user job queue, real
    time queue, feedback queues and active process.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void tick(void) {
    PCB **next = NULL;                                  /* the next process to execute - from either the real time queue or a feedback queue */
    
    /* a blank line will separate ticks in the output */
    fprintf(__STANDARD_OUTPUT, "\n");
    
    /* unload pending processes from the associated queues */
    unload_pending_input_processes();
    unload_pending_user_processes();

    /* if there is an active process, decrement its remaining CPU time (terminating the process if time has expired) */
    if (active != NULL) {
        active = decrement_remaining_cpu_time(&active);
        
        /* make sure there is still an active process - ie. that the active process hasn't terminated */
        if (active != NULL) {
            /* if the running process is a real time process then it does not need to be suspended */
            if (active->priority != REAL_TIME_PROCESS_PRIORITY) {
                /* check if there are any ready queued processes (with an equal or higher priority) on the feedback queues */
                if ((next = next_queued_PCB(active->priority)) != NULL) {
                    /* suspend the active process */
                    PCB *p = suspend_PCB(&active);
                    active = NULL;
                    p = lower_priority(&p);
                
#ifdef DEBUG
                    fprintf(__DEBUG_OUTPUT, "Enqueuing PCB %d onto feedback queue RQ%d.\n", p->id, p->priority);
#endif
                    enqueue_PCB(&feedback_queue[p->priority - 1], &p);
                }
#ifdef DEBUG
                else {
                    fprintf(__DEBUG_OUTPUT, "No ready PCBs with priority greater than or equal to %d in feedback queues. No need to suspend active PCB %d.\n", active->priority, active->id);
                }
#endif
            }
#ifdef DEBUG
            else {
                fprintf(__DEBUG_OUTPUT, "PCB %d is a real time process. No need to suspend process.\n", active->id);
            }
#endif
        }
    }
    
    /* if there is no next PCB specified, get the next queued PCB of any priority */
    if (next == NULL) {
        next = next_queued_PCB(LOWEST_PRIORITY);
    }
    
    /* if there is no active process but there is a ready queued process, then start/resume the next rocess */
    if ((active == NULL)) {
        if (next != NULL) {
            /* remove next PCB from its queue and set is as the active PCB */
            active = dequeue_PCB(next);
            
            /* process has been started if its PID is non-zero */
            if (active->pid != 0) {
                /* resume the active process */
                restart_PCB(&active);
            } else {
                /* if this is a real time process, allocate memory and resources (in real time, obviously) */
                if (active->pid == REAL_TIME_PROCESS_PRIORITY) {
                    if (!allocate_memory_and_resources(active)) {
                        fprintf(__ERROR_OUTPUT, "Failed to allocate memory and resources for real time process PCB %d. This process will not be executed.\n", active->id);
                        free_PCB(&active);
                        active = NULL;
                    }
#ifdef DEBUG
                    else {
                        fprintf(__DEBUG_OUTPUT, "Allocated memory and resources for real time process PCB %d.\n", active->id);
                    }
#endif
                }
                
                /* start the active process */
                start_PCB(&active);
            }
        }
#ifdef DEBUG
        else {
            fprintf(__DEBUG_OUTPUT, "There is no process ready to execute at this stage.\n");
        }
#endif
    }
    
    /* Increment the clock and sleep for one second to emulate a real 'tick' */
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Ticking.\n");
#endif
    clock++;
    sleep(1);

    /* output current dispatcher status */
    print_status();
    
    /* Keep ticking if there are still remaining queued/active processes */
    if (!finished()) {
        tick();
    }
}

/*
    ===========================================================
    unload_pending_input_processes
    ===========================================================
    Unload any pending processes from the input queue to the
    user job queue or real time queue.
    
    This function uses global variables for the clock, input 
    queue and user job queue.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void unload_pending_input_processes(void) {
    PCB *input = input_queue;                           /* for iterating through input queue */
#ifdef DEBUG

    fprintf(__DEBUG_OUTPUT, "Unloading any pending processes from the input queue.\n");
#endif

    /* unload processes from the input queue to the user job queue whilst there are processes on the input queue and the process is ready (arrival time has been reached) */
    while (input != NULL) {
        PCB *prev = NULL;                               /* for restoring queue head pointer */
        PCB *next = NULL;                               /* for restoring queue head pointer */
        
        /* check that PCB is ready */
        if (input->arrival_time <= clock) {
            /* capture next and previous PCBs */
            prev = input->prev;
            next = input->next;
            
            /* remove the process from the input queue */
            PCB *p = dequeue_PCB(&input);
            
            /* check if PCB is a real time process */
            if (p->priority == REAL_TIME_PROCESS_PRIORITY) {
#ifdef DEBUG
                fprintf(__DEBUG_OUTPUT, "Unloading PCB %d to real time queue.\n", p->id);
                
#endif
                /* add the process to the real time queue */
                enqueue_PCB(&real_time_queue, &p);
            } else {
#ifdef DEBUG
                fprintf(__DEBUG_OUTPUT, "Unloading PCB %d to user job queue.\n", p->id);
                
#endif
                /* add the process to the user job queue */
                enqueue_PCB(&user_job_queue, &p);
            }
            
            /* restore input queue pointer */
            if (prev == NULL) {
                input_queue = next;
            }
        } else {
#ifdef DEBUG
            fprintf(__DEBUG_OUTPUT, "PCB %d is not yet ready and will remain on the input queue.\n", input->id);

#endif
            /* go to next input PCB */
            input = input->next;
        }
    }
}

/*
    ===========================================================
    unload_pending_user_processes
    ===========================================================
    Unload any pending processes from the user job queue to
    the relevant feedback queue. The memory and resources
    required by a process are allocated to the process before
    it is unloaded to the feedback queue.
    
    This function uses global variables for the user job queue
    and the feedback queues.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void unload_pending_user_processes(void) {
    PCB *user_job = user_job_queue;                     /* for iterating through the user job queue */
#ifdef DEBUG

    fprintf(__DEBUG_OUTPUT, "Unloading any pending processes from the user job queue.\n");
#endif

    /* unload processes from the user job queue to the feedback queue while there are processes on the user job queue and there is enough memory for the new process */
    while (user_job != NULL) {
        PCB *prev = NULL;                               /* for restoring queue head pointer */
        PCB *next = NULL;                               /* for restoring queue head pointer */
        
        /* check that the system can provide the memory and resources that the process requires) */
        if (check_memory_and_resources(user_job)) {
            /* check that memory and resources can be allocated for this PCB */
            if (allocate_memory_and_resources(user_job)) {
#ifdef DEBUG
                fprintf(__DEBUG_OUTPUT, "Allocated memory and resources for process PCB %d.\n", user_job->id);
                fprintf(__DEBUG_OUTPUT, "Unloading PCB %d to feedback queue RRQ%d.\n", user_job->id, user_job->priority);
        
#endif
                /* capture next and previous PCBs */
                prev = user_job->prev;
                next = user_job->next;

                /* remove the process from the user job queue and add the process to the appropriate feedback queue */
                PCB *p = dequeue_PCB(&user_job);
                enqueue_PCB(&feedback_queue[p->priority - 1], &p);
                
                /* if this PCB was the head of the user job queue, then we must restore user job queue pointer */
                if (prev == NULL) {
                    user_job_queue = next;
                }
            } else {
#ifdef DEBUG
                fprintf(__DEBUG_OUTPUT, "Unable to allocate memory and resources for PCB %d at this stage.\n", user_job->id);
                
#endif
                /* go to next user job */
                user_job = user_job->next;
            }
        } else {
            fprintf(__ERROR_OUTPUT, "PCB %d requested more memory or resources than the system can provide. This process will not be executed.\n", user_job->id);
            
            /* delete PCB */
            PCB *p = dequeue_PCB(&user_job);
            free_PCB(&p);
            
            /* if this PCB was the head of the user job queue, then we must restore user job queue pointer */
            if (prev == NULL) {
                user_job_queue = next;
            }
        }
    }
}

/*
    ===========================================================
    check_memory_and_resources
    ===========================================================
    Check that the memory and resources required by a process
    do not exceed the memory/resources that the system can 
    provide.
    
    This function will return false if the process requests
    more memory or resources than the system can provide.
    -----------------------------------------------------------
    PCB:
                Pointer to the process for which to allocate
                memory and resources.
    -----------------------------------------------------------
    Return value:
                A boolean value indicating that the system is
                able to provide (however, not necessarily at 
                this instant) the memory and resources that the
                process requires.
    -----------------------------------------------------------
*/
boolean check_memory_and_resources(PCB *pcb) {    
    /* check memory */
    if (pcb->mbytes > (AVAILABLE_MEMORY - RESERVED_MEMORY)) {
        return false;
    }
    
    /* check printers */
    if (pcb->num_printers > AVAILABLE_PRINTERS) {
        return false;
    }
    
    /* check scanners */
    if (pcb->num_scanners > AVAILABLE_SCANNERS) {
        return false;
    }
    
    /* check modems */
    if (pcb->num_modems > AVAILABLE_MODEMS) {
        return false;
    }
    
    /* check CDs */
    if (pcb->num_cds > AVAILABLE_CDS) {
        return false;
    }
    
    return true;
}

/*
    ===========================================================
    allocate_memory_and_resources
    ===========================================================
    Allocates the memory and resources required by a process.
    If any of the allocations fail, then the function will
    unallocate all memory/resources that have been allocated.
    
    This function will return false if the process requests
    more memory or resources than the system can provide.
    -----------------------------------------------------------
    PCB:
                Pointer to the process for which to allocate
                memory and resources.
    -----------------------------------------------------------
    Return value:
                A boolean value indicating whether the memory
                and resources were successfully allocated.
    -----------------------------------------------------------
*/
boolean allocate_memory_and_resources(PCB *pcb) {
    /* free any memory and resources already allocated to process */
    if (pcb->memory != NULL) {
        pcb->memory = mem_free(pcb->memory);
    }
    resource_free(pcb);
    
    /* attempt to allocate memory (only if requested memory is nonzero) */
    if (pcb->mbytes > 0) {
        if ((pcb->memory = mem_alloc(pcb->mbytes)) == NULL) {
            /* allocation failed - roll back */
            return false;
        }
    }
    
    /* check that there is still enough memory for a real time process (unless pcb IS a real time process) */
    if (pcb->priority != REAL_TIME_PROCESS_PRIORITY) {
        MAB *m = memory;                                    /* for iterating through memory list */
        unsigned int largest_MAB_size = 0;                  /* size of the largest MAB */
        while (m != NULL) {
            if (!m->allocated && (m->size >= largest_MAB_size)) {
                largest_MAB_size = m->size;
            }
            
            /* go to the next MAB */
            m = m->next;
        }
        if (largest_MAB_size < RESERVED_MEMORY) {
            /* allocation failed - roll back */
            pcb->memory = mem_free(pcb->memory);
            return false;
        }
    }
    
    /* attempt to allocate printers */
    for (unsigned int i = 0; i < pcb->num_printers; i++) {
        if (resource_alloc(Printer_Resource, pcb) == NULL) {
            /* allocation failed - roll back */
            pcb->memory = mem_free(pcb->memory);
            resource_free(pcb);
            return false;
        }
    }
    
    /* attempt to allocate scanners */
    for (unsigned int i = 0; i < pcb->num_scanners; i++) {
        if (resource_alloc(Scanner_Resource, pcb) == NULL) {
            /* allocation failed - roll back */
            pcb->memory = mem_free(pcb->memory);
            resource_free(pcb);
            return false;
        }
    }
    
    /* attempt to allocate modems */
    for (unsigned int i = 0; i < pcb->num_modems; i++) {
        if (resource_alloc(Modem_Resource, pcb) == NULL) {
            /* allocation failed - roll back */
            pcb->memory = mem_free(pcb->memory);
            resource_free(pcb);
            return false;
        }
    }
    
    /* attempt to allocate CDs */
    for (unsigned int i = 0; i < pcb->num_cds; i++) {
        if (resource_alloc(CD_Resource, pcb) == NULL) {
            /* allocation failed - roll back */
            pcb->memory = mem_free(pcb->memory);
            resource_free(pcb);
            return false;
        }
    }
    
    /* memory and resource allocation was successful */
    return true;
}

/*
    ===========================================================
    next_queued_PCB
    ===========================================================
    Gets the next queued PCB with the highest priority from the
    real time queue or the relevant feedback queue. It will 
    only return a PCB if that PCB is ready to be started (clock
    has reached arrival time). This function does not dequeue 
    the PCB from its queue. 
    
    A minimum priority can be specified such that a PCB will 
    not be returned unless its priority is equal to or greater
    than (numerically less than) the minimum priority.
    
    This function uses global variables for the clock, real 
    time queue and feedback queues.
    -----------------------------------------------------------
    min_priority:
                The PCB returned must have a priority greater
                than (numerically less than) or equal to this
                priority.
    -----------------------------------------------------------
    Return value:
                The next PCB with the highest priority (and a 
                priority at least equal to min_priority) from 
                the relevant feedback queue. Returns NULL if 
                there are no queued processes.
    -----------------------------------------------------------
*/
PCB **next_queued_PCB(unsigned int min_priority) {
    /* check if there are any queued real time proceeses (these processes will always have priority of at least min_priority so no need to check priority) */
    if ((real_time_queue != NULL) && (real_time_queue->arrival_time <= clock)) {
        return &real_time_queue;
    }
    
    /* check each feedback queue, from highest priority to lowest priority */
    unsigned int upper_limit;
    if (min_priority > NUM_FEEDBACK_QUEUES) {
        upper_limit = NUM_FEEDBACK_QUEUES;
    } else {
        upper_limit = min_priority;
    }
    for (unsigned int i = 0; i < upper_limit; i++) {
        /* if the feedback queue is not empty (head is not null) and the head element is ready, return the head element (if the head element is ready) */
        if ((feedback_queue[i] != NULL) && (feedback_queue[i]->arrival_time <= clock)) {
            return &feedback_queue[i];
        }
    }
    
    /* no queued PCBs are ready */
    return NULL;
}

/*
    ===========================================================
    finished
    ===========================================================
    Checks if the host dispatcher has completed by inspecting
    the active and queued processes.
    
    This function uses global variables for the active process,
    input queue, user job queue, real time queue and feedback 
    queues.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    Return value:
                A boolean value indicating whether the host
                dipatcher has completed.
    -----------------------------------------------------------
*/
boolean finished(void) {
    /* check for active process */
    if (active != NULL) {
        return false;
    }
    
    /* check if there are any queued real time proceeses */
    if (real_time_queue != NULL) {
        return false;
    }
    
    /* check each feedback queue, from highest priority to lowest priority */
    for (unsigned int i = 0; i < NUM_FEEDBACK_QUEUES; i++) {
        if (feedback_queue[i] != NULL) {
            return false;
        }
    }
    
    /* check if there are any queued user job proceeses */
    if (user_job_queue != NULL) {
        return false;
    }
    
    /* check if there are any queued input proceeses */
    if (input_queue != NULL) {
        return false;
    }
    
    /* no queued PCBs - host dispatcher has completed */
    return true;
}

/*
    ===========================================================
    print_help
    ===========================================================
    Prints a help file explaining various terminology and 
    abbreviations used within the program.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    (no return value).
    -----------------------------------------------------------
*/
void print_help(void) {
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
    fprintf(__STANDARD_OUTPUT, "ABBREVIATIONS AND TERMINOLOGY\n");
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
    fprintf(__STANDARD_OUTPUT, "FIELDS\n");
    fprintf(__STANDARD_OUTPUT, "\tID\t\tUnique identifier.\n");
    fprintf(__STANDARD_OUTPUT, "\tPID\t\tProcess ID.\n");
    fprintf(__STANDARD_OUTPUT, "\tARRIVE\t\tProcess arrival time.\n");
    fprintf(__STANDARD_OUTPUT, "\tREMAIN\t\tRemaining CPU time.\n");
    fprintf(__STANDARD_OUTPUT, "\tPRIOR\t\tProcess priority.\n");
    fprintf(__STANDARD_OUTPUT, "\tMB\t\tMegabytes of memory required by process.\n");
    fprintf(__STANDARD_OUTPUT, "\tMAB ID\t\tMemory allocation block currently allocated to process.\n");
    fprintf(__STANDARD_OUTPUT, "\tPRINT\t\tNumber of printer resources required by process.\n");
    fprintf(__STANDARD_OUTPUT, "\tSCAN\t\tNumber of scanner resources required by process.\n");
    fprintf(__STANDARD_OUTPUT, "\tMODEM\t\tNumber of modem resources required by process.\n");
    fprintf(__STANDARD_OUTPUT, "\tCD\t\tNumber of CD resources required by process.\n");
    fprintf(__STANDARD_OUTPUT, "\tSTATUS\t\tCurrent status of process.\n");
    fprintf(__STANDARD_OUTPUT, "\n");
    
    fprintf(__STANDARD_OUTPUT, "STATUSES\n");
    fprintf(__STANDARD_OUTPUT, "\tACTIVE\t\tProcess is currently being exectuted.\n");
    fprintf(__STANDARD_OUTPUT, "\tQUEUED-RT\tProcess is queued in the real time queue.\n");
    fprintf(__STANDARD_OUTPUT, "\tQUEUED-RRQx\tProcess is queued in the feedback (round robin) queue with priority x and \n\t\t\thas not been started.\n");
    fprintf(__STANDARD_OUTPUT, "\tSUSPENDED-RRQx\tProcess is queued in the feedback (round robin) queue with priority x and \n\t\t\thas been suspended.\n");
    fprintf(__STANDARD_OUTPUT, "\tPENDING\t\tProcess is in the user job queue and has not yet been allocated memory or \n\t\t\tresources.\n");
    fprintf(__STANDARD_OUTPUT, "\tUNLOADED\tProcess is in the input dispatcher queue and is not ready to be executed \n\t\t\tyet.\n");
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
}

/*
    ===========================================================
    print_status
    ===========================================================
    Prints the current status of the host dispatcher and all
    processes.
    
    This function uses global variables for the clock, memory
    resources, resources, input queue, user job queue, real
    time queue, feedback queues and active process.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    (no return value).
    -----------------------------------------------------------
*/
void print_status(void) {
    PCB *input = input_queue;                           /* to iterate through the input queue */
    PCB *user_job = user_job_queue;                     /* to iterate through the user job queue */
    PCB *real_time = real_time_queue;                   /* to iterate through the real time queue */
    
    /* Output header */
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
    fprintf(__STANDARD_OUTPUT, "Time:\t\t\t%d\n", clock);
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
    fprintf(__STANDARD_OUTPUT, "ID\t| PID\tARRIVE\tREMAIN\tPRIOR\t| MB\tMAB ID\t| PRINT\tSCAN\tMODEM\tCD\t| STATUS\n");
    fprintf(__STANDARD_OUTPUT, "----------------------------------------------------------------------------------------------------\n");
    
    /* Output process information */
    if (!finished()) {
        if (active != NULL) {
            if (active->memory != NULL) {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| ACTIVE\n", active->id, active->pid, active->arrival_time, active->remaining_cpu_time, active->priority, active->mbytes, active->memory->id, active->num_printers, active->num_scanners, active->num_modems, active->num_cds);
            } else {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| ACTIVE\n", active->id, active->pid, active->arrival_time, active->remaining_cpu_time, active->priority, active->mbytes, active->num_printers, active->num_scanners, active->num_modems, active->num_cds);
            }
        }
    
        while (real_time != NULL) {
            if (real_time->memory != NULL) {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| QUEUED-RT\n", real_time->id, real_time->pid, real_time->arrival_time, real_time->remaining_cpu_time, real_time->priority, real_time->mbytes, real_time->memory->id, real_time->num_printers, real_time->num_scanners, real_time->num_modems, real_time->num_cds);
            } else {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| QUEUED-RT\n", real_time->id, real_time->pid, real_time->arrival_time, real_time->remaining_cpu_time, real_time->priority, real_time->mbytes, real_time->num_printers, real_time->num_scanners, real_time->num_modems, real_time->num_cds);
            }
            real_time = real_time->next;
        }
    
        for (unsigned int i = 0; i < NUM_FEEDBACK_QUEUES; i++) {
            if (feedback_queue[i] != NULL) {
                PCB *feedback = feedback_queue[i];      /* to iterate through current feedback queue */
                while (feedback != NULL) {
                    /* check if the process has been started (PID is non-zero) */
                    if (feedback->pid != 0) {
                        if (feedback->memory != NULL) {
                            fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| SUSPENDED-RRQ%d\n", feedback->id, feedback->pid, feedback->arrival_time, feedback->remaining_cpu_time, feedback->priority, feedback->mbytes, feedback->memory->id, feedback->num_printers, feedback->num_scanners, feedback->num_modems, feedback->num_cds, i + 1);
                        } else {
                            fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| SUSPENDED-RRQ%d\n", feedback->id, feedback->pid, feedback->arrival_time, feedback->remaining_cpu_time, feedback->priority, feedback->mbytes, feedback->num_printers, feedback->num_scanners, feedback->num_modems, feedback->num_cds, i + 1);
                        }
                    } else {
                        if (feedback->memory != NULL) {
                            fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| QUEUED-RRQ%d\n", feedback->id, feedback->pid, feedback->arrival_time, feedback->remaining_cpu_time, feedback->priority, feedback->mbytes, feedback->memory->id, feedback->num_printers, feedback->num_scanners, feedback->num_modems, feedback->num_cds, i + 1);
                        } else {
                            fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| QUEUED-RRQ%d\n", feedback->id, feedback->pid, feedback->arrival_time, feedback->remaining_cpu_time, feedback->priority, feedback->mbytes, feedback->num_printers, feedback->num_scanners, feedback->num_modems, feedback->num_cds, i + 1);
                        }
                    }
                    feedback = feedback->next;
                }
            }
        }
    
        while (user_job != NULL) {
            if (user_job->memory != NULL) {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| PENDING\n", user_job->id, user_job->pid, user_job->arrival_time, user_job->remaining_cpu_time, user_job->priority, user_job->mbytes, user_job->memory->id, user_job->num_printers, user_job->num_scanners, user_job->num_modems, user_job->num_cds);
            } else {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| PENDING\n", user_job->id, user_job->pid, user_job->arrival_time, user_job->remaining_cpu_time, user_job->priority, user_job->mbytes, user_job->num_printers, user_job->num_scanners, user_job->num_modems, user_job->num_cds);
            }
            user_job = user_job->next;
        }

        while (input != NULL) {
            if (input->memory != NULL) {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t%d\t| %d\t%d\t%d\t%d\t| UNLOADED\n", input->id, input->pid, input->arrival_time, input->remaining_cpu_time, input->priority, input->mbytes, input->memory->id, input->num_printers, input->num_scanners, input->num_modems, input->num_cds);
            } else {
                fprintf(__STANDARD_OUTPUT, "%d\t| %d\t%d\t%d\t%d\t| %d\t(null)\t| %d\t%d\t%d\t%d\t| UNLOADED\n", input->id, input->pid, input->arrival_time, input->remaining_cpu_time, input->priority, input->mbytes, input->num_printers, input->num_scanners, input->num_modems, input->num_cds);
            }
            input = input->next;
        }
    } else {
        fprintf(__STANDARD_OUTPUT, "(none)\n");
    }
    
    /* End of output */
    fprintf(__STANDARD_OUTPUT, "====================================================================================================\n");
    
    /* Output memory and resources */
    print_MAB_list(memory);
    print_RAS_list(resources);
}