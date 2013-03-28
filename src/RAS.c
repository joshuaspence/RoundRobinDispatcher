/*
 * RAS.h
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * This file contains the functions relating to resource allocation structures
 * (RASs).
 */
#include "../inc/RAS.h"
#include "../inc/PCB.h"
#include "../inc/output.h"
#include <stdlib.h>

extern RAS * resources; // global variable for head of resource list
static unsigned int _id = 1; // counter used to track assigned IDs

/*
 * Creates an RAS list, containing the specified number of resources.
 *
 * PARAMETERS
 *     num_printers: The number of printer resources available.
 *     num_scanners: The number of scanner resources available.
 *     num_modems: The number of modem resources available.
 *     num_cds: The number of CD resources available.
 *
 * RETURN VALUE
 * A pointer to the head of the new RAS list.
 */
RAS * create_resources(unsigned int num_printers, unsigned int num_scanners, unsigned int num_modems, unsigned int num_cds) {
    RAS * head = NULL; // head of newly-created RAS list
    RAS * prev = NULL; // used to keep track of most recent RAS

    // Create printer resources
    for (unsigned int i = 0; i < num_printers; i++) {
        RAS * new_ras = create_null_RAS();
        new_ras->resource = Printer_Resource;

        // Set next, prev and head pointers
        new_ras->prev = prev;
        if (prev != NULL) {
            prev->next = new_ras;
        }
        if (head == NULL) {
            head = new_ras;
        }
        prev = new_ras;
    }

    // Create scanner resources
    for (unsigned int i = 0; i < num_scanners; i++) {
        RAS * new_ras = create_null_RAS();
        new_ras->resource = Scanner_Resource;

        // set next, prev and head pointers
        new_ras->prev = prev;
        if (prev != NULL) {
            prev->next = new_ras;
        }
        if (head == NULL) {
            head = new_ras;
        }
        prev = new_ras;
    }

    // Create modem resources
    for (unsigned int i = 0; i < num_modems; i++) {
        RAS * new_ras = create_null_RAS();
        new_ras->resource = Modem_Resource;

        // Set next, prev and head pointers
        new_ras->prev = prev;
        if (prev != NULL) {
            prev->next = new_ras;
        }
        if (head == NULL) {
            head = new_ras;
        }
        prev = new_ras;
    }

    // Create CD resources
    for (unsigned int i = 0; i < num_cds; i++) {
        RAS *new_ras = create_null_RAS();
        new_ras->resource = CD_Resource;

        // Set next, prev and head pointers
        new_ras->prev = prev;
        if (prev != NULL) {
            prev->next = new_ras;
        }
        if (head == NULL) {
            head = new_ras;
        }
        prev = new_ras;
    }

    // Return the head of the RAS list
    return head;
}

/*
 * Creates a new RAS, with all values initialised to logical default values.
 *
 * RETURN VALUE
 * A pointer to the new RAS.
 */
RAS * create_null_RAS(void) {
    RAS * new_ras = (RAS *) malloc(sizeof(RAS));

    new_ras->id = (_id++);
    new_ras->resource = Null_Resource;

    new_ras->allocated = NULL;
    new_ras->prev = NULL;
    new_ras->next = NULL;

    return new_ras;
}

/*
* Checks if there is a resource of the specified type available. Note that this
* does not allocate the resource.
*
* Uses the global variable 'resources' as the head of the RAS list.
*
* PARAMETERS
*     type: The type of resource requested.
*
* RETURN VALUE
* A pointer to the requested RAS. NULL if no such RAS could be found.
*/
RAS * resource_check(ResourceType type) {
    RAS * r = resources; // for iterating through the RAS list

    // Look for the requested resource
    while (r != NULL) {
        // Check if this RAS is an appropriate choice
        if ((r->allocated == NULL) && (r->resource == type)) {
            return r;
        }

        // Go to the next RAS in the list
        r = r->next;
    }

    // No resource found
    return NULL;
}

/*
 * Allocates a RAS of the specified type and returns a pointer to the allocated
 * RAS.
 *
 * Uses the global variable 'resources' as the head of the RAS list.
 *
 * PARAMETERS
 *     type: The type of resource requested.
 *     pcb: The process requesting the resource.
 *
 * RETURN VALUE
 * A pointer to the requested RAS. NULL if no such RAS could be found.
 */
RAS * resource_alloc(ResourceType type, PCB * pcb) {
    RAS * r = NULL; // the requested RAS - null if none found

    if ((r = resource_check(type))) {
        r->allocated = pcb;
    }

    // Return the resource
    return r;
}

/*
 * Frees all resources allocated to a process.
 *
 * Uses the global variable 'resources' as the head of the RAS list.
 *
 * PARAMETERS
 *     pcb: The process being freed.
 *
 * RETURN VALUE
 * NULL if the RAS list was freed successfully.
 */
RAS * resource_free(PCB * pcb) {
    RAS * r = resources; // to iterate through the RAS list

    // Scan the RAS list for resources allocated to process pcb
    while (r != NULL) {
        if (r->allocated == pcb) {
            r->allocated = NULL;
        }

        // Go to the next resource
        r = r->next;
    }

    // Sucess
    return NULL;
}

/*
 * Prints the list of RASs with their attributes.
 *
 * PARAMETERS
 *     head: Pointer to the head of the RAS list.
 */
void print_RAS_list(RAS * head) {
    RAS * r = head; // for iterating through the list

    // Output header
    fprintf(__DEBUG_OUTPUT, "==================================\n");
    fprintf(__DEBUG_OUTPUT, "ID\tRESOURCE\tALLOCATED\n");
    fprintf(__DEBUG_OUTPUT, "----------------------------------\n");

    // Output list
    if (r != NULL) {
        while (r != NULL) {
            if (r->resource == Printer_Resource) {
                if (r->allocated != NULL) {
                    fprintf(__DEBUG_OUTPUT, "%d\tPrinter\t\tPCB%d\n", r->id, r->allocated->id);
                } else {
                    fprintf(__DEBUG_OUTPUT, "%d\tPrinter\t\t(null)\n", r->id);
                }
            } else if (r->resource == Scanner_Resource) {
                if (r->allocated != NULL) {
                    fprintf(__DEBUG_OUTPUT, "%d\tScanner\t\tPCB%d\n", r->id, r->allocated->id);
                } else {
                    fprintf(__DEBUG_OUTPUT, "%d\tScanner\t\t(null)\n", r->id);
                }
            } else if (r->resource == Modem_Resource) {
                if (r->allocated != NULL) {
                    fprintf(__DEBUG_OUTPUT, "%d\tModem\t\tPCB%d\n", r->id, r->allocated->id);
                } else {
                    fprintf(__DEBUG_OUTPUT, "%d\tModem\t\t(null)\n", r->id);
                }
            } else if (r->resource == CD_Resource) {
                if (r->allocated != NULL) {
                    fprintf(__DEBUG_OUTPUT, "%d\tCD\t\tPCB%d\n", r->id, r->allocated->id);
                } else {
                    fprintf(__DEBUG_OUTPUT, "%d\tCD\t\t(null)\n", r->id);
                }
            }

            // Go to the next RAS in the list
            r = r->next;
        }
    } else {
        fprintf(__DEBUG_OUTPUT, "(none)\n");
    }

    // End
    fprintf(__DEBUG_OUTPUT, "==================================\n");
}
