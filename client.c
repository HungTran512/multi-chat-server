#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/un.h>

/**
 * nonblock - a function that makes a file descriptor non-blocking
 * @param fd file descriptor
 */
void nonblock(int fd) {
  int flags;

  if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
    perror("fcntl (get):");
    exit(1);
  }
  if (fcntl(fd, F_SETFL, flags | FNDELAY) == -1) {
    perror("fcntl (set):");
    exit(1);
  }
}

int main(int argc, char *argv[]){
  int sockfd,status, getadd, wbytes, rbytes;
  char buf[1024];
  char opt;
  struct addrinfo hints, *servinfo;
  char* port;
  char* server;
  char* name;
  struct timeval time;
  fd_set read_fds;
  int a;
  int serflag;
  // options for clients

  if(argc == 1){
    perror("select an option -h or -p\n");
    exit(1);
  }

  while((opt = getopt(argc, argv, "p:h:n")) != -1 ){
    switch(opt){
      case 'h':
      // server = strdup(argv[2]);
      serflag = 1;
      break;
      case 'p':
      port = strdup(optarg);
      break;
      case 'n':
      name = malloc(strlen(argv[6]) + 20);
      name = strdup("[");
      strcat(name, argv[6]);
      strcat(name, "]: ");
      break;
    }
  }

  if (serflag == 1){
    server = strdup(argv[2]);
  }
  if (strcmp(server, "senna") != 0){
    perror("only avail server is senna pls type senna I beg u\n");
    exit(1);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if((getadd = getaddrinfo(server, port, &hints, &servinfo)) != 0){
    perror("client: getaddrinfo\n");
    exit(1);
  }
  //set up socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("client: socket error\n");
    exit(1);
  }
  if ((status = connect(sockfd, servinfo->ai_addr,servinfo->ai_addrlen)) == -1){
    close(sockfd);
    perror("client: connect\n");
    exit(1);
  }

  printf("connecting to server....\n");

  if (servinfo == NULL) {
    perror("client: failed to connect\n");
    exit(1);
  }
  freeaddrinfo(servinfo);

  time.tv_sec = 0;
  time.tv_usec = 100000;
  nonblock(sockfd);
  nonblock(STDIN_FILENO);
  while(1){
    FD_SET(sockfd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    a = select(sockfd + 1, &read_fds, NULL, NULL, &time);

    if(a == -1){
      perror("client: select\n");
      exit(1);
    }

    if(FD_ISSET(STDIN_FILENO,&read_fds)){
      rbytes = read(STDIN_FILENO, buf, sizeof(buf));
      if(rbytes == 0){
        printf("hanging up\n");
        break;
      }
      else if(rbytes == -1){
        perror("client:read\n");
        exit(1);
      }
      send(sockfd, name, sizeof(name)+ 3, 0);
      wbytes = send(sockfd, buf, rbytes, 0);
      if(wbytes == -1){
        perror("client:write\n");
        exit(1);
      }
    }
    else if(FD_ISSET(sockfd,&read_fds)){
      rbytes = recv(sockfd, buf, sizeof(buf), 0);
      if(rbytes == -1){
        perror("client:read\n");
        exit(1);
      }
      else if(rbytes == 0){
        break;
      }
      wbytes = write(STDOUT_FILENO, buf, rbytes);
      if(wbytes == -1){
        perror("client:write\n");
        exit(1);
      }
    }
  }
  free(name);
  close(sockfd);
  return 0;
}
