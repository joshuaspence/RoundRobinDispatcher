/*
    PCB.c

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the functions relating to process control blocks (PCBs).
*/
#include "../inc/PCB.h"
#include "../inc/output.h"
#include "../inc/MAB.h"
#include "../inc/RAS.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

static unsigned int _id = 1;                            /* counter used to track assigned IDs */

/*
    ===========================================================
    create_null_PCB
    ===========================================================
    Creates a new PCB, with all values initialised to logical
    default values.
    -----------------------------------------------------------
    (no parameters)
    -----------------------------------------------------------
    return value:
                A pointer to the new PCB.
    -----------------------------------------------------------
*/
PCB *create_null_PCB(void) {
    PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
    
    new_pcb->id = (_id++);
    new_pcb->pid = 0;
    
    new_pcb->arrival_time = 0;
    new_pcb->remaining_cpu_time = 0;
    new_pcb->priority = 0;
    
    new_pcb->num_printers = 0;
    new_pcb->num_scanners = 0;
    new_pcb->num_modems = 0;
    new_pcb->num_cds = 0;
    
    new_pcb->mbytes = 0;
    new_pcb->memory = NULL;
    
    new_pcb->prev = NULL;
    new_pcb->next = NULL;
    
    return new_pcb;
}

/*
    ===========================================================
    enqueue_PCB
    ===========================================================
    Add a PCB to the tail of the queue.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    head:       Pointer to the head of the queue.
    pcb:        Pointer to the PCB to add to the queue.
    -----------------------------------------------------------
    return value:
                A pointer to the head of the queue.
    -----------------------------------------------------------
*/
PCB *enqueue_PCB(PCB **head, PCB **pcb) {
    PCB *p = *head;                                     /* for iterating through the PCBs */
    
    /* make sure that pcb is valid */
    if (*pcb != NULL) {
        /* if the queue is empty then the new node becomes the head */
        if (*head == NULL) {
            *head = *pcb;
            (*head)->prev = NULL;
            (*head)->next = NULL;
            return *head;
        }
    
        /* find the tail of the queue */
        while (p->next != NULL) {
            p = p->next;
        }
        
        /* Set next node of current tail of queue to new node */
        p->next = *pcb;
        (*pcb)->prev = p;
        
        /* PCB is the tail of the queue */
        (*pcb)->next = NULL;
    }
    
    /* Return the head of the queue */
    return *head;
}

/*
    ===========================================================
    dequeue_PCB
    ===========================================================
    Remove and return a PCB from the head of a queue. 
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    head:       Pointer to the head queue element to be 
                removed.
    -----------------------------------------------------------
    return value:
                A pointer to the removed element of the queue.
    -----------------------------------------------------------
*/
PCB *dequeue_PCB(PCB **head) {
    PCB *p = *head;                                     /* the element to be removed */
    PCB *prev = p->prev;                                /* the previous PCB */
    PCB *next = p->next;                                /* the next PCB */
    
    /* if PCB is null then there is nothing to do */
    if (*head == NULL) {
        return NULL;
    }
    
    /* increment the head pointer */
    *head = (*head)->next;
    
    /* remove PCB from queue */
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    
    /* p has been removed from the queue */
    p->prev = NULL;
    p->next = NULL;
        
    /* return the removed PCB */
    return p;
}

/*
    ===========================================================
    decrement_remaining_cpu_time
    ===========================================================
    Decrement the remaining CPU time from a process. If the 
    process has no remaining CPU time, then the process will 
    be terminated.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to alter.
    -----------------------------------------------------------
    return value:
                A pointer to the same process, unless this 
                process was terminated in which case NULL is
                returned.
    -----------------------------------------------------------
*/
PCB *decrement_remaining_cpu_time(PCB **pcb) {
    if (*pcb != NULL) {
        /* decrement the remaining CPU time for the process and check whether the process has any remaining CPU time*/
        if ((--((*pcb)->remaining_cpu_time)) <= 0) {
        /* time's up - terminate process */
            *pcb = terminate_PCB(pcb);
            
            /* free memory associated with the PCB */
            free_PCB(pcb);
            
            /* the PCB has been freed, return NULL */
            return NULL;
        }
    }
    
    /* return the PCB */
    return *pcb;
}

/*
    ===========================================================
    lower_priority
    ===========================================================
    Lower the priority of a process. If the priority of the 
    process is already at (or lower than) the value defined as 
    LOWEST_PRIORITY, then the priority is set to 
    LOWEST_PRIORITY.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to alter.
    -----------------------------------------------------------
    return value:
                A pointer to the same process.
    -----------------------------------------------------------
*/
PCB *lower_priority(PCB **pcb) {
    /* lower the priority of the process (unless already at lowest priority) */
    if ((*pcb)->priority < LOWEST_PRIORITY) {
        ((*pcb)->priority)++;
    } else {
        (*pcb)->priority = LOWEST_PRIORITY;
    }
    
    return *pcb;
}

/*
    ===========================================================
    start_PCB
    ===========================================================
    Starts a process by forking the current process.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to start.
    -----------------------------------------------------------
    return value:
                A pointer to the same process, or NULL if 
                starting the process failed.
    -----------------------------------------------------------
*/
PCB *start_PCB(PCB **pcb) {
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Starting PCB %d. Remaining CPU time: %d\n", (*pcb)->id, (*pcb)->remaining_cpu_time);
#endif
    /* fork the current process */
    switch((*pcb)->pid = fork()) {
        case -1: /* fork failed */
            fprintf(__ERROR_OUTPUT, "Forking of PCB %d failed.\n", (*pcb)->id);
            return NULL;
            break;
            
        case 0: /* child */
#ifdef DEBUG
            fprintf(__DEBUG_OUTPUT, "PCB %d forked (PID: %d).\n", (*pcb)->id, (int)getpid());
#endif
            /* execute the command with the appropriate arguments */
            execvp((*pcb)->args[0], (*pcb)->args);
            
            /* if execution reaches this line, an error has occured as execvp should never return */
            fprintf(__ERROR_OUTPUT, "Execution of PCB %d (PID: %d) failed.\n", (*pcb)->id, (int)getpid());
            return NULL;
            break;
                            
        default: /* parent */
            return *pcb;
            break;
    }
}

/*
    ===========================================================
    suspend_PCB
    ===========================================================
    Suspends a process using the SIGTSTP signal.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to suspend.
    -----------------------------------------------------------
    return value:
                A pointer to the same process, or NULL if 
                suspending the process failed.
    -----------------------------------------------------------
*/
PCB *suspend_PCB(PCB **pcb) {
    int status;
    
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Suspending PCB %d (PID: %d). Remaining CPU time: %d\n", (*pcb)->id, (int)(*pcb)->pid, (*pcb)->remaining_cpu_time);
#endif
    /* send the suspend signal */
    if (kill((*pcb)->pid, SIGTSTP)) {
        fprintf(__ERROR_OUTPUT, "Suspension of PCB %d (PID: %d) failed.\n", (*pcb)->id, (int)(*pcb)->pid);
        return NULL;
    }
    
    /* wait for the process to respond to the signal */
    waitpid((*pcb)->pid, &status, WUNTRACED);
    
    return *pcb;
}

/*
    ===========================================================
    restart_PCB
    ===========================================================
    Restarts a process that was suspended with SIGTSTP using 
    the SIGCONT signal.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to restart.
    -----------------------------------------------------------
    return value:
                A pointer to the same process, or NULL if 
                restarting the process failed.
    -----------------------------------------------------------
*/
PCB *restart_PCB(PCB **pcb) {
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Restarting PCB %d (PID: %d). Remaining CPU time: %d\n", (*pcb)->id, (int)(*pcb)->pid, (*pcb)->remaining_cpu_time);
#endif
    /* send the continue signal */
    if (kill((*pcb)->pid, SIGCONT)) {
        fprintf(__ERROR_OUTPUT, "Restarting of PCB %d (PID: %d) failed.\n", (*pcb)->id, (int)(*pcb)->pid);
        return NULL;
    }
    
    return *pcb;
}

/*
    ===========================================================
    terminate_PCB
    ===========================================================
    Terminates a process using the SIGINT signal.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to restart.
    -----------------------------------------------------------
    return value:
                A pointer to the same process, or NULL if 
                terminating the process failed.
    -----------------------------------------------------------
*/
PCB *terminate_PCB(PCB **pcb) {
    int status;
    
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Terminating PCB %d (PID: %d). Remaining CPU time: %d\n", (*pcb)->id, (int)(*pcb)->pid, (*pcb)->remaining_cpu_time);
#endif
    /* Send the kill signal */
    if (kill((*pcb)->pid, SIGINT)) {
        fprintf(__ERROR_OUTPUT, "Termination of PCB %d (PID: %d) failed.\n", (*pcb)->id, (int)(*pcb)->pid);
        return NULL;
    }

    /* wait for the process to respond to the signal */
    waitpid((*pcb)->pid, &status, WUNTRACED);
    
    /* Free the memory associated with the process */
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Freeing the memory associated with PCB %d.\n", (*pcb)->id);
#endif
    (*pcb)->memory = mem_free((*pcb)->memory);
    
    /* Free the resources associated with the process */
#ifdef DEBUG
    fprintf(__DEBUG_OUTPUT, "Freeing the resources associated with PCB %d.\n", (*pcb)->id);
#endif
    resource_free(*pcb);
    
    return *pcb;
}

/*
    ===========================================================
    free_PCB
    ===========================================================
    Frees the memory associated with a PCB.
    
    The parameters for this function are pointers to pointers
    of a struct so that the underlying pointer can be changed
    by this function.
    -----------------------------------------------------------
    pcb:        Pointer to the PCB to free.
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void free_PCB(PCB **pcb) {
    if (*pcb) {
        /* free args array */
        for (unsigned int i = 0; i < MAX_ARGS; i++) {
            if ((*pcb)->args[i]) {
                free((*pcb)->args[i]);
            }
        }
        
        /* remove pointers to this PCB */
        if ((*pcb)->prev != NULL) {
            (*pcb)->prev->next = (*pcb)->next;
        }
        if ((*pcb)->next != NULL) {
            (*pcb)->next->prev = (*pcb)->prev;
        }
        
        /* free PCB structure */
        free(*pcb);
    }
}

#ifdef DEBUG
/*
    ===========================================================
    print_PCB_queue
    ===========================================================
    Prints the queue of PCBs starting at the specified head.
    -----------------------------------------------------------
    head:       The head of the queue to be printed.
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void print_PCB_queue(PCB *head) {
    PCB *p = head;                                      /* to loop through the queue */
    
    if (head != NULL) {
        fprintf(__DEBUG_OUTPUT, "{");
        while (p != NULL) {
            /* print the PCB's ID */
            if ((p->next) != NULL) {
                fprintf(__DEBUG_OUTPUT, "%d, ", p->id);
            } else {
                fprintf(__DEBUG_OUTPUT, "%d", p->id);
            }
            
            /* go to the next PCB */
            p = p->next;
        }
        fprintf(__DEBUG_OUTPUT, "}");
    } else {
        fprintf(__DEBUG_OUTPUT, "(empty)");
    }
}

/*
    ===========================================================
    print_PCB
    ===========================================================
    Prints a detailed description of a single PCB.
    -----------------------------------------------------------
    pcb:        The PCB to be printed.
    -----------------------------------------------------------
    (no return value)
    -----------------------------------------------------------
*/
void print_PCB(PCB *pcb) {
    fprintf(__DEBUG_OUTPUT, "PCB %d: {\n", pcb->id);
    fprintf(__DEBUG_OUTPUT, "\tpid:\t\t\t%d\n", pcb->pid);
    fprintf(__DEBUG_OUTPUT, "\n");
    
    fprintf(__DEBUG_OUTPUT, "\tarrival_time:\t\t%d\n", pcb->arrival_time);
    fprintf(__DEBUG_OUTPUT, "\tremaining_cpu_time:\t%d\n", pcb->remaining_cpu_time);
    fprintf(__DEBUG_OUTPUT, "\tpriority:\t\t%d\n", pcb->priority);
    fprintf(__DEBUG_OUTPUT, "\n");
    
    fprintf(__DEBUG_OUTPUT, "\tmbytes:\t\t\t%d\n", pcb->mbytes);
    if (pcb->memory != NULL) {
        fprintf(__DEBUG_OUTPUT, "\tmemory:\t\t\tMAB%d(%d)\n", pcb->memory->offset, pcb->memory->size);
    } else {
        fprintf(__DEBUG_OUTPUT, "\tmemory:\t\t\t(null)\n");
    }
    fprintf(__DEBUG_OUTPUT, "\n");
    
    fprintf(__DEBUG_OUTPUT, "\tnum_printers:\t\t%d\n", pcb->num_printers);
    fprintf(__DEBUG_OUTPUT, "\tnum_scanners:\t\t%d\n", pcb->num_scanners);
    fprintf(__DEBUG_OUTPUT, "\tnum_modems:\t\t%d\n", pcb->num_modems);
    fprintf(__DEBUG_OUTPUT, "\tnum_cds:\t\t%d\n", pcb->num_cds);
    fprintf(__DEBUG_OUTPUT, "\n");
    
    for (unsigned int i = 0; i < MAX_ARGS; i++) {
        fprintf(__DEBUG_OUTPUT, "\targs[%d]:\t\t%s\n", i, pcb->args[i]);
    }
    fprintf(__DEBUG_OUTPUT, "\n");

    if (pcb->prev) {
        fprintf(__DEBUG_OUTPUT, "\tprev:\t\t\tPCB %d\n", pcb->prev->id);
    } else {
        fprintf(__DEBUG_OUTPUT, "\tprev:\t\t\t(none)\n");
    }
    if (pcb->next) {
        fprintf(__DEBUG_OUTPUT, "\tnext:\t\t\tPCB %d\n", pcb->next->id);
    } else {
        fprintf(__DEBUG_OUTPUT, "\tnext:\t\t\t(none)\n");
    }
    fprintf(__DEBUG_OUTPUT, "}");   
}
#endif