/*
    MAB.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the functions for parsing an input CSV file into a queue of PCBs.
*/
#include "../inc/input.h"
#include "../inc/hostd.h"
#include "../inc/PCB.h"
#include "../inc/output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    ===========================================================
    read_process_list
    ===========================================================
    Parse an input CSV file, creating a queue of PCBs.
    -----------------------------------------------------------
    file:       The input file to parse.
    -----------------------------------------------------------
    Return value:
                A pointer to the head of the input queue.
    -----------------------------------------------------------
*/
PCB *read_process_list(FILE *file) {
    PCB *head = NULL;                                   /* the head of the input queue */
    PCB *tail = NULL;                                   /* the tail of the input queue */
    PCB *new_pcb = NULL;                                /* the new PCB to add to the tail of the inpit queue */
    int status;                                         /* return value from fscanf */
    
    char buffer[INPUT_BUFFER];                          /* buffer for reading a line of input */
    
    unsigned int arrival_time;                          /* arrival time of PCB */
    unsigned int priority;                              /* priority of PCB */
    unsigned int remaining_cpu_time;                    /* remaining CPU time of PCB */
    unsigned int mbytes;                                /* memory required by process */
    unsigned int num_printers;                          /* the number of printer resources required by the process */
    unsigned int num_scanners;                          /* the number of scanner resources required by the process */
    unsigned int num_modems;                            /* the number of modem resources required by the process */
    unsigned int num_cds;                               /* the number of CD resources required by the process */
    
    /* Read input file until at end of file */
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        status = sscanf(buffer, "%u, %u, %u, %u, %u, %u, %u, %u", &arrival_time, &priority, &remaining_cpu_time, &mbytes, &num_printers, &num_scanners, &num_modems, &num_cds);

        if (status == 8) {
            /* Create new PCB node */
            new_pcb = create_null_PCB();
            
            /* Parse input file */
#ifdef DEBUG
            fprintf(__DEBUG_OUTPUT, "Parsing input file for PCB %d.\n", new_pcb->id);
#endif
        
            /* store PCB parameters */
            new_pcb->arrival_time = arrival_time;
            if (priority > LOWEST_PRIORITY) {
                fprintf(__ERROR_OUTPUT, "PCB %d has an invalid priority value (%d). Setting priority to lowest priority (%d).\n", new_pcb->id, priority, LOWEST_PRIORITY);
                new_pcb->priority = LOWEST_PRIORITY;
            } else {
                new_pcb->priority = priority;
            }
            new_pcb->remaining_cpu_time = remaining_cpu_time;
            new_pcb->mbytes = mbytes;
            new_pcb->num_printers = num_printers;
            new_pcb->num_scanners = num_scanners;
            new_pcb->num_modems = num_modems;
            new_pcb->num_cds = num_cds;
            
            /* store the program to run in args[0] */
            new_pcb->args[0] = (char *)malloc(STRING_BUFFER);
            strcpy(new_pcb->args[0], PROCESS);          
            
            /* store the program arguments in args[1] */
            new_pcb->args[1] = (char *)malloc(STRING_BUFFER);
            sprintf(new_pcb->args[1], "%d", new_pcb->remaining_cpu_time); 
            
            /* args array is null terminated */
            new_pcb->args[2] = '\0';
            
            /* if real time process then check resources and memory */
            if (new_pcb->priority == REAL_TIME_PROCESS_PRIORITY) {
                if (new_pcb->mbytes > REAL_TIME_PROCESS_MAX_MBYTES) {
                    new_pcb->mbytes = REAL_TIME_PROCESS_MAX_MBYTES;
                }
                new_pcb->num_printers = 0;
                new_pcb->num_scanners = 0;
                new_pcb->num_modems = 0;
                new_pcb->num_cds = 0;
            }
            
            /* Link previous node to new node */
#ifdef DEBUG
            print_PCB(new_pcb);
            fprintf(__DEBUG_OUTPUT, "\nAdding PCB %d to input queue.\n", new_pcb->id);
#endif

            if (head == NULL) {
                head = new_pcb;
            } else if (tail != NULL) {
                tail->next = new_pcb;
                new_pcb->prev = tail;
            }
                
            /* Process next node */
            tail = new_pcb;
        } else {
            fprintf(__ERROR_OUTPUT, "Invalid data in input file. Skipping line: '%s'.\n", buffer);
        }
    }
    
    return head;
}