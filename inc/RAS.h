/*
    RAS.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the functions relating to resource allocation structures (RASs).
*/
#ifndef RAS_H_
#define RAS_H_

#include "PCB.h"

/* an enumerator to identify the type of resource */
typedef enum _ResourceType {
    Null_Resource,
    Printer_Resource,
    Scanner_Resource,
    Modem_Resource,
    CD_Resource
} ResourceType;

typedef struct RAS {
    unsigned int id;                                    /* unique identifier */
    
    ResourceType resource;                              /* type of resource */
    PCB *allocated;                                     /* PCB to which the resource is allocated (null if not allocated) */
    
    struct RAS *next;                                   /* next RAS in the list */
    struct RAS *prev;                                   /* previous RAS in the list */
} RAS;

RAS *create_resources(unsigned int num_printers, unsigned int num_scanners, unsigned int num_modems, unsigned int num_cds);
RAS *create_null_RAS(void);

RAS *resource_check(ResourceType type);
RAS *resource_alloc(ResourceType type, PCB *pcb); 
RAS *resource_free(PCB *pcb);

void print_RAS_list(RAS *head);

#endif