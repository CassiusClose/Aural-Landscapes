#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H



/* 
 * Breakpoint:
 * Holds the values of one breakpoint: a time-value pair
 */
typedef struct breakpoint {
    float time;
    float val;
} Breakpoint;

/*
 * Breakpoints:
 * Holds an array of Breakpoint's and associated data.
 */
typedef struct breakpoints {
    Breakpoint* list; 
    int len; // The length of the breakpoint list

    float maxtime; // The maximum time value of the breakpoints
} Breakpoints;


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
Breakpoints* load_bp_file(char* filename);

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
float get_timeval(Breakpoints* bp, float time);


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
float get_percentval(Breakpoints* bp, float percentage);


/*
 * free_breakpoints():
 * Frees the given Breakpoints struct and its associated resources.
 *
 * bp:              A pointer to the Breakpoints struct to free
 *
 */
void free_breakpoints(Breakpoints* bp);


#endif
