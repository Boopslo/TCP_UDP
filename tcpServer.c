#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define BUFSIZE 256
#define BUFLEN 1025

// function to receive file
void recvFile(int);
// function to send file
void fileSend(int, const char *);

// socketfd: used to connect with clients
// newsockfd: for interprocessing and exchanging data
// charNum: the number of chars read/written
int main(int argc, char *argv[]) {
  /* code */
  int socketfd, newsocketfd, port, clientLength, charNum;
  //uint8_t buffer[BUFSIZE]; // store the data
  struct sockaddr_in serverAddr, clientAddr;
  int receiveData;

  if( (argc != 3) && (argc != 4)) {
    fprintf(stderr, "usage: <%s> <port> <send> <filename>\nusage: <%s> <port> <recv>\n", argv[0], argv[0]);
    exit(1);
  }

  // create socket, it will return an entry to the file descriptor table
  socketfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: Internet domain, SOCK_STREAM: TCP socket
  // if error in creating socket
  if(socketfd < 0) {
    fprintf(stderr, "Create socket error\n");
    exit(1);
  } else {
    printf("socket descriptor obtained, socket created...\n");
  }

  // initialize server value to 0
  bzero((char *)&serverAddr, sizeof(serverAddr));
  port = atoi(argv[1]); // assign port number
  // set server
  serverAddr.sin_family = AF_INET; // set TCP
  serverAddr.sin_port = htons(port); // convert to little endian byte order
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // gets the ip address on which the server is running

  // bind the socket to the ip address
  int bindStatus = bind(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
  if (bindStatus < 0) {
    fprintf(stderr, "socket binding error\n");
    exit(1);
  }

  // listen for connections, upmost 5 connections can be waiting(max)
  int listenStatus = listen(socketfd, 5);
  if(listenStatus < 0) {
    fprintf(stderr, "Connection listening failed.\n");
    exit(1);
  } else {
    printf("Start listening\n");
  }

  int ifConnected = 0;
  int isChatting = 0;
  // loop to continuously listen for new connections, "0" if available for connections
  while(ifConnected == 0) {
    printf("waiting for client to connect...\n");
    // initialize client address
    clientLength = sizeof(clientAddr);
    // use new socket for transferring data
    newsocketfd = accept(socketfd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLength);
    if (newsocketfd < 0) {
      fprintf(stderr, "New socket creation error\n" );
      exit(1);
    } else {
      printf("New socket connected.\n");
    }
    /* check sending file or receiving file */
    if ( strcmp(argv[2], "send") == 0 ) {
      if (argc != 4) {
        fprintf(stderr, "have to specify file name to send.\n");
        exit(1);
      } else {
        // if sending a file
        printf("start sending file...\n");
        fileSend(newsocketfd, argv[3]);
      }
    } else if( strcmp(argv[2], "recv") == 0) {
      if (argc != 3) {
        /* code */
        fprintf(stderr, "argument amount incorrect!\n");
        exit(1);
      } else {
        printf("start receiving file...\n");
        recvFile(newsocketfd);
      }
    } else {
      fprintf(stderr, "argument error.\n");
    }
  } // while connected

  // close socketfd
  if(close(socketfd) != 0) {
    printf("Server-sockfd closing failed!\n");
  } else {
    printf("Server-sockfd successfully closed!\n");
  }

  return 0;
} // end of main()

// function to receive file from the server
void recvFile(int socketfd) {
  uint8_t buf[BUFLEN];
  const char *filename;
  uint8_t recvFileNameBuf[BUFLEN];
  bzero(recvFileNameBuf, BUFLEN);
  // get file name and file size
  int getFileName;
  if( (getFileName = recv(socketfd, recvFileNameBuf, BUFLEN, 0)) > 0 ) {
    printf("get file name: %s", recvFileNameBuf);
  } else {
    fprintf(stderr, "receive file name error\n");
    exit(1);
  }

  /* get file size to calculate packet loss rate */
  uint8_t recvFileSizeBuf[BUFLEN];
  bzero(recvFileSizeBuf, BUFLEN);
  int getFileSize;
  if( (getFileSize = recv(socketfd, recvFileSizeBuf, BUFLEN, 0)) > 0 ) {
    printf("file size: %s\n", recvFileSizeBuf);
  } else {
    fprintf(stderr, "file size receiving error.\n");
    exit(1);
  }

  printf("start receiving file...\n");
  filename = (char *)recvFileNameBuf;
  FILE *file = fopen(filename, "wb");
  if(file == NULL) {
    printf("File %s Cannot be opened file on server.\n", filename);
  } else {
    bzero(buf, BUFLEN);
    int fr_block_sz = 0;

    /* TODO */
    while((fr_block_sz = recv(socketfd, buf, BUFLEN, 0)) > 0) {
      printf("recv: %d from client\n", fr_block_sz);
      int write_sz = fwrite(buf, sizeof(char), fr_block_sz, file);
      if(write_sz < fr_block_sz) {
        fprintf(stderr, "File write failed on server.\n");
        exit(1);
      }
      bzero(buf, BUFLEN);
      if(fr_block_sz <= 0) {
        printf("chen yi\n");
        break;
      }
    }
    printf("File received from client!\n");
    fclose(file);
  }

  close(socketfd);

}


//////// bug ////////
// function to send file
void fileSend(int socketfd, const char *fileToSend) {
  uint8_t buf[1025];
  //const char* filename = fileToSend;
  time_t timer;
  struct tm *tm_info;
  uint8_t timeBuf[30];
  uint8_t charfileSize[20];

  FILE *file = fopen(fileToSend, "rb");
  if (!file) {
    printf("Can't open file for reading");
    return ;
  }
  int fs_block_sz;
  bzero(buf, BUFLEN);
  int count1 = 1, count2 = 1;
  double percent;

  long file_size;
  // check to the end of file
  fseek(file, 0, SEEK_END);
  // get file size
  file_size = ftell(file);
  // goes back to the beginning of file(fp)
  rewind(file);
  printf("file name: %s\n", fileToSend);
  printf("file size: %ld kb\n", file_size/1024);
  // how many percent sent
  percent = file_size/100.0;
  printf("%f\n", percent);

  /* send file name */
  int sendFileName;
  // length of file name
  if( (sendFileName = send(socketfd, fileToSend, strlen(fileToSend), 0)) > 0) {
    printf("file name sent.\n");
  } else {
    perror("file name send error.\n");
    exit(1);
  }
  sleep(1);

  /* send file size */
  int temp = file_size/1024;
  sprintf((char *)charfileSize, "%d", temp);
  // send file size to calculate packet loss rate
  int sendFileSize;
  if( (sendFileSize = send(socketfd, charfileSize, sizeof(charfileSize), 0)) > 0 ) {
    printf("file size sent.\n");
  } else {
    fprintf(stderr, "Failed to send file size.\n");
    exit(1);
  }

  printf("start sending file...\n");

  int sent ;
  time(&timer);
  int count = 1;
  while((fs_block_sz = fread(buf, sizeof(uint8_t), BUFLEN, file)) > 0) {
    printf("count: %d\n", count);
    if((sent = send(socketfd, buf, fs_block_sz, 0)) > 0){
      //printf("sent: %d bytes\n", sent);
      tm_info = localtime(&timer);
      printf("%.2f%% ( %d bytes) sent.\t", (double)(count1 + sent) / percent , sent);
      strftime((char *)timeBuf, sizeof(timeBuf), "%Y:%m:%d %H:%M:%S", tm_info);
      printf("%s\n", timeBuf);
      count1 += sent;
    } else {
      fprintf(stderr, "ERROR: Failed to send file\n");
      fclose(file);
      return;
    }
    bzero(buf, BUFLEN);
    count++;
  }

  printf("File sent.\n");
  //pthread_exit(fileThread);
  fclose(file);
  close(socketfd);
}
