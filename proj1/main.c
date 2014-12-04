/* CSci4061 F2014 Assignment 1
 * login:
 * date: 2/10/2014
 * name: An-An Yu, Justin Wells, Vera Novitskaia
 * id: yuxx0535, wells369, novit022
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "util.h"

//an array of targets to represent the DAG
target_t* targets [MAX_TARGETS];
int num_of_targets = 0;
bool flag_n,flag_B,flag_m = FALSE;

// find and return a node in the DAG or create if doesn't exist
target_t* findTargetElseCreate(char* name){
	int i;
	for(i = 0; i < num_of_targets; i++){
		if(!strcmp(targets[i]->name,name))
			return targets[i];
	}
	target_t* newNode = (target_t *) malloc(sizeof(target_t));
	newNode->num_of_dep = 0;
	newNode->isTarget = FALSE;
	newNode->file_state = INELIGIBLE;
	strcpy(newNode->name,name);
	targets[num_of_targets++] = newNode;
	return newNode;
}

//This function will parse makefile input from user or default makeFile.
int parse(char * lpszFileName){
	int nLine=0;
	char szLine[MAX_LENGTH],copy[MAX_LENGTH];
	char *lpszLine,*target,*cmd;
	FILE * fp = file_open(lpszFileName);
	target_t* current_target = NULL;
	target_t* dep = NULL;

	if(fp == NULL){
		perror("File error: Cannot open Makefile\n");
		return -1;
	}
	while(file_getline(szLine, fp) != NULL)
	{
		nLine++;
		//Remove newline character at end if there is one
		lpszLine = strtok(szLine,"\n");
		// skip if comment or empty line
		if (!lpszLine || 0 == (strchr(lpszLine,'#')-lpszLine)) continue;
		strcpy(copy,lpszLine);
		char* token = strtok(copy," \t");
		// ignore if line only has blanks or tabs
		if(token == NULL)
			continue;
		// is reading a target line
		if(strchr(lpszLine,':')){
			target = strtok(lpszLine,": ");
			if(strncmp(target, "\t", strlen("\t")) == 0){
				fprintf(stderr,"Syntax Error in line %d, target line starts with a tab\n",nLine);
				exit(1);
			}
			current_target = findTargetElseCreate(target);
			current_target->isTarget = TRUE;
			while((target = strtok(NULL," "))){
				dep = findTargetElseCreate(target);
				current_target->dep[(current_target->num_of_dep)++] = dep;
			}
		// is reading a command line
		}else if(strchr(lpszLine,'\t')){
			if(!current_target){
				fprintf(stderr,"Syntax Error in line %d, the command's target is undefined\n",nLine);
				exit(1);
			}
			cmd = strtok(lpszLine,"\t");
			if(cmd[0] == '#'){
				fprintf(stderr,"Syntax Error in line %d, Invalid comment\n",nLine);
				exit(1);
			}
			strcpy(current_target->cmd,cmd);
			current_target = NULL;
		}else{
			// must be a command line that starts with blanks
				fprintf(stderr,"Syntax Error in line %d, Unexpected leading character\n",nLine);
				exit(1);
		}
	}
	fclose(fp);			// close makefile

	return 0;
}

// Print out the full error message of the make4061 usage
void show_error_message(char * lpszFileName){
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", lpszFileName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a maumfile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	fprintf(stderr, "-B\t\tDon't check files timestamps.\n");
	fprintf(stderr, "-m FILE\t\tRedirect the output to the file specified .\n");
	exit(0);
}

// finds and return a target_t in our DAG by its name. If it does not exist, returns NULL
target_t* findTargetNodeByName(char* name){
	if(!strlen(name))	return targets[0];
	int i;
	for(i = 0; i < num_of_targets; i++){
		if(!strcmp(targets[i]->name,name))
			return targets[i];
	}
	return NULL;
}

/* input a target_t and the function checks all its dependent files
 * to see if the target_t is ready to execute.

 * returns FALSE if: - it's not a target
 *                   - not all of the dependencies have been built
 *       --> Do NOT execute target_t's command
 *
 * returns TRUE if:  - it's a leaf target (has no dependencies, ready to execute)
 *                   - all of the target's dependencies have been built
 *       --> DOES execute target_t's command
 */
bool checkDependencies(target_t* target){
	if(!target)
		return FALSE;
	if(!target->num_of_dep){				// if is leaf (num_of_dep == 0)
		if(target->isTarget){				// ex: clean: , test1: ...
			target->file_state = READY;
			return TRUE;
		}
		else{						 		// ex: main.c
			if(is_file_exist(target->name) != -1){
				target->file_state = FINISHED;
			}else{
				fprintf(stderr, "Dependency error, Cannot find target %s\n",target->name);
				exit(1);
			}
		}
	}
	int i;
	// check each subtree
	for(i = 0; i < target->num_of_dep; i++){
		if(!checkDependencies(target->dep[i]))
			return FALSE;
	}
	target->file_state = READY;
	return TRUE;
}

// Forks - execute target's command using execvp inside a child process
pid_t forkAndExecute(char* cmd, char* delim){
        char **argv = (char **)malloc(sizeof(char *)*MAX_CMD_TOKENS);
        int num_of_tokens,i ;
        num_of_tokens = makeargv(cmd,delim,&argv);
		if (strlen(cmd)!=0)
				printf("  executing:  %s\n",cmd);
		pid_t child;
		child = fork();
		/* Child process */
		if(child == 0){
			execvp(argv[0],&argv[0]);				// will return on success
			fprintf(stderr, "Execvp error, child process could not do %s.\n",cmd);
			exit(1);
		}
		/* Parent process */
		freemakeargv(argv);
		return child;
}

// execute DAG for makefile
int execute_f(target_t *specifiedTarget){
	pid_t child;
	int status,i; 		// Child's exit status

	// Base case : target has no dependencies or ready, then execute
	if(!specifiedTarget->num_of_dep){
		if(specifiedTarget->file_state == FINISHED)
			return 0;
		if(strlen(specifiedTarget->cmd) == 0) 		// there is no command to execute
			return 0;
		if(flag_n){
			printf("%s\n",specifiedTarget->cmd);
			specifiedTarget->file_state = FINISHED;
			return 0;
		}
		// execute target's cmd inside a child process
		printf("building target: %s\n",specifiedTarget->name);
		child = forkAndExecute(specifiedTarget->cmd," ");

		/* Parent process */
		waitpid(child, &status, 0);
		specifiedTarget->file_state = FINISHED ;
	}
	else{
		int dontRebuild = 0;						// number of dependents are uptodate, and don't need to rebuild
		int cmp;									// result of comparing timestamps

		// iterate thru the dependencies, see how many files are uptodate
		for(i = 0; i < specifiedTarget->num_of_dep; i++){
			execute_f(specifiedTarget->dep[i]);		// recurse to thoroughly check each subtree
			cmp = compare_modification_time(specifiedTarget->name,specifiedTarget->dep[i]->name);
			if(cmp == 0 || cmp == 1)
				dontRebuild++;
		}
		if(dontRebuild == specifiedTarget->num_of_dep && !flag_B){	// if all of target's dependents are uptodate and -B isn't on
			printf("  Skipped building target %s\n",specifiedTarget->name);
			return 0;
		}
		// then build itself
		if(specifiedTarget->file_state == FINISHED)
			return 0;
		if(strlen(specifiedTarget->cmd) == 0) 		// there is no command to execute
			return 0;

		if(flag_n){									// just print cmd, don't execute
			printf("%s\n",specifiedTarget->cmd);
			specifiedTarget->file_state = FINISHED;
			return 0;
		}
		printf("building target: %s\n",specifiedTarget->name);
		child = forkAndExecute(specifiedTarget->cmd," ");		// execute target
		/* Parent process */
		waitpid(child, &status, 0);
		specifiedTarget->file_state = FINISHED;
	}

	return 0;
}

// Main driver, parses and executes Makefiles, supporting flags -fnBm
int main(int argc, char **argv){
	// Declarations for getopt
	extern int optind;
	extern char * optarg;
	int ch;
	char * format = "f:hnBm:";
	bool valid = FALSE;
	// Default makefile name will be Makefile
	char szMakefile[MAX_FILE_NAME] = "Makefile";
	char szTarget[MAX_FILE_NAME];
	char tmp[MAX_FILE_NAME];
	char szLog[MAX_FILE_NAME];
	szTarget[0] = '\0';
	while((ch = getopt(argc, argv, format)) != -1){
		switch(ch){
			case 'f':
				strcpy(szMakefile, strdup(optarg));	// get filename
				break;
			case 'n':				// dry run
				flag_n = TRUE;
				break;
			case 'B':				// recompile
				flag_B = TRUE;
				break;
			case 'm':				// -m log.txt
				flag_m = TRUE;
				strcpy(szLog, strdup(optarg));		// get logfile name
				break;
			default:
				show_error_message(argv[0]);
				exit(1);
		}
		// if an extra string is followed after options -> specified target name
		if (optind < argc && argv[optind][0] != '-' && !strlen(szTarget)){	// Target hasn't been defined
			strcpy(szTarget,argv[optind++]);
		}else if(optind < argc && argv[optind][0] != '-' && strlen(szTarget)>0){ //Target is already defined
			fprintf(stderr, "More than one target were specified\n");
			show_error_message(argv[0]);
		}
	}
	argv += optind;					//argv : targets specified on the command line
	argc -= optind;					//argc : number of them.

	if(argc == 1){						// case: ./make4061 specificTarget
		if(strlen(szTarget) == 0)			// if target hasn't been defined
			strcpy(szTarget,argv[0]);
		else{						// if target is already defined
			fprintf(stderr, "More than one target were specified\n");
			show_error_message(argv[0]);
		}
	}else if(argc > 1){
		show_error_message(argv[0]);
		return EXIT_FAILURE;
	}

	if(flag_m){
		if(is_file_exist(szLog)!= -1){			// remove existing file with the same name,
			remove(szLog);				// this is following Linux make 's behavior
		}
		FILE* newOut = fopen(szLog, "a+");
   		if (!newOut){
   			perror("Cannot open log file\n");
   			return EXIT_FAILURE;
   		}
   		else{
   			dup2(fileno(newOut), 1);
 	  		fclose(newOut);
   		}
	}
	if(is_file_exist(szMakefile) == -1){
		fprintf(stderr, "File error: Makefile '%s' does't exist in this directory\n",szMakefile);
	}
	/* Parse graph file or die */
	if((parse(szMakefile)) == -1) {
		printf("parsing %s failed\n",szMakefile);
		return EXIT_FAILURE;
	}
	// check if specified target is in the graph
	target_t *specifiedTarget = findTargetNodeByName(szTarget);
	if (!specifiedTarget) {
		fprintf(stderr, "Specified Target does not exist\n");
		return EXIT_FAILURE;
	}
	//check all dependencies of target, only execute target when dependencies are statisfied.
	if(checkDependencies(specifiedTarget)){
		execute_f(specifiedTarget);
	}
	else{
		fprintf(stderr, "Dependency error in building target: %s\n",specifiedTarget->name);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
