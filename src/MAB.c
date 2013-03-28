/*
 * MAB.c
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * This file contains the functions relating to memory allocation blocks (MABs).
 */
#include "../inc/MAB.h"
#include "../inc/boolean.h"
#include "../inc/output.h"
#include <stdlib.h>

extern MAB * memory; // global variable to pointer to the head of the memory list
static unsigned int _id = 1; // counter used to track assigned IDs

/*
 * Creates a new MAB, with all values initialised to logical default values.
 *
 * RETURN VALUE
 * A pointer to the new MAB.
 */
MAB * create_null_MAB(void) {
    MAB * new_mab = (MAB *) malloc(sizeof(MAB));

    new_mab->id = (_id++);

    new_mab->offset = 0;
    new_mab->size = 0;
    new_mab->allocated = false;

    new_mab->prev = NULL;
    new_mab->next = NULL;

    return new_mab;
}

/*
 * Checks if there is a MAB of at least the right size available. If there is
 * such a MAB, it is returned.
 *
 * Uses the global variable 'memory' as the head of the MAB list.
 *
 * PARAMETERS
 *     size:            The size of the MAB being requested.
 *
 * RETURN VALUE
 * A pointer to the requested MAB. NULL if no such MAB could be found.
 */
MAB * mem_check(unsigned int size) {
    MAB * m = memory; // for iterating through the MAB list

    // Look for available memory
    while (m != NULL) {
        // Check if this MAB is an appropriate choice
        if (!m->allocated && m->size >= size) {
            return m;
        }

        // Go to the next MAB in the list
        m = m->next;
    }

    // Available memory not found
    return NULL;
}

/*
 * Allocates a MAB of the specified size and returns a pointer to this MAB. The
 * allocation algorithm employed is a first-fit policy.
 *
 * Uses the global variable 'memory' as the head of the MAB list.
 *
 * PARAMETERS
 *     size:            The size of the MAB being requested.
 *
 * RETURN VALUE
 * A pointer to the requested MAB. NULL if no such MAB could be found.
 */
MAB * mem_alloc(unsigned int size) {
    MAB * m = NULL; // the requested MAB - null if none found

    if (size > 0) {
        // Try to allocate memory
        if ((m = mem_check(size)) && mem_split(m, size)) {
            m->allocated = true;
        }
    }

    // Return the (un)allocated memory
    return m;
}

/*
 * Frees a MAB.
 *
 * PARAMETERS
 *     mab:             Pointer to the MAB to be freed.
 *
 * RETURN VALUE
 * NULL if the MAB was freed successfully.
 */
MAB * mem_free(MAB * mab) {
    if (mab != NULL) {
        MAB * prev = mab->prev; // remember the previous MAB before we free mab

        // Mark the MAB as not being allocated
        mab->allocated = false;

        // Try to merge the MAB that was freed with the next MAB in the list
        mab = mem_merge(mab);

        // Try to merge the MAB that was freed with the previous MAB in the list
        prev = mem_merge(prev);
    }

    // Success
    return NULL;
}

/*
 * Merge the specified MAB with the next MAB in the list, if appropriate.
 *
 * PARAMETERS
 *     mab:             Pointer to the MAB to be merged.
 *
 * RETURN VALUE
 * A pointer to the merge MAB. If no merge occurred, returns a pointer to the
 * same element.
 */
MAB * mem_merge(MAB * mab) {
    if (mab != NULL) {
        MAB * m = mab->next; // remember the next element

        if (m != NULL) {
            // Make sure a merge is a valid operation
            if ((!(mab->allocated)) && (!(m->allocated))) {
                // Combine the sizes of the two MABs
                mab->size += m->size;

                // Fix the list next/prev pointers
                if (m->next != NULL) {
                    m->next->prev = mab;
                    mab->next = m->next;
                } else {
                    mab->next = NULL;
                }

                // Free the redundant MAB
                free(m);
            }
        }
    }

    return mab;
}

/*
 * Split the specified MAB into two MABs - one of size (size) and the other of
 * size (mab->size - size).
 *
 * PARAMETERS
 *     mab:             Pointer to the MAB to split.
 *     size:            The requested size.
 *
 * RETURN VALUE
 * Returns a value to the first of the split MAB elements (the MAB with size
 * (size)). Returns NULL if the split cannot be performed.
 */
MAB * mem_split(MAB * mab, unsigned int size) {
    // Check that splitting the MAB would be a valid operation
    if ((!(mab->allocated)) && (mab->size >= size)) {
        if (mab->size > size) {
            // Perform the split
            MAB *new_mab = create_null_MAB(); // the new MAB that is created
            new_mab->offset = mab->offset + size;
            new_mab->size = mab->size - size;
            mab->size = size;

            // Set the list next/prev pointers
            new_mab->prev = mab;
            new_mab->next = mab->next;
            if (mab->next != NULL) {
                mab->next->prev = new_mab;
            }
            mab->next = new_mab;
        }
        // else mab is already the right size

        return mab;
    }

    return NULL;
}

/*
 * Prints the list of MABs with their attributes.
 *
 * PARAMETERS
 *     head:            The head of the list.
 */
void print_MAB_list(MAB * head) {
    MAB * m = head; // for iterating through the list

    // Output header
    fprintf(__DEBUG_OUTPUT, "==================================\n");
    fprintf(__DEBUG_OUTPUT, "ID\tOFFSET\tSIZE\tALLOCATED\n");
    fprintf(__DEBUG_OUTPUT, "----------------------------------\n");

    // Output MAB list
    if (m != NULL) {
        while (m != NULL) {
            if (m->allocated) {
                fprintf(__DEBUG_OUTPUT, "%d\t%d\t%d\tTRUE\n", m->id, m->offset, m->size);
            } else {
                fprintf(__DEBUG_OUTPUT, "%d\t%d\t%d\tFALSE\n", m->id, m->offset, m->size);
            }

            // Go to the next MAB in the list
            m = m->next;
        }
    } else {
        fprintf(__DEBUG_OUTPUT, "(none)\n");
    }

    // End
    fprintf(__DEBUG_OUTPUT, "==================================\n");
}

#ifdef DEBUG
/*
 * Prints a detailed description of a MAB.
 *
 * PARAMETERS
 *     mab:             The mab to print.
 */
void print_MAB(MAB * mab) {
    fprintf(__DEBUG_OUTPUT, "MAB %d: {\n", mab->id);
    fprintf(__DEBUG_OUTPUT, "\toffset:\t\t%d\n", mab->offset);
    fprintf(__DEBUG_OUTPUT, "\tsize:\t\t%d\n", mab->size);
    if (mab->allocated) {
        fprintf(__DEBUG_OUTPUT, "\tallocated:\t\ttrue\n");
    } else {
        fprintf(__DEBUG_OUTPUT, "\tallocated:\t\tfalse\n");
    }
    fprintf(__DEBUG_OUTPUT, "\n");

    if (mab->prev) {
        fprintf(__DEBUG_OUTPUT, "\tprev:\t\tMAB %d\n", mab->prev->id);
    } else {
        fprintf(__DEBUG_OUTPUT, "\tprev:\t\t(none)\n");
    }
    if (mab->next) {
        fprintf(__DEBUG_OUTPUT, "\tnext:\t\tMAB %d\n", mab->next->id);
    } else {
        fprintf(__DEBUG_OUTPUT, "\tnext:\t\t(none)\n");
    }

    fprintf(__DEBUG_OUTPUT, "}\n");
}
#endif // #ifdef DEBUG
