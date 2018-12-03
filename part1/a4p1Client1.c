#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 10010 /*port*/
//127.0.0.1
int
main(int argc, char **argv)
{
 int sockfd;
 struct sockaddr_in servaddr;
 char sendline[MAXLINE], recvline[MAXLINE];
 char* fileName;
 FILE* file;

 // alarm(300);  // to terminate after 300 seconds

 //basic check of the arguments
 //additional checks can be inserted
 if (argc !=4) {
  perror("Usage: TCPClient <Server IP> <Server Port><log name>");
  exit(1);
 }
 fileName = argv[3];

 printf("Filename:%s\n",fileName);

 //Create a socket for the client
 //If sockfd<0 there was an error in the creation of the socket
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  perror("Problem in creating the socket");
  exit(2);
 }

 //Creation of the socket
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr= inet_addr(argv[1]);
 servaddr.sin_port =  htons((int) strtol(argv[2], (char **)NULL, 10));
 //servaddr.sin_port =  htons(SERV_PORT); //convert to big-endian order

 //Connection of the client to the socket
 if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  perror("Problem in connecting to the server");
  exit(3);
 }
 printf("\n:>");
 while (fgets(sendline, MAXLINE, stdin) != NULL) {

  send(sockfd, sendline, strlen(sendline), 0);
  if (strcmp(sendline,"exit\n")==0) exit(0);

  if (recv(sockfd, recvline, MAXLINE,0) == 0){
   //error: server terminated prematurely
   perror("The server terminated prematurely");
   exit(4);
  }
  system("hostname; date");
  printf("%s", "String received from the server: ");
  fputs(recvline, stdout);
  file = fopen(fileName, "a");
  fwrite(recvline, 1 , strlen(recvline) , file );
  fwrite("---------------------\n",1,strlen("---------------------\n"),file);
  fclose(file);
  printf("-------------------------------\n");
  printf("\n:>");
 }

 exit(0);
}
