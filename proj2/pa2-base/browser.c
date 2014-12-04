/* CSci4061 F2014 Assignment 2
* date: 10/27/2014
* name: An An Yu, Justin Wells
* id: yuxx0535, wells369 */

#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#define DEBUG
#define MAX_TAB 100
extern int errno;
//create pipe for communication with controller
comm_channel * comm; 

/*
 * Name:		uri_entered_cb
 * Input arguments:'entry'-address bar where the url was entered
 *			 'data'-auxiliary data sent along with the event
 * Output arguments:void
 * Function:	When the user hits the enter after entering the url
 *			in the address bar, 'activate' event is generated
 *			for the Widget Entry, for which 'uri_entered_cb'
 *			callback is called. Controller-tab captures this event
 *			and sends the browsing request to the router(/parent)
 *			process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data)
{
	if(data == NULL){	
		return;
	}
	browser_window* b_window = (browser_window*)data;
	comm_channel channel = b_window->channel;
	
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);
	if(tab_index < 0){
		fprintf(stderr, "uri_entered_cb@43: Invalid tab index\n");
        return;
	}
	// Get the URL.
	char* uri = get_entered_uri(entry);

	// Send request to router
	child_req_to_parent req;
	req.type = NEW_URI_ENTERED;
	strcpy(req.req.uri_req.uri, uri);
	req.req.uri_req.render_in_tab = tab_index;
	
	if (write(channel.child_to_parent_fd[1], &req, sizeof(child_req_to_parent)) == -1)
		perror("uri_entered_cb@55: Controller write request in pipe");
}

/*
 * Name:		new_tab_created_cb
 * Input arguments:	'button' - whose click generated this callback
 *			'data' - auxillary data passed along for handling
 *			this event.
 * Output arguments:    void
 * Function:		This is the callback function for the 'create_new_tab'
 *			event which is generated when the user clicks the '+'
 *			button in the controller-tab. The controller-tab
 *			redirects the request to the parent (/router) process
 *			which then creates a new child process for creating
 *			and managing this new tab.
 */ 
void new_tab_created_cb(GtkButton *button, gpointer data)
{
	if(data == NULL){
		return;
	}
 	int tab_index = ((browser_window*)data)->tab_index;

	//This channel have pipes to communicate with router. 
	comm_channel channel = ((browser_window*)data)->channel;

	// Create a new request of type CREATE_TAB
	child_req_to_parent new_req;

	// Users press + button on the control window. 
	new_req.type = CREATE_TAB;
	new_req.req.new_tab_req.tab_index = tab_index;
	
	//Send the request to parent(Router)
	if (write (channel.child_to_parent_fd[1], &new_req, sizeof(child_req_to_parent)) == -1){
		perror("new_tab_created_cb@91:Controller writes request");
	}
	return;
}

/*
 * Name:                run_control
 * Input arguments:     void
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminate.
 */
int run_control(void)
{
	browser_window * b_window = NULL;
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm[0]);
	show_browser();
	return 0;
}

/*
* Name:                 run_url_browser
* Input arguments:      'nTabIndex': URL-RENDERING tab index
* Output arguments:     void
* Function:             This function will make a URL-RENDRERING tab Note.
*                       You need to use below functions to handle tab event. 
*                       1. process_all_gtk_events();
*                       2. process_single_gtk_event();
*                       3. render_web_page_in_tab(uri, b_window);
*                       For more details please Appendix B.
*/
int run_url_browser(int nTabIndex)	
{
	browser_window * b_window = NULL;
	int bytesRead = 0;
	char uri [512];
	// Launch a url rendering browser
	create_browser(URL_RENDERING_TAB, nTabIndex, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm[nTabIndex]);
	// Check for instructions sent from the router or gtk events
	child_req_to_parent req;
	while (1) 
	{
		usleep(1000);
		if((bytesRead = read(comm[nTabIndex].parent_to_child_fd[0], &req, sizeof(child_req_to_parent))) == -1){ 
			if(errno == EAGAIN)			// if nothing to be read yet, process browser events and wait for another round
				process_single_gtk_event();
			else
				perror("run_url_browser@134: Url browser reads from pipe");
		}
		else{							// perform instructions
			switch(req.type){	
				case NEW_URI_ENTERED:	// render the given uri
				    strcpy(uri,req.req.uri_req.uri);
					render_web_page_in_tab(uri, b_window);
					break;
				case TAB_KILLED:		// process gtk events and terminate this process with exit success
					process_single_gtk_event();
					exit(EXIT_SUCCESS);
				default: 				// invalid request type
					fprintf(stderr, "run_url_browser@149: Invalid request for url rendering browser\n");
					break;
			}
		}
	}
	return 0;
}

/*
* Name:                 createPipesAndSetToNonBlock
* Input arguments:      'nTabIndex': a tab index
* Output arguments:     '-1': Failed
*						'0' : Success
* Function:             This function will create pipes for the specified tab index,
*                       and set them to nonblocking.
*/
int createPipesAndSetToNonBlock(int nTabIndex){
	int flagCtoP,flagPtoC;
	//initialize
	comm[nTabIndex].isOpen = false;
	// create pipes
	if( pipe(comm[nTabIndex].parent_to_child_fd) == -1 || pipe(comm[nTabIndex].child_to_parent_fd) == -1){
		perror("createPipesAndSetToNonBlock@171: pipe");
		return -1;
	}
	// set flags to nonblock
	if( (flagPtoC = fcntl (comm[nTabIndex].parent_to_child_fd[0], F_GETFL, 0)) == -1){
		perror("createPipesAndSetToNonBlock@176: fcntl");
		return -1;
	}
	if(fcntl (comm[nTabIndex].parent_to_child_fd[0], F_SETFL, flagPtoC | O_NONBLOCK) == -1){
		perror("createPipesAndSetToNonBlock@180: fcntl");
		return -1;
	}
	if( (flagCtoP = fcntl (comm[nTabIndex].child_to_parent_fd[0], F_GETFL, 0)) == -1){
		perror("createPipesAndSetToNonBlock@184: fcntl");
		return -1;
	}
	if (fcntl (comm[nTabIndex].child_to_parent_fd[0], F_SETFL, flagCtoP | O_NONBLOCK) == -1){
		perror("createPipesAndSetToNonBlock@188: fcntl");
		return -1;
	}
	comm[nTabIndex].isOpen = true;
	return 0;
}

/*
* Name:                 closePipes
* Input arguments:      'nTabIndex': a tab index
* Output arguments:     '-1': Failed
*						'0' : Success
* Function:             This function will close all pipes associated with the specified tab index
*/
int closePipes(int nTabIndex){
	while (close(comm[nTabIndex].child_to_parent_fd[0]) == -1){
		if(errno != EINTR){
			perror("closePipes@204: close pipes");
			return -1;
		}
	}
	while (close(comm[nTabIndex].child_to_parent_fd[1]) == -1){
		if (errno != EINTR){
			perror("closePipes@210: close pipes");
			return -1;
		}
	}
	while (close(comm[nTabIndex].parent_to_child_fd[0]) == -1){
		if (errno != EINTR){
			perror("closePipes@216: close pipes");
			return -1;
		}
	}	
	while (close(comm[nTabIndex].parent_to_child_fd[1]) == -1){ 
		if (errno != EINTR){
			perror("closePipes@222: close pipes");
			return -1;
		}
	}
	comm[nTabIndex].isOpen = false;
	return 0;
}

/*
* Name:                 kill_process
* Input arguments:      'nTabIndex': a tab index
* Output arguments:     '-1': Failed
*						'0' : Success
* Function:             This function will send a kill process message to a url rendering browser process
*/
int kill_process(nTabIndex){
	// set up request
	child_req_to_parent kill_req;
	kill_req.req.killed_req.tab_index = nTabIndex;
	kill_req.type = TAB_KILLED;

	// write to the router-browser pipe
	if(write(comm[nTabIndex].parent_to_child_fd[1], &kill_req, sizeof(child_req_to_parent)) == -1){
		perror("kill_process@246: Failed to write kill tab request");
		return -1;
	}else
		return 0;
}

/*
* Name:                 main
* Input arguments:      none
* Output arguments:     none
* Function:             This function creates the router and handles all child processes,
* 						terminating when the controller is killed by the user.
*/
int main()
{
	pid_t childpid;
	int i,j,k,l,newTabIndex,tailIndex = 0;
	size_t bytesWritten,bytesRead;
	comm = (comm_channel*)calloc (MAX_TAB, sizeof(comm_channel));

	// Create pipes for controller and set to nonblocking read
	if(createPipesAndSetToNonBlock(0) < 0){
		fprintf(stderr, "main@268: Failed to create pipes for controller\n");
		exit(EXIT_FAILURE);
	}
	// Fork to create controller
	if((childpid = fork()) == 0){
		run_control();
	}else{
		
		tailIndex++;		
		child_req_to_parent request;
	
		while(1){
			// Router polls for requests from children (controller + url browsers)
			for(i = 0; i < tailIndex; i++){
				// only read from active processes (indicating pipe is open)
				if(!comm[i].isOpen)	
					continue;
				usleep(1000);

				// Uses non-blocking read 
				bytesRead = read(comm[i].child_to_parent_fd[0], &request, 
					sizeof(child_req_to_parent));

				// continue working with other tabs even if one of them had crashed 
				if( bytesRead == -1 && errno != EAGAIN){
					perror("main@293: Router fails to read tab's request");
					continue;				
				}

				// Router reads a request, act accordingly
				if( bytesRead!= -1){
					switch(request.type){
						case CREATE_TAB:
							// set the new tab index to an appropriate number
							newTabIndex = 0;
							for( l = 1; l < tailIndex; l++){
								if(comm[l].isOpen == false){
									newTabIndex = l;
									break;
								}
							}
							// didn't find an unused slot
							if(!newTabIndex){	
								if(tailIndex <= MAX_TAB)
									newTabIndex = tailIndex++;
								else{
									fprintf(stderr, "main@315: Reached maximum number of tabs\n");
									break;
								}
							}
							// set up pipes for the new tab
							if(createPipesAndSetToNonBlock(newTabIndex) < 0){
								fprintf(stderr, "main@321: Failed to create pipes for new tab\n");
								break;
							}
							// fork to create new tab
							if((childpid = fork()) == 0){
								run_url_browser(newTabIndex);
							}
							break;

						case NEW_URI_ENTERED:
							if(!request.req.uri_req.render_in_tab){
								fprintf(stderr, "main@331: Tab index unspecified\n");
								break;							
							}
							// No url rendering browser existd
							if(tailIndex <= 1){
								fprintf(stderr, "main@336: Create an url window to render the page\n");
								break;
							}
							if(!comm[request.req.uri_req.render_in_tab].isOpen){
								fprintf(stderr, "main@340: The specified tab isn't open\n");
								break;
							}
							if((bytesWritten = write(comm[request.req.uri_req.render_in_tab].parent_to_child_fd[1], 
								&request, sizeof(child_req_to_parent))) == -1){
								perror("main@344: Failed to write url");
							}
							break;

						case TAB_KILLED: //Close file descriptors of corresponding tab's pipes.
							j = request.req.killed_req.tab_index;
							if (j > 0){	// close relevant pipes, kill the process
								if(comm[j].isOpen){
									if(kill_process(j) < 0){
										fprintf(stderr, "main@354: Failed to send kill process request\n");
										fprintf(stderr, "main@354: Cannot kill tab %d process\n",j);
										break;
									}
									if(closePipes(j) < 0){
										fprintf(stderr, "main@359: Tab %d process is killed successfully\n",j);
										fprintf(stderr, "main@359: But failed to close its pipes\n");
										fprintf(stderr, "main@359: OS will close these pipes on program exit\n");
										break;
									}	
									if(j == tailIndex - 1)
										tailIndex--;

								}else{
									fprintf(stderr, "main@368: Tab %d is invalid process to kill\n",j);
									break;
								}
							}else{	
								// Killing controller -> close all tabs  				
								for (k = tailIndex - 1; k > 0; k--){
									if(!comm[k].isOpen)
										continue;
									if(kill_process(k) < 0){
										fprintf(stderr, "main@377: Failed to send kill process request\n");
										fprintf(stderr, "main@377: Cannot kill tab %d process\n",k);
										break;
									}
									if(closePipes(k) < 0){
										fprintf(stderr, "main@382: Tab %d process is killed successfully\n",k);
										fprintf(stderr, "main@382: But failed to close its pipes\n");
										fprintf(stderr, "main@382: OS will close these pipes on program exit\n");
										break; 					
									}	
								}
								closePipes(0);
								exit(EXIT_SUCCESS);
							}	
							break;
						default:
							fprintf(stderr, "main@393: Controller received invalid request\n");
							break;
					}	
				}
			}
		}
	}
	return 0 ;
}


