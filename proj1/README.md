/*CSci4061 F2014 Assignment 1
* login: cse_labs ????
* date: 09/25/2014
* name: An-An Yu, Justin Wells, Vera Novitskaia
* id: yuxx0535, wells369, novit022
*/

- The purpose of the program:

makefile4061 determines which components of a program need to be built, so that the target can be executed with satisfied dependencies. 
The program mimics regular Linux make and, using the DAG structure, also checks timestamps before building to avoid uneccessary re-compiling.

- How to compile the program:

To compile our program, simply use make. Using the built-in make, our make4061 is compiled. It can then be used in a similar fashion to the regular make. 

- How to use the program from the shell(syntax):

Using the following command,

./make4061 [options] [target_name] [none or options]

where the options could be:
  -f FILENAME           : Runs on a specific Makefile.
  -n                    : Dry runs (aka print commands instead of executing).
  -m LOGFILENAME        : Outputs into a logfile, replacing a previous logfile if one existed.
  -B                    : Recompiles everything.

the following format doesn't meet the specification and thus would fail:
./make4061 [target] [options]

- What exactly the program does:

1. Reads the user input command line arguments, and sets up execution based on flags and arguments
    ( eg. set up makefile name, target name, values of each flag , open logfile to write in later ...etc )
2. Parses Makefile - If there is no syntax error, it builds the dependency DAG and continues.
3. Checks the specified target's dependencies (if not specified, then the first target)
   verifies that all source files exist (or are another target) in order to build the target and
   it's dependencies.
4. Execute target after recursively building all it's dependencies: 
     Only after all dependencies have been built, can the parent target be executed. 
     All executions are handled by forkAndExecute(), where the execution takes place in a child process.
     By default, we will do timestamp checks for each dependency before forking to avoid uneccessary executions. 
5. Outputs the sequence of execution in STDIO (in the terminal) along with any error messages. If log
   file specified, then prints to log file insead of STDIO.
   
