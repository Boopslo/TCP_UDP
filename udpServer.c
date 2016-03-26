#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define BUFLEN 1025

void sendFile(char *, char []);
void recvFile(char *);


int main(int argc, char *argv[]) {

  // temporarily just specify port, test photo first.
  if( (argc != 3) && (argc != 4)) {
    fprintf(stderr, "usage: <%s> <port> <send> <filename>\nusage: <%s> <port> <recv>\n", argv[0], argv[0]);
    exit(1);
  }

  if( argc == 4) {
    // to send file to client
    sendFile(argv[1], argv[3]);
  } else if(argc == 3){
    // if 3 arguments, means to receive file from client
    recvFile(argv[1]);
  } else {
    perror("incorrect argument amount\n");
    exit(1);
  }

  return 0;
}

void sendFile(char *port, char filename[]) {
  int socketfd;
  /* my address information */
  struct sockaddr_in server_addr;
  /* connector’s address information */
  struct sockaddr_in client_addr;
  socklen_t clientLength;
  int numbytes;
  uint8_t buffer[BUFLEN];
  int portNum = atoi(port);
  time_t timer;
  uint8_t charfileSize[20];
  int count = 0;
  double percent;
  struct tm *tm_info;
  uint8_t timeBuf[30];

  // create socket file descriptor
  if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Server-socket() sockfd error lol!");
    exit(1);
  } else {
    printf("Server-socket() sockfd is OK...\n");
  }

  /* host byte order */
  server_addr.sin_family = AF_INET;
  /* short, network byte order */
  server_addr.sin_port = htons(portNum);
  /* automatically fill with my IP */
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  /* zero the rest of the struct */
  memset(&(server_addr.sin_zero), 0, 8);

  // bind the socket to the server ip address
  if(bind(socketfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("server socket bind to IP error lol!\n");
    exit(1);
  } else {
    printf("successfully bind server ip with socket ...\n");
  }
  // client_addr.sin_port = htons(portNum);
  //client_addr.sin_family = AF_INET;
  clientLength = sizeof(client_addr);

  //*  for ensuring client connection  *//
  int tempGet;
  uint8_t tempBuf[BUFLEN];
  if( (tempGet = recvfrom(socketfd, tempBuf, BUFLEN, 0, (struct sockaddr *)&client_addr, &clientLength)) > 0 ) {
    if( strcmp( tempBuf, "send file" ) == 0) {
      printf("Can start transferring file...\n");
    }
  }

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

    //client_addr.sin_family = AF_INET;
    /////////// dealing with files ///////////
    /* send file name */
    // send file name and file size first
    int sendFileName;
    // length of file name
    if( (sendFileName = sendto(socketfd, filename, strlen(filename), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in))) >= 0) {
      printf("file name sent.\n");
    } else {
      //printf("%d\n", sendFileName);
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
    if( (sendFileSize = sendto(socketfd, charfileSize, sizeof(charfileSize), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in) )) >= 0 ) {
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
      if((sent = sendto(socketfd, buffer, file_block_size, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in))) > 0){
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
    perror("server socket closing is failed!\n");
  } else {
    printf("server socket successfully closed!\n");
  }

} // end of sendFile()

void recvFile(char *port) {
  int socketfd;
  /* my address information */
  struct sockaddr_in server_addr;
  /* connector’s address information */
  struct sockaddr_in client_addr;
  socklen_t clientLength;
  int numbytes;
  uint8_t buffer[BUFLEN];
  time_t timer;

  // create socket file descriptor
  if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Server-socket() sockfd error lol!");
    exit(1);
  } else {
    printf("Server-socket() sockfd is OK...\n");
  }

  /* host byte order */
  server_addr.sin_family = AF_INET;
  /* short, network byte order */
  server_addr.sin_port = htons(atoi(port));
  /* automatically fill with my IP */
  server_addr.sin_addr.s_addr = INADDR_ANY;
  /* zero the rest of the struct */
  memset(&(server_addr.sin_zero), '\0', 8);

  // bind the socket to the server ip address
  if(bind(socketfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("server socket bind to IP error lol!\n");
    exit(1);
  } else {
    printf("successfully bind server ip with socket ...\n");
  }
  // get client length
  clientLength = sizeof(client_addr);

  ///////////// dealing with files //////////////
  /* receive file name */
  uint8_t recvFileNameBuf[BUFLEN];
  // get file name and file size
  int getFileName;
  if ( (getFileName = recvfrom(socketfd, recvFileNameBuf, BUFLEN, 0, (struct sockaddr *)&client_addr, &clientLength)) > 0 ) {
    printf("get file name: %s", recvFileNameBuf);
  } else {
    fprintf(stderr, "receive file name error\n");
    exit(1);
  }
  //sleep(1);

  /* get file size to calculate packet loss rate */
  uint8_t recvFileSizeBuf[BUFLEN];
  int getFileSize;
  if( (getFileSize = recvfrom(socketfd, recvFileSizeBuf, BUFLEN, 0, (struct sockaddr *)&client_addr, &clientLength)) >= 0 ) {
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
  const unsigned char *fileToSave = recvFileNameBuf;
  int receivedFileSize = 0;
  FILE *fp = fopen(fileToSave, "wb");
  if(!fp) {
    perror("open file failed.\n");
  } else {
    printf("file %s opened.\n", fileToSave);
    printf("waiting for file transfer...\n");
    int fileget = 0;
    int fileWriteSize = 0;
    int count = 0;
    int toStop = 0;
    // start transferring file
    while(toStop == 0) {
      if( (fileget = recvfrom(socketfd, buffer, BUFLEN, 0, (struct sockaddr *)&client_addr, &clientLength)) > 0 ) {
        printf("got packet with size %d (bytes)\n", fileget);
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
    printf("server sockfd closing failed!\n");
  } else {
    printf("server sockfd successfully closed!\n");
  }

  //close(socketfd);
  fclose(fp);
}
