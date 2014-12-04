/********************
 * util.h
 *
 * You may put your utility function definitions here
 * also your structs, if you create any
 *********************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// the following ifdef/def pair prevents us from having problems if 
// we've included util.h in multiple places... it's a handy trick
#ifndef _UTIL_H_
#define _UTIL_H_

typedef int bool;
#define TRUE 1
#define FALSE 0

//You can change any value in this file as you want. 
#define INELIGIBLE 0 			// file not ready to compile
#define READY 1 	 			// ready to compile
#define RUNNING 2    			// executing cmd (compiling)
#define FINISHED 3 				// file built

#define MAX_LENGTH 1024			// maximum length of a line in makefile
#define MAX_DEPENDENCIES 10 	// maximum number of dependent files a target could have
#define MAX_TARGETS 30			// maximum number of targets in the DAG

// self defined 
#define MAX_TARGET_NAME_LENGTH 100	// maximum length of specified target's name
#define MAX_CMD_LENGTH 40		// maximum command length
#define MAX_CMD_TOKENS 30		// maximum number of tokens in a command, separated by blanks
#define MAX_FILE_NAME 64		// maximum length of makefile name 

// This stuff is for easy file reading
FILE * file_open(char*);
char * file_getline(char*, FILE*);
int is_file_exist(char *);
int get_file_modification_time(char *);
int compare_modification_time(char *, char *);
int makeargv(const char *s, const char *delimiters, char ***argvp);
void freemakeargv(char **argv);

//You will need to fill this struct out to make a graph.
typedef struct target{			
	char name[MAX_TARGET_NAME_LENGTH];		
	struct target* dep[MAX_DEPENDENCIES];		// a target_t array of dependent files
	int num_of_dep;					// number of dependent files
	int file_state;					// INELIGIBLE, READY, RUNNING, FINISHED			
	char cmd[MAX_CMD_LENGTH];			// the command to execute target
	bool isTarget;					// TRUE: file state can be updated by executions (eg: main.o,clean)
							// FALSE: file state is always finished(eg: foo.c, foo.h)
}target_t;

#endif
