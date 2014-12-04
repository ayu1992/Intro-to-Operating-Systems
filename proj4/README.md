/* csci4061 F2013 Assignment 4 
* section: 7 
* date: 12/03/14 
* names: An An Yu, Justin Wells
* UMN Internet ID, Student ID (yuxx0535, 5116372), (wells369, 4227626)
*/

Multi-threaded Web Server:

The purpose of this program is to function as a server, handling http
requests. The threads function in a dispatcher/worker model. Upon execution,
the user specifies the port number, the path to the http requests, the
number of dispatcher threads, the number of worker threaeds, and the
length of the queue. (Does not run using a cache system)

After execution, the log of the server can be found in the file
webserver_log.

**NOTE: Port has to be greater than 1024

Use: (Server)
--> cd /home/<path_to_project>/testing
--> rm -f webserver_log
--> touch webserver_log
--> make
--> ./web_server_http <port> /home/<path_to_project>/testing <num_dispatcher> <num_workers> <queue_length>

	ex: $ ./web_server_http 9000 /home/wells369/OS/testing 100 100 100
	
Second Terminal: (Client)
--> cd /home/<path_to_project>/testing
--> wget -i /home/<path_to_project>/testing/urls -O myres

	ex: $ wget -i /home/wells369/OS/testing/urls -O myres

Client should end successfully, while the server will run indefinitely
until it is forced to stop by Ctrl C (^C).

