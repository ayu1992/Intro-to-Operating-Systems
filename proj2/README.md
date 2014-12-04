/* CSci4061 F2014 Assignment 2
* date: 10/27/2014
* name: An An Yu, Justin Wells
* id: yuxx0535, wells369 */

1. The purpose of your program

	The purpose of the program is to create an internet browser that uses
	separate processes for each tab, using a router to communicate between
	the processes, and a controller to communicate the user inputted fields
	to the corresponding tab processes.

2. How to compile the program
   
   if (using Ubuntu 14) {
		>> setenv PATH /soft/gcc-4.7.2/ubuntuamd2010/bin/:$PATH
   }
   >> make

3. How to use the program from the shell (syntax)
   
   >> ./browser
   
4. What exactly your program does

	Our program uses multiple processes and inter-process communication
	to function as a web browser. The original parent process functions as
	the router, communicating between all of the child processes and passing
	the packets of data and the signals between the processes like an actual
	router does. The first child processes functions as the controller,
	which handles user-inputted data, such as clicking on a new tab, or
	specifying a url to render for a given tab. For every time a new tab
	is created through the controller, a new process is created. This process
	renders the specified url from the controller, and terminates when the
	window is closed. When the controller and all of the tabs are closed,
	the router successfully terminates.

5. Any explicit assumptions you have made
   
   a. User opens tab before specifying url
		- error handled without terminating, appropriate error message passed

6. Your strategies for error handling

   - When a url-rendering tab encounters an error, an error message is printed
   and the browser tries the operation again later. 
   - For the router, if one of its children had crashed or encountered an error, 
   the router displays error message and continues to work with other children.
   - While attempting to kill a process, if the process terminates but the pipe
   fails to close due to file descriptor or memory errors, then the program can
   still create new tabs and pipes correctly. However, the pipe that previously
   failed to close will not be reopened. Instead, the pipe of the next sequential
   number (other than said tab) will be opened. We assume the erronious pipe will
   be closed by the OS when the program terminates.   

   Cases that would result in program exit: 
     a. failed to create pipes for controller -> exit failure
     b. controller is closed -> exit success
     
