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

/* the port users will be connecting to */
#define BUFLEN 1025


//////// mechanism to send serial number




// send file function declaration
void sendFile(char *, char *, char *);
// receive file function declaration
void recvFile(char *, char *);

int main(int argc, char *argv[]) {

  // check argument amount
  if( (argc != 5) && (argc != 4)) {
    fprintf(stderr, "usage: <%s> <hostname> <port> <send> <filename>\nusage: <%s> <hostname> <port> <recv>\n", argv[0], argv[0]);
    exit(1);
  }

  // check sending or receiving file
  if( argc == 5) {
    // to send file
    sendFile(argv[1], argv[2], argv[4]);
  } else if(argc == 4){
    recvFile(argv[1], argv[2]);
  } else {
    perror("incorrect arguments\n");
    exit(1);
  }

  return 0;
} // end of main

void sendFile(char *hostname, char *portNum, char *filename) {
  int socketfd;
  /* connector’s address information */
  struct sockaddr_in server_addr;
  struct hostent *server;
  int numbytes;
  int port_number;
  uint8_t buffer[BUFLEN];
  // time_t timer and time struct and buffer for storing time data
  time_t timer;
  struct tm *tm_info;
  uint8_t timeBuf[30];
  int count = 1;
  double percent;
  uint8_t charfileSize[20];

  /* get host info */
  if ((server = gethostbyname(hostname)) == NULL) {
    perror("client gethostbyname() error !\n");
    exit(1);
  } else {
    printf("get host name successfully...\n");
  }

  /* create socket file descriptor */
  if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("client create udp socket error !\n");
    exit(1);
  } else {
    printf("client create udp socket done...\n");
  }

  // assign port number
  port_number = atoi(portNum);

  /* host byte order */
  server_addr.sin_family = AF_INET;
  // port number
  server_addr.sin_port = htons(port_number);
  server_addr.sin_addr = *((struct in_addr *)server->h_addr);
  /* zero the rest of the struct */
  memset(&(server_addr.sin_zero), 0, 8);

  // open file and read by bytes
  FILE *fp = fopen(filename, "rb");
  if ( !fp ) {
    // open file error
    perror("Error opening the file\n");
    exit(1);
  } else {
    // successfully opened the file that's going to be transferred
    printf("file opened: %s\n", filename);

    // get file size
    int file_block_size = 0;
    bzero(buffer, BUFLEN);
    long file_size;
    // check to the end of file
    fseek(fp, 0, SEEK_END);
    // get file size
    file_size = ftell(fp);
    // goes back to the beginning of file(fp)
    rewind(fp);
    printf("file name: %s\n", filename);
    printf("file size: %ld kb\n", file_size/1024);
    // get time
    time(&timer);

    //////////// dealing with files ////////////
    /* send file name */
    // send file name and file size first
    int sendFileName;
    // length of file name
    if( (sendFileName = sendto(socketfd, filename, strlen(filename), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) != -1) {
      printf("file name sent.\n");
    } else {
      perror("file name send error.\n");
      exit(1);
    }
    //sleep(1);

    /* send file size */
    // make int to char array
    int temp = file_size/1024;
    sprintf(charfileSize, "%d", temp);
    // send file size to calculate packet loss rate
    int sendFileSize;
    if( (sendFileSize = sendto(socketfd, charfileSize, sizeof(charfileSize), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) ) >= 0 ) {
      printf("file size sent.\n");
    } else {
      fprintf(stderr, "Failed to send file size.\n");
      exit(1);
    }
    //sleep(1);

    /* start sending file */
    int sent;
    percent = file_size/100.0;
    while( (file_block_size = fread(buffer, sizeof(char), BUFLEN, fp)) > 0 ) {
      if((sent = sendto(socketfd, buffer, file_block_size, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) > 0){
        //printf("sent: %d bytes\n", sent);
        tm_info = localtime(&timer);
        printf("%.2f%% ( %d bytes) sent.\t", (double)(count + sent) / percent, sent);
        strftime(timeBuf, sizeof(timeBuf), "%Y:%m:%d %H:%M:%S", tm_info);
        printf("%s\n", timeBuf);
        count += sent;
      } else {
        //printf("%d\n%s\n%d\n", socketfd, buf, fs_block_sz);
        fprintf(stderr, "ERROR: Failed to send file\n");
        fclose(fp);
        exit(1);
      }
      // clean the buffer again
      bzero(buffer, BUFLEN);

    } // end of reading file and sending it to the udp socket, while loop
    printf("file sent.\n");
    //printf("sent %d bytes to %s\n", numbytes, inet_ntoa(server_addr.sin_addr));
  }

  /* close socket file descriptor */
  if (close(socketfd) != 0){
    perror("client socket closing is failed!\n");
  } else {
    printf("client socket successfully closed!\n");
  }

  fclose(fp);
}

void recvFile(char *hostname, char *portNum) {
  int socketfd;
  /* connector’s address information */
  struct sockaddr_in server_addr;
  struct hostent *server;
  int numbytes;
  int port_number;
  const unsigned char *filename;
  uint8_t buffer[BUFLEN];
  // time_t timer and time struct and buffer for storing time data
  time_t timer;
  struct tm *tm_info;
  uint8_t timeBuf[30];
  int percent, count = 1;
  uint8_t charfileSize[20];
  socklen_t serverLength;

  /* get host info */
  if ((server = gethostbyname(hostname)) == NULL) {
    perror("client gethostbyname() error !\n");
    exit(1);
  } else {
    printf("get host name successfully...\n");
  }

  /* create socket file descriptor */
  if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("client create udp socket error !\n");
    exit(1);
  } else {
    printf("client create udp socket done...\n");
  }

  // assign port number
  port_number = atoi(portNum);

  /* host byte order */
  server_addr.sin_family = AF_INET;
  // port number
  server_addr.sin_port = htons(port_number);
  server_addr.sin_addr = *((struct in_addr *)server->h_addr);
  /* zero the rest of the struct */
  memset(&(server_addr.sin_zero), 0, 8);
  serverLength = sizeof(server_addr);

  /////// prevention for unreliable file transfer
  int tempGet;
  if( (tempGet = sendto(socketfd, "send file", 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in))) >= 0 ) {
    printf("check connection and wait....\n");
  }

  //sleep(1);
  ///////////// dealing with files //////////////
  /* receive file name */
  uint8_t recvFileNameBuf[BUFLEN];
  bzero(recvFileNameBuf, BUFLEN);
  // get file name and file size
  int getFileName;
  if ( (getFileName = recvfrom(socketfd, recvFileNameBuf, BUFLEN, 0, (struct sockaddr *)&server_addr, &serverLength)) > 0 ) {
    /* code */
    printf("get file name: %s\n", recvFileNameBuf);
  } else {
    fprintf(stderr, "receive file name error\n");
    exit(1);
  }
  //sleep(1);

  /* get file size to calculate packet loss rate */
  uint8_t recvFileSizeBuf[BUFLEN];
  bzero(recvFileSizeBuf, BUFLEN);
  int getFileSize;
  if( (getFileSize = recvfrom(socketfd, recvFileSizeBuf, BUFLEN, 0, (struct sockaddr *)&server_addr, &serverLength)) >= 0 ) {
    printf("file size: %s\n", recvFileSizeBuf);
  } else {
    fprintf(stderr, "file size receiving error.\n");
    exit(1);
  }
  // transmit file size in char to int
  int file_size;
  sscanf(recvFileSizeBuf, "%d", &file_size);
  file_size *= 1024;
  //sleep(1);

  /* file receiving */
  // create a file named exactly same as received one
  int receivedFileSize = 0;
  filename = recvFileNameBuf;
  FILE *fp = fopen(filename, "wb");
  if(!fp) {
    perror("open file failed.\n");
  } else {
    printf("file %s opened.\n", filename);
    printf("waiting for file transfer...\n");
    int fileget = 0;
    int fileWriteSize = 0;
    int count = 0;
    int toStop = 0;
    // start transferring file
    while(toStop == 0) {
      if( (fileget = recvfrom(socketfd, buffer, BUFLEN, 0, (struct sockaddr *)&server_addr, &serverLength)) > 0 ) {
        printf("got packet from %s with size %d (bytes)\n", hostname, fileget);
        fileWriteSize = fwrite(buffer, sizeof(char), fileget, fp);
        //printf("count: %d\n", count);
        if(fileWriteSize < fileget) {
          fprintf(stderr, "file failed to write in ...\n");
          exit(1);
        }
        bzero(buffer, BUFLEN);
        // check if data is correctly received as a whole
        if (fileget == 0 || fileget != BUFLEN) {
          toStop = 1;
          //break;
        }
        count ++;
        // update received file size
        receivedFileSize += fileget;
      } else {
        printf("socket receives data failed\n");
        exit(1);
      }
      // reset buffer
      bzero(buffer, BUFLEN);
    }
    printf("file receiving done.\n");
  }

  // calculate packet loss rate(compare file size difference)
  double rate = (file_size - receivedFileSize)/(double)file_size;
  printf("file size: %d, received size: %d, packet loss rate: %.2f\n", file_size, receivedFileSize, rate);

  // close socketfd
  if(close(socketfd) != 0){
    printf("Server-sockfd closing failed!\n");
  } else {
    printf("Server-sockfd successfully closed!\n");
  }

  //close(socketfd);
  fclose(fp);
}
