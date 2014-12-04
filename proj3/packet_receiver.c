#include "packet.h"

int msqid = -1;

static message_t message;   /* current message structure */
static mm_t MM;             /* memory manager will allocate memory for packets */
static int pkt_cnt = 0;     /* how many packets have arrived for current message */
static int pkt_total = 1;   /* how many packets to be received for the message */

/* Read the packet from message queue when a SIGIO signal is raised.
 * Handle and store the packet, assemble message when all packets have arrived.
 */
static void packet_handler(int sig) {
  packet_t pkt;
  void *chunk;
  packet_queue_msg pktbuf;
    
  //get the "packet_queue_msg" from the queue.
  if( msgrcv(msqid, &pktbuf, sizeof(packet_queue_msg),0,0) < 0){
    perror("msgrcv:Failed to receive packet");
    exit(1);
  }
  //extract the packet from "packet_queue_msg" and store it in the memory from memory manager
  pkt = pktbuf.pkt;
  // the first packet in a message will set up some fields
  if( pkt_cnt == 0 ){
    pkt_total = pkt.how_many;
  }
  pkt_cnt++;
  chunk = mm_get(&MM);
  //get a piece of free memory to store packet temporarily
  if(memcpy(chunk,&pkt.data,PACKET_SIZE*sizeof(char)) == NULL){
    perror("memcpy:Failed to copy packet into chunk.");
    exit(1);
  } 
  // write the packet to message
  message.data[pkt.which] = chunk;
  message.num_packets++;
}

/* 
 * Create message from packets ... deallocate packets.
 * Return a pointer to the message on success, or NULL
 */
static char *assemble_message() {
  char *msg;
  int i;
  int msg_len = message.num_packets * sizeof(data_t);
  /*Allocate msg and assemble packets into it */
  // Allocate one extra space to make sure no buffer errors
  msg = calloc(msg_len+1,sizeof(char));
  for( i = 0; i < message.num_packets ; i++){
    if( memcpy(&msg[i*PACKET_SIZE],message.data[i],PACKET_SIZE*sizeof(char)) == NULL){
      perror("memcpy:Failed to assemble packets into a string.");
      exit(1);
    }
    mm_put(&MM,message.data[i]);
  }
  msg[msg_len] = '\0';
  /* reset these for next message */
  pkt_total = 1;
  pkt_cnt = 0;
  message.num_packets = 0;
  return msg;
}

/* Input: int (number of messages to pass)
 * Output: none (prints to STDIO)
 * 
 * Main driver that receives the messages from message queue, put there by the 
 * packet_sender program via dynamic memory allocation and signals */
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: packet_sender <num of messages to receive>\n");
    exit(-1);
  }

  int k = atoi(argv[1]); /* no of messages you will get from the sender */
  int i;
  char *msg;

  /* init memory manager for NUM_CHUNKS chunks of size CHUNK_SIZE each */
  if (mm_init(&MM,NUM_CHUNKS, CHUNK_SIZE) < 0){
    fprintf(stderr, "Failed in mm_init\n");
    exit(1);
  }
  
  message.num_packets = 0;
  
  /* initialize msqid to send pid and receive messages from the message queue.*/
  key_t kk = key;
  msqid = msgget(kk, 0666 | IPC_CREAT);
  if(msqid < 0){
    perror("msgget:Unable to create/access the message queue");
    exit(1);
  }
  /*send process pid to the sender via the message queue */
  pid_queue_msg pidMsg;
  pidMsg.mtype = 2;
  pidMsg.pid = getpid();
  if( msgsnd(msqid, (void *)&pidMsg, sizeof(int),0) == -1){
    perror("msgsnd:Failed to send receiver pid to the queue.");
    exit(1);
  }
  
  /*set up SIGIO handler to read incoming packets from the queue. */
  struct sigaction act;  
  act.sa_handler = packet_handler;
  act.sa_flags = 0;
  if(sigfillset(&act.sa_mask) < 0){ //Block out every signal during the handler
    perror("sigfillset:Failed to mask out signals.");
    exit(1);
  } 
  if(sigaction(SIGIO, &act, NULL) < 0){
    perror("sigaction:");
    exit(1);
  }

  /* wait and receive k messages */
  for (i = 1; i <= k; i++) {
    while (pkt_cnt < pkt_total) {
      pause();
    }
    msg = assemble_message();
    if (msg == NULL) {
      perror("Failed to assemble message");
    }
    else {
      fprintf(stderr, "GOT IT: message=%s\n", msg);
      free(msg);
    }
  }

  /*deallocate memory manager */
  mm_release(&MM);
  /* remove the queue once done */
  if( msgctl(msqid, IPC_RMID, NULL) == -1 ){
    perror("msgctl: Failed to remove the message queue.");
    exit(1);
  }
  return EXIT_SUCCESS;
}
