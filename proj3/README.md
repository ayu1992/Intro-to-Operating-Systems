CSci4061 F2014 Assignment 3
Section: 7
Date: 11/11/14
Names: An An Yu, Justin Wells
ID: (yuxx0535, 5116372), (wells369, 4227626)

Teamwork: An-An did much of the coding, but Justin was present and giving input
          as she coded. We also worked together to debug and document.

Part A:
	PURPOSE:
		main_mm.c is a program that functions as a dynamic memory allocator,
		much like malloc.

	COMPILIING THE PROGRAM:
		rm -f mm.o main_mm main_malloc
		gcc -o mm.o -c mm.c          (Ignore warnings)
		gcc -o main_mm main_mm.c mm.o
		gcc -o main_malloc main_malloc.c mm.o

	USING THE PROGRAM:
		./main_mm
		./main_malloc

	WHAT THE PROGRAM DOES:
		Dynamically allocates memory and clocks how fast the process runs,
		printing the resulting time necessary to complete the dynamic
		memory allocation.
		
		So executing ./main_mm will give our time, while ./main_malloc
		gives the time it takes to actually run malloc instead of our memory
		allocator. Compare the resulting printed times.
		
		**Depending on the system, Valgrind on Mac OS X showed us that our
		mm.c and mm.h files runs approximately 1.5 times faster than malloc. But on other
		systems, malloc would run the same as or faster than our version.
		We have no system-dependant code, so we are unsure why this is the case.
		

Part B:
	PURPOSE:
		To implement a simple interrupt-driven message-handling capability
		using signals as well as our above-mentioned dynamic memory allocator.

	COMPILIING THE PROGRAM:
		rm -f mm.o packet_sender packet_receiver
		gcc -o mm.o -c mm.c          (Ignore warnings)
		gcc -o packet_sender packet_sender.c mm.o
		gcc -o packet_receiver packet_receiver.c mm.o

	USING THE PROGRAM:
		In one terminal ------> ./packet_sender {specify_any_positive_integer}
		In a second terminal -> ./packet_receiver {same_number_as_above}

	WHAT THE PROGRAM DOES:
		There are two separate executable files running in two separate terminals.
		The sender sends packets of data for the receiver to pick up and print
		out in STDOUT.

	ASSUMPTIONS:
		We assume the user DOES NOT terminate the process early (before the
		number of messages equals the positive integer specified as the parameter).

	ERROR HANDLING STRATEGIES:
		1) If the user DOES terminate the process execution before successful
		termination, then the following executions may produce an error. If this
		or any other errors occur, the user will need to manually delete all
		messages in the message queue. To do so, enter the following in the
		terminal window:
		
			--> ipcs -q
			--> ipcrm -q <msquid>   (where msqid is the message ID printed after
								  after the first command)
			

	CASES THAT WOULD RESULT IN PROGRAM EXIT:
		1) If the user DOES terminate the process execution before successful
		completion, there may be a Segmentation Fault (core dump). This is
		because there is still a message in the queue from the last execution.

		2) If the numbers do not match for the parameters for the sender and
		the receiver, the program will not function correctly and may exit
		abruptly. Make sure the numbers align.
		
		3) Errors in system calls or memory manager occurred.
