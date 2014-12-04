/* csci4061 F2013 Assignment 4 
* section: 7 
* date: 12/03/14 
* names: An An Yu, Justin Wells
* UMN Internet ID, Student ID (yuxx0535, 5116372), (wells369, 4227626)
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"

#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100
#define MAX_REQUEST_LENGTH 1024
pthread_t dispatchers[MAX_THREADS];
pthread_t workers[MAX_THREADS];

//Structure for request
typedef struct request_t{
	int		m_socket;
	char*	m_szRequest;
} request;

//Structure for queue.
typedef struct request_queue_t{
	request*	items[MAX_QUEUE_SIZE];
	int			head;
	int			tail;			
	int 		size;
} request_queue;
// Global request queue (circular queue)
request_queue rq;

pthread_mutex_t disLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rqLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rqNotFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t rqNotEmpty = PTHREAD_COND_INITIALIZER;

static int num_dispatcher = 0;
static int num_workers = 0;
static int q_len = 0;
// Dequeue a request
request* dequeue(void){
	request* item;
    if (!rq.size) {
    	fprintf(stderr,"request queue is empty\n");        
    	return NULL;
    }
    rq.size--;
    item = rq.items[rq.head];
    rq.head = (rq.head + 1) % MAX_QUEUE_SIZE;
    return item;
}
// Enqueue a request
void enqueue(request* req){
    // check queue is full
    if (rq.size == q_len){
    	fprintf(stderr,"request queue is full\n");
    	return;
    }
    rq.items[rq.tail] = req;
    rq.tail = (rq.tail + 1) % MAX_QUEUE_SIZE;
    rq.size++;
    return;
}
// map filename to corresponding file type
char* mapFileType(char* filename){
	if( strstr(filename, ".htm") != NULL){
		return "text/html";
	}else if(strstr(filename, ".jpg") != NULL){
		return "image/jpeg";
	}else if(strstr(filename, ".gif") != NULL){
		return "image/gif";
	}else
		return "text/plain";
}
int workerIDLookUp(pthread_t id){
	int i;
	for(i = 0; i < num_workers; i++){
		if (workers[i] == id)
			return i;
	}
	return -1;
}
// function for dispatcher threads
void * dispatch(void * arg){	
	while(true){
		/* accept connection*/
		int connectionfd = accept_connection();
		fprintf(stderr, "Dispatcher gets connection\n");
		if(connectionfd < 0){  
			fprintf(stderr, "Error in accept_connection()@67:\n");
			fprintf(stderr, "A dispatcher exiting\n");
			pthread_mutex_lock(&disLock);
			num_dispatcher--;
			// when the last dispatcher exits, 
			// alert all pending and working workers to prepare for exit as well 
			if (!num_dispatcher){
				fprintf(stderr,"Last dispatcher exiting\n");
				pthread_cond_signal(&rqNotEmpty);	
			}
			pthread_mutex_unlock(&disLock);
			pthread_exit(0);
		}
		/* get_request()*/
		request* newReq = (request*) malloc(sizeof(request));
		newReq->m_socket = connectionfd;
		newReq->m_szRequest = (char*) malloc(sizeof(char) * MAX_REQUEST_LENGTH);
		fprintf(stderr, "[106]connectionfd: %d\n", connectionfd);
		/*place request in queue*/
		while(get_request(newReq->m_socket, newReq->m_szRequest) != 0){			
			fprintf(stderr, "Error in get_request()@75\n");
			continue;				// will try another connection
		}
		fprintf(stderr, "dispatcher gets request : %s\n", newReq->m_szRequest);
		pthread_mutex_lock(&rqLock);
		/* wait until there's an open slot in q, then write req into q*/
		while(rq.size == MAX_QUEUE_SIZE){
			pthread_cond_wait(&rqNotFull, &rqLock);
		}
		enqueue(newReq);
		pthread_cond_signal(&rqNotEmpty);
		pthread_mutex_unlock(&rqLock);
	}
}

void* worker(void * arg){
	struct stat st;
	FILE*flog;
	int fp;
	int fsize,ret,reqNum = 1;
	char file_path[1024];
	char* buf;
	while(true){
		/*wait for work*/
		pthread_mutex_lock(&rqLock);
		/* wait until there's an req q*/
		while(rq.size == 0){
			if (num_dispatcher == 0){
				pthread_mutex_unlock(&rqLock);
				pthread_exit(0);	
			} 
			pthread_cond_wait(&rqNotEmpty, &rqLock);
		}
		fprintf(stderr, "Worker gets a request\n");
		request* newReq = dequeue();
		strcpy(file_path,newReq->m_szRequest);

		fprintf(stderr, "Request content: %s\n", file_path);
		pthread_cond_signal(&rqNotFull);
		pthread_mutex_unlock(&rqLock);

		/* process requested file*/
		
		if ( (fp = open(file_path+1,O_RDONLY)) == -1){
			return_error(newReq->m_socket,"File not found\n");
			pthread_mutex_lock(&logLock);
			flog = fopen("webserver_log","a+");
			fprintf(flog, "[%d][%d][%d][%s][File Not Found.]\n",workerIDLookUp(pthread_self()), reqNum++, newReq->m_socket,file_path);
			pthread_mutex_unlock(&logLock);
			fclose(flog);
			free(newReq);
			continue;
		}
		fstat(fp,&st);
		fsize = st.st_size;
		/* copy the file into a buffer in heap*/
		buf = malloc(fsize);
		if(buf == NULL){
			perror("malloc@165");
			continue;
		}
		read(fp,buf,fsize);
	
	  	/* return result to client*/
		ret = return_result(newReq->m_socket,mapFileType(newReq->m_szRequest),buf,fsize);
		if(ret != 0){
			fprintf(stderr, "Error in return_result@175\n");
		}
		/* free memories*/
		pthread_mutex_lock(&logLock);
		flog = fopen("webserver_log","a+");
		fprintf(flog, "[%d][%d][%d][%s][%d]\n",workerIDLookUp(pthread_self()), reqNum++, newReq->m_socket,newReq->m_szRequest,fsize);
		pthread_mutex_unlock(&logLock);
		close(fp);
		fclose(flog);
		free(newReq);
		free(buf);
	}
}

int main(int argc, char **argv)
{
	// init queue.
	rq.head = 0;
	rq.tail = 0;
	int i,j;
	//Error check first.
	if(argc != 6 && argc != 7)
	{
		printf("usage: %s port path num_dispatcher num_workers queue_length [cache_size]\n", argv[0]);
		return -1;
	}
	// 1: port 2:path 3:num_dispatcher 4:num_workers 5:q_len 6: cache_entries
	if (chdir(argv[2])!=0) {
	      fprintf(stderr,"Cannot chdir to the right directory\n");
      	      return -1;
        }

	/* Init port */
	init(atoi(argv[1]));	
	fprintf(stderr, "port initialized\n");		
	num_dispatcher = atoi(argv[3]); 
	fprintf(stderr, "num_dispatchers %d\n",num_dispatcher);
	num_workers = atoi(argv[4]);
	q_len = atoi(argv[5]);
	/* Check Bad inputs*/
	if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
	     fprintf(stderr,"Invalid num of dispatchers\n");
	     return -1;
	}
	if (num_workers < 1 || num_workers > MAX_THREADS) {
	     fprintf(stderr,"Invalid num of workers\n");
	     return -1;
	}
	if (q_len > MAX_QUEUE_SIZE || q_len < 1) {
	     fprintf(stderr,"Invalid queue length\n");
	     return -1;
	}
	/* Create dispatcher threads*/
	for(i = 0; i < num_dispatcher; i++){
		if( pthread_create(&dispatchers[i], NULL, dispatch, NULL) != 0){
			perror("pthread_create@71:");
			exit(1);
		}
	}
	/* Create worker threads*/
	for(i = 0; i < num_workers; i++){
		if( pthread_create(&workers[i], NULL, worker, NULL) != 0){
			perror("pthread_create@79:");
			exit(1);
		}
	}
	for (i = 0; i < num_dispatcher; i++) pthread_join(dispatchers[i], NULL);
	for (i = 0; i < num_workers; i++) pthread_join(workers[i], NULL);

	fprintf(stderr, "main function waiting to exit\n");
	return 0;
}
