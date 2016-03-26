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

void fileSend(int, const char *);
//void *threadFileSend(void *);
void recvFile(int);


int main(int argc, char *argv[]) {
  int socketfd, port, sendData;
  struct sockaddr_in serverAddr;
  struct hostent *server; // server data
  uint8_t buffer[BUFSIZE];

  if( (argc != 5) && (argc != 4)) {
    fprintf(stderr, "usage: <%s> <hostname> <port> <send> <filename>\nusage: <%s> <hostname> <port> <recv>\n", argv[0], argv[0]);
    exit(1);
  }

  // set port
  port = atoi(argv[2]);
  // set up socket file descriptor: SOCK_STREAM
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0) {
    fprintf(stderr, "create socket file descriptor failed.\n");
    exit(1);
  }

  // get server name
  server = gethostbyname(argv[1]);
  // if the hostname doesn't exist, error happened
  if (server == NULL) {
    fprintf(stderr, "No such host\n");
  }

  // flush the server buffer then initialize server fields
  bzero((char *)&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  // bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
  memcpy((char *)&serverAddr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

  // try connect to the server
  int connectStatus = connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
  if(connectStatus < 0) {
    fprintf(stderr, "Error in connecting to the server...\n");
    exit(1);
  }

  /* check sending file or receiving file */
  if ( strcmp(argv[3], "send") == 0 ) {
    if (argc != 5) {
      /* code */
      fprintf(stderr, "have to specify file name to send.\n");
      exit(1);
    } else {
      // if sending a file
      printf("start sending file...\n");
      fileSend(socketfd, argv[4]);
    }
  } else if( strcmp(argv[3], "recv") == 0) {
    if (argc != 4) {
      /* code */
      fprintf(stderr, "argument amount incorrect!\n");
      exit(1);
    } else {
      printf("start receiving file...\n");
      recvFile(socketfd);
    }
  } else {
    fprintf(stderr, "argument error.\n");
  }

  close(socketfd);
  printf("Socket closed.\n");

  return 0;
} // end of main()

//////////  bug  ///////////
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
    int check = 0;
    int count = 1;
    int countSize = 0;
    while((fr_block_sz = recv(socketfd, buf, BUFLEN, 0)) > 0) {
      ////////// asdadghsakgfj
      printf("%d\n", count);
      /* code */
      printf("recv: %d from server\n", fr_block_sz);
      int write_sz = fwrite(buf, sizeof(uint8_t), fr_block_sz, file);
      if(write_sz < fr_block_sz) {
        fprintf(stderr, "File write failed on server.\n");
      }
      bzero(buf, BUFLEN);
      if (fr_block_sz <= 0 || countSize == getFileSize ) {
        printf("chen yi\n");
        //check = 1;
        break;
      }
      countSize += fr_block_sz;
      count++;

    }
    printf("File received from server!\n");
    fclose(file);
  }

  //close(socketfd);
}

void fileSend(int socketfd, const char *fileToSend) {
  uint8_t buf[1025];
  //const char* filename = fileToSend;
  time_t timer;
  struct tm *tm_info;
  uint8_t timeBuf[30];
  uint8_t charfileSize[20];

  FILE *file = fopen(fileToSend, "rb");
  if (!file)  {
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
  // make int to char array
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

  sleep(1);

  printf("start sending file...\n");

  int sent ;
  time(&timer);
  while((fs_block_sz = fread(buf, sizeof(char), BUFLEN, file)) > 0) {
    if((sent = send(socketfd, buf, fs_block_sz, 0)) > 0){
      //printf("sent: %d bytes\n", sent);
      tm_info = localtime(&timer);
      printf("%.2f%% ( %d bytes) sent.\t", (double)(count1 + sent) / percent , sent);
      strftime((char *)timeBuf, sizeof(timeBuf), "%Y:%m:%d %H:%M:%S", tm_info);
      printf("%s\n", timeBuf);
      count1 += sent;
    } else {
      //printf("%d\n%s\n%d\n", socketfd, buf, fs_block_sz);
      fprintf(stderr, "ERROR: Failed to send file\n");
      fclose(file);
      return;
    }
    bzero(buf, BUFLEN);
  }

  printf("File sent.\n");
  //pthread_exit(fileThread);
  fclose(file);
  //close(socketfd);
}
