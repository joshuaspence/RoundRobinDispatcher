/*
    MAB.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the functions for parsing an input CSV file into a queue of PCBs.
*/
#ifndef INPUT_H_
#define INPUT_H_

#define PROCESS                         "./sigtrap"     /* the process to be executed for each PCB */
#define STRING_BUFFER                   256             /* size of buffer for string arguments for processes */
#define INPUT_BUFFER                    1024            /* buffer for storing a line of input */

#include "PCB.h"
#include <stdio.h>

PCB *read_process_list(FILE *file);

#endif
