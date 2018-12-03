/*
1)  Concurrent server
2)  Has a pool of threads of which each thread will serve each client when it is
    connected
3)  Each thread can be created either:
      (1)   in the beginning of the server as it starts
      or(2) when a client is connecteed.
4)  Once the server is up and listening, the client connects to the server and
    runs in a loop.
5)  The server assigns its worker-thread to serve the client.
6)  The worker-thread goes in a loop:
      (1) receive a transaction from client.
      (2) process the transaction
      (3) send its result back to the client
      (4) to go back in a loop.
7)  For each transaction in loop, the worker-thread displays:
      (1) the thread information (the thread id)
      (2) the server information
      (3) the hostname and the date/time of the transaction being processed
      (4) the transaction from the client
      (5) the result of the transaction to the server console.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>

/*
    CONCURRENT SERVER: THREAD EXAMPLE
    Must be linked with the "pthread" library also, e.g.:
       cc -o example example.c -lnsl -lsocket -lpthread

    This program creates a connection socket, binds a name to it, then
    listens for connections to the sockect.  When a connection is made,
    it accepts messages from the socket until eof, and then waits for
    another connection...

    This is an example of a CONCURRENT server -- by creating threads several
    clients can be served at the same time...

    This program has to be killed to terminate, or alternately it will abort in
    120 seconds on an alarm...
*/

//#define PORTNUMBER 10010

struct serverParm {
           int connectionDesc;
};

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
char* logfilename;
void logFile(char* msg){
  FILE* file;

  pthread_mutex_lock(&mtx);

  file = fopen(logfilename, "a");
  fwrite(msg, 1 , strlen(msg) , file );
  fclose(file);

  pthread_mutex_unlock(&mtx);
}

#define FILEM 8192
char* readFile(char* fileName){
  char fileBuf[FILEM];
  FILE *file;
  int rr;
  char* result;

  memset(fileBuf,'\0',FILEM);
  //printf("filename:%s\n",fileName);

  file = fopen(fileName, "r");
  if (file == NULL){
    printf("Error: file %s not found/in_read_mode.\n",fileName);
    return (char*)NULL;
  }//else
  rr = fread(fileBuf,FILEM-1,1,file);
  if (rr != 0){
    if (rr < 0){
      printf("Error reading file. rr:%d\n",rr);
      return (char*)NULL;
    }
    else {//rr > 0
      printf("Error file size > %d. rr:%d\n",FILEM,rr);
      return (char*)NULL;
    }
  }

  //printf("L:%lu\n", strlen(fileBuf));
  result = malloc((strlen(fileBuf) + 1) * sizeof(char));
  strcpy(result,fileBuf);
  return result;
}

void *serverThread(void *parmPtr) {

#define PARMPTR ((struct serverParm *) parmPtr)
    int recievedMsgLen;
    char messageBuf[8192];
    char systemCmd[8192];
    int ptid = (int)pthread_self();
    char toCon[8192];
    char* sendit;
    char fileName[8192];
    int i;


    /* Server thread code to deal with message processing */
    printf("DEBUG: connection made, connectionDesc=%d\n",
            PARMPTR->connectionDesc);
    if (PARMPTR->connectionDesc < 0) {
        printf("Accept failed\n");
        return(0);    /* Exit thread */
    }

    /* Receive messages from sender... */
    while ((recievedMsgLen=
            read(PARMPTR->connectionDesc,messageBuf,sizeof(messageBuf)-1)) > 0)
    {
        recievedMsgLen[messageBuf] = '\0';

        //printf("%s\n",toCon);
        sprintf(fileName,"%d.txt",ptid);

        messageBuf[strlen(messageBuf)-1]='\0';
        //printf("MB:%s\n", messageBuf);

        sprintf(systemCmd,"(%s) > %s",messageBuf,fileName);
        //printf("systemCMD:%s\n",systemCmd);
        if (strcmp (messageBuf,"exit") == 0){
          system("hostname ; date");
          sprintf(toCon,"Thread id:%d\ntransaction:%s\n\nresult:%s\n%s",
          ptid,messageBuf,"DISCONECTING CLIENT",
          "-------------------------------\n");
          printf("%s",toCon);
          logFile(toCon);
          pthread_exit(NULL);
        }
        system(systemCmd);
        sendit = readFile(fileName);
        //printf("SEND IT:%s\n",sendit);
        system("hostname ; date");
        sprintf(toCon,"Thread id:%d\ntransaction:%s\n\nresult:%s\n%s",
        ptid,messageBuf,sendit,
        "-------------------------------\n");
        printf("%s\n",toCon);
        logFile(toCon);



        if (write(PARMPTR->connectionDesc,sendit,1025) < 0) {
               perror("Server: write error");
               return(0);
       }

       sprintf(toCon,"rm %s",fileName);
       system(toCon);

    }
    close(PARMPTR->connectionDesc);  /* Avoid descriptor leaks */
    free(PARMPTR);                   /* And memory leaks */
    return(0);                       /* Exit thread */
}

int main (int argc , char** argv) {
    int listenDesc, portnumber;
    struct sockaddr_in myAddr;
    struct serverParm *parmPtr;
    int connectionDesc;
    pthread_t threadID;

    if (argc !=3) {
     perror("Usage: <Port><log name>");
     exit(1);
    }
    portnumber = atoi(argv[1]);
    logfilename=argv[2];

    /* For testing purposes, make sure process will terminate eventually */
    alarm(600);  /* Terminate in 600 seconds */

    /* Create socket from which to read */
    if ((listenDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("open error on socket");
        exit(1);
    }

    /* Create "name" of socket */
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    //printf("myAddr.sin_addr.s_addr:%u",INADDR_ANY);
    myAddr.sin_port = htons(portnumber);

    if (bind(listenDesc, (struct sockaddr *) &myAddr, sizeof(myAddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    /* Start accepting connections.... */
    /* Up to 5 requests for connections can be queued... */
    listen(listenDesc,5);

    while (1) /* Do forever */ {
        /* Wait for a client connection */
        connectionDesc = accept(listenDesc, NULL, NULL);

        /* Create a thread to actually handle this client */
        parmPtr = (struct serverParm *)malloc(sizeof(struct serverParm));
        parmPtr->connectionDesc = connectionDesc;
        if (pthread_create(&threadID, NULL, serverThread, (void *)parmPtr)
              != 0) {
            perror("Thread create error");
            close(connectionDesc);
            close(listenDesc);
            exit(1);
        }

        printf("Parent ready for another connection\n");
    }

}
