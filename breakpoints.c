#include "breakpoints.h"

#include <stdlib.h>
#include <stdio.h>


/*
 * get_timeval():
 * Gets the value from the Breakpoint file at the given time. If the time
 * is not specified by time-value pair, linearly interpolates between the two
 * closest times.
 *
 * bp:              A pointer to the Breakpoints to look through
 * time:            The time at which to get a corresonding value
 *
 * return:          The value for the given time
 */
float get_timeval(Breakpoints* bp, float time) {
    if(time < 0) {
        printf("Can't read a negative time value from a breakpoint list\n");
        return 0;
    }

    if(bp->len == 0) {
        printf("Reading value from empty breakpoint list\n");
        return 0;
    }

    int i;
    // Loop until next time is past the chosen time
    for(i = 0; i < bp->len-1; i++)
        if(bp->list[i+1].time > time)
            break; 

    // If we're at the last one, then just use that
    if(i == bp->len)
        return bp->list[i].val;
   

    // Linear interpolation between current and next
    float dt = bp->list[i+1].time - bp->list[i].time;
    float da = bp->list[i+1].val - bp->list[i].val;

    float frac = (time - bp->list[i].time)/dt;
    float val = bp->list[i].val + da*frac;

    return val;
}


/*
 * get_percentval():
 * Gets the value from the Breakpoint file at the given percentage through
 * the Breakpoints' length.
 *
 * This is used to stretch the breakpoint envelope out along different lengths.
 *
 * bp:              A pointer to the Breakpoints file to look through
 * percentage:      The percentage of the way through the BP to get a
 *                  value for, between 0 and 1
 *
 * return:          The value at that percentage of the time
 *
 */
float get_percentval(Breakpoints* bp, float percentage) {
    return get_timeval(bp, bp->maxtime * percentage);
}


/*
 * load_bp_file():
 * Loads a breakpoint from the given filename into a malloc'ed Breakpoints
 * struct. The user will need to free the struct with free_breakpoints().
 *
 * The breakpoint file must have one time-value pair per line, and the
 * time and value must be separate by a comma.
 *
 * filename:        The file to load the breakpoints from
 *
 * return:          A malloc'ed Breakpoints struct.
 *
 */
Breakpoints* load_bp_file(char* filename) {
    FILE* file = fopen(filename, "r"); 

    Breakpoints* bp = (Breakpoints*) malloc(sizeof(Breakpoints));

    int len = 5;
    // Allocate initial length
    bp->list = (Breakpoint*) malloc(sizeof(Breakpoint) * len);
    if(bp->list == NULL) {
        printf("Out of memory reading breakpoint file %s\n", filename);
        free(bp->list);
        free(bp);
        fclose(file);
        return NULL;
    }


    int count = 0;
    float time;
    float val;
    char line[255];
    // Get one line at a time
    while(fgets(line, 255, file)) {
        // If we've filled the existing allocation, rellocate with more space
        if(count == len) {
            len += 5;
            bp->list = realloc(bp->list, sizeof(Breakpoint) * len);
            if(bp->list == NULL) {
                printf("Out of memory reading breakpoint file %s\n", filename);
                free(bp->list);
                free(bp);
                fclose(file);
                return NULL;
            }
        }

        // Read in time-value pair
        if(sscanf(line, "%f, %f\n", &time, &val) != 2) {
            printf("Error reading breakpoint file %s: %s\n", filename, line);
            free(bp->list);
            free(bp);
            fclose(file);
            return NULL;
        }

        // Validate time, values
        if(count == 0 && time != 0) {
            printf("Error reading breakpoint file %s: First time value must be 0\n", filename);
            free(bp->list);
            free(bp);
            fclose(file);
            return NULL;
        }
        else if(count > 0 && time < bp->list[count-1].time) {
            printf("Error reading breakpoint file %s: Time values must increase\n", filename);
            free(bp->list);
            free(bp);
            fclose(file);
            return NULL;
        }
        
        bp->list[count].time = time;
        bp->list[count].val = val;

        count++;
    }

    bp->maxtime = time;

    // Resize the allocation to just fit the array
    if(count != len) {
        bp->list = realloc(bp->list, sizeof(Breakpoint) * count+1);
        if(bp->list == NULL) {
            printf("Weird memory error reading breakpoint file %s\n", filename);
            free(bp->list);
            free(bp);
            fclose(file);
            return NULL;
        }
    }

    bp->len = count;

    fclose(file);

    return bp;
}


/*
 * free_breakpoints():
 * Frees the given Breakpoints struct and its associated resources.
 *
 * bp:              A pointer to the Breakpoints struct to free
 *
 */
void free_breakpoints(Breakpoints* bp) {
    free(bp->list);
    free(bp);
}

