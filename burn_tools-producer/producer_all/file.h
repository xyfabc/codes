#ifndef __BURN_FILE__
#define __BURN_FILE__

/* start to burn the file */
extern int start_burn_file(char name[], int isdir);

/* burn the file */
extern int burn_file(char buf[], int len);

/* stop burning the file */
extern int close_file();
#endif
