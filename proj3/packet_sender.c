#include <time.h>
#include "packet.h"

static int pkt_cnt = 0;     /* how many packets have been sent for current message */
static int pkt_total = 1;   /* how many packets to send send for the message */
static int msqid = -1;      /* id of the message queue */
static int receiver_pid;    /* pid of the receiver */

/* Generates packet
 * 
   Returns the packet for the current message. The packet is selected randomly.
   The number of packets for the current message are decided randomly.
   Each packet has a how_many field which denotes the number of packets in the current message.
   Each packet is string of 3 characters. All 3 characters for given packet are the same.
   For example, the message with 3 packets will be aaabbbccc. But these packets will be sent out order.
   So, message aaabbbccc can be sent as bbb -> aaa -> ccc
   */
static packet_t get_packet() {
  static int which = -1;
  static int how_many;
  static int num_of_packets_sent = 0;
  static int is_packet_sent[MAX_PACKETS];
  int i;

  packet_t pkt;

  if (num_of_packets_sent == 0) {
    how_many = rand() % MAX_PACKETS;
    if (how_many == 0) {
      how_many = 1;
    }
    printf("Number of packets in current message: %d\n", how_many);
    which = -1;
    for (i = 0; i < MAX_PACKETS; ++i) {
      is_packet_sent[i] = 0;
    }
  }
  which = rand() % how_many;
  if (is_packet_sent[which] == 1) {
    i = (which + 1) % how_many;
    while (i != which) {
      if (is_packet_sent[i] == 0) {
        which = i;
        break;
      }
      i = (i + 1) % how_many;
    } 

  }
  pkt.how_many = how_many;
  pkt.which = which;

  memset(pkt.data, 'a' + which, sizeof(data_t));

  is_packet_sent[which] = 1;
  num_of_packets_sent++;
  if (num_of_packets_sent == how_many) {
    num_of_packets_sent = 0;
  }

  return pkt;
}

/* 
 * On receiving SIGALRM signal, send the packet to the receiver via the message queue.
   Notify the receiver when sent successful by raising SIGIO signal*/
static void packet_sender(int sig) {
  packet_t pkt;
  pkt = get_packet();
  pkt_cnt++;
  pkt_total = pkt.how_many;
  
  // temp is just used for temporarily printing the packet.
  char temp[PACKET_SIZE + 2];
  strcpy(temp, pkt.data);
  temp[3] = '\0';
  printf ("Sending packet: %s\n", temp);

  //Create a packet_queue_msg for the current packet.
  packet_queue_msg pktToSend;
  pktToSend.mtype = 1;
  pktToSend.pkt = pkt;

  //send this packet_queue_msg to the receiver. Handle any error appropriately.
  if( msgsnd(msqid, (void *)&pktToSend, sizeof(packet_queue_msg),0) < 0){
    perror("msgsnd:Failed in sending message.");
    exit(1);
  }
  //send SIGIO to the receiver if message sending was successful.
  if( kill(receiver_pid,SIGIO) == -1){
    perror("kill:Failed to send SIGIO.");
    exit(1);
  }
}

/* Input: int (number of messages to pass)
 * Output: none (prints to STDIO)
 * 
 * Main driver that sends the messages to the message queue via dynamic
 * memory allocation and signals */
int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: packet_sender <num of messages to send>\n");
    exit(-1);
  }
  int k = atoi(argv[1]);/* number of messages  to send */
  srand(time(NULL));    /* seed for random number generator */

  int i;
  struct itimerval interval;
  struct sigaction act;           

  /*Create a message queue :*/ 
  key_t kk = key;
  msqid = msgget(kk, 0666 | IPC_CREAT);        // rwrwrw
  if(msqid < 0){
    perror("msgget:Unable to create/access the message queue");
    exit(1);
  }
  pid_queue_msg msgbuf;
  /*read the receiver pid from the queue and store it for future use*/
  if( msgrcv(msqid, &msgbuf, sizeof(pid_queue_msg),2,0) < 0){
    perror("msgrcs:Failed to get the receiver pid from queue.");
    exit(1);  
  }

  receiver_pid = msgbuf.pid;
  printf("Got pid : %d\n", receiver_pid);
 
  /* set up alarm handler -- mask all signals within it */
  /* The alarm handler will get the packet and send the packet to the receiver.
   * Don't care about the old mask, and SIGALRM will be blocked for us anyway,
   * but we want to make sure act is properly initialized.
   */
  act.sa_handler = packet_sender;
  act.sa_flags = 0;
  if(sigfillset(&act.sa_mask) < 0){ //Block out every signal during the handler
    perror("sigfillset:Failed to mask out signals.");
    exit(1);
  } 
  
  if(sigaction(SIGALRM, &act, NULL) < 0){ //to identify a handler for the signal
    perror("sigaction:");
    exit(1);
  }
  /*  
   * turn on alarm timer ...
   * use  INTERVAL and INTERVAL_USEC for sec and usec values
  */
    interval.it_value.tv_sec = INTERVAL;
    interval.it_value.tv_usec = INTERVAL_USEC;
    interval.it_interval.tv_sec = INTERVAL;
    interval.it_interval.tv_usec = INTERVAL_USEC;
    /* And the timer */
    if (setitimer(ITIMER_REAL, &interval, NULL)) {
    perror("setitimer");
    exit(1);
  }
  
  for (i = 1; i <= k; i++) {
    printf("==========================\n");
    printf("Sending Message: %d\n", i);
    while (pkt_cnt < pkt_total) {
      /* block until next packet is sent. SIGALARM will unblock and call the handler.*/
      pause();
    }
    pkt_cnt = 0;
  }
  return EXIT_SUCCESS;
}
