/*
    MAB.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the functions relating to memory allocation blocks (MABs).
*/
#ifndef MAB_H_
#define MAB_H_

#include "boolean.h"

typedef struct MAB {
    unsigned int id;                                    /* unique identifier */

    unsigned int offset;                                /* the start address (in megabytes) of this MAB */
    unsigned int size;                                  /* the size (in megabytes) of this MAB */
    boolean allocated;                                  /* has this MAB been allocated to a process? - note that the MAB does not know which process it has been allocated to */

    struct MAB *next;                                   /* next MAB in the list */
    struct MAB *prev;                                   /* previous MAB in the list */
} MAB;

MAB *create_null_MAB(void);

MAB *mem_check(unsigned int size);
MAB *mem_alloc(unsigned int size);
MAB *mem_free(MAB *mab);
MAB *mem_merge(MAB *mab);
MAB *mem_split(MAB *mab, unsigned int size);

void print_MAB_list(MAB *head);

#ifdef DEBUG
void print_MAB(MAB *mab);
#endif

#endif