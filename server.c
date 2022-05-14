
/*
* server.c - a chat server (and monitor) that uses pipes and sockets
*/

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
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/time.h>

#define MAX_CLIENTS 10

// constants for pipe FDs
#define WFD 1
#define RFD 0

/**
 * nonblock - a function that makes a file descriptor non-blocking
 * @param fd file descriptor
 */
void nonblock(int fd) {
  int flags;

  if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
    perror("fcntl (get):");
    exit(1);
  }
  if (fcntl(fd, F_SETFL, flags | FNDELAY) < 0) {
    perror("fcntl (set):");
    exit(1);
  }

}

/*
* monitor - provides a local chat window
* @param srfd - server read file descriptor
* @param swfd - server write file descriptor
*/
void monitor(int srfd, int swfd) {
  // implement me

  char buf[1024];
  int rbytes, wbytes;
  int a;
  fd_set read_fds;

  nonblock(STDIN_FILENO);
  nonblock(srfd);
  //find maxfd
  while(1){
    FD_ZERO(&read_fds);
    FD_SET(srfd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    a = select(srfd + 1, &read_fds, NULL, NULL, NULL);
    if(a < 0){
      perror("monitor: select\n");
      exit(1);
    }
    if(FD_ISSET(srfd, &read_fds)){
      rbytes = read(srfd, buf, sizeof(buf));
      if(rbytes < 0){
        perror("monitor: read\n");
        exit(1);
      }
      wbytes = write(STDOUT_FILENO, buf, rbytes);
      if(wbytes < 0){
        perror("monitor: write to STDOUT\n");
        exit(1);
      }
    }

    if(FD_ISSET(STDIN_FILENO,&read_fds)){
      rbytes = read(STDIN_FILENO, buf, sizeof(buf));
      if(rbytes == 0){
        printf("hanging up\n");
        break;
      }
      else if(rbytes < 0){
        perror("monitor: read\n");
        exit(1);
      }
      wbytes = write(swfd, buf, rbytes);
      if(wbytes < 0){
        perror("monitor: write to server\n");
        exit(1);
      }
    }
  }
}




/*
* server - relays chat messages
* @param mrfd - monitor read file descriptor
* @param mwfd - monitor write file descriptor
* @param portno - TCP port number to use for client connections
*/
void server(int mrfd, int mwfd, const char* portno, char* monitor, int c) {
  // implement me
  int sockfd, getadd, newsockfd, a, r, s, x, b;
  int val = 1;
  int rbytes, wbytes;
  char buf[1024];
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in their_addr;
  socklen_t sin_size;
  int acclist[MAX_CLIENTS];
  struct timeval time;
  fd_set read_fds, acc_fds;
  int maxsockfd = 0;
  int maxclientfd = 0;


  for (int i = 0; i < MAX_CLIENTS; i++){
    acclist[i] = -1;
  }
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  // hints.ai_flags |= AI_CANONNAME;
  if((getadd = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0){
    perror("server: getaddrinfo\n");
    exit(1);
  }


    p = servinfo;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      perror("server: socket\n");
      exit(1);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0){
      perror("server: setsockopt\n");
      exit(1);
    }
    //bind to clients available
    if((bind(sockfd, p->ai_addr, p->ai_addrlen)) < 0){
      close(sockfd);
      perror("server: bind\n");
      exit(1);
    }
    if((listen(sockfd, MAX_CLIENTS)) < 0){
      perror("server: listen");
      exit(1);
    }

    freeaddrinfo(servinfo);
    time.tv_sec = 0;
    time.tv_usec = 100000;
    nonblock(sockfd);
    nonblock(mrfd);

    while(1){
      FD_ZERO(&read_fds);
      FD_ZERO(&acc_fds);
      FD_SET(sockfd,&read_fds);
      FD_SET(mrfd, &read_fds);

      if(sockfd > mrfd){
        maxsockfd = sockfd;
      }
      else{
        maxsockfd = mrfd;
      }
      for (int i = 0; i < MAX_CLIENTS; i++){
        if(acclist[i] > 0){
          FD_SET(acclist[i], &acc_fds);
        }
      }

      a = select(maxsockfd + 1, &read_fds, NULL, NULL, &time);
      if(a < 0){
        perror("relay server: select\n");
        exit(1);
      }

      if(FD_ISSET(sockfd, &read_fds)){
        sin_size = sizeof(their_addr);
        if((newsockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) < 0){
          perror("server: accept\n");
          exit(1);
        }
        if (c == 1){
          printf("Client connected from: %s\n", inet_ntoa(their_addr.sin_addr));
        }
        for(int i = 0; i < MAX_CLIENTS; i++){
          if(acclist[i] < 0){
            acclist[i] = newsockfd;
            FD_SET(acclist[i], &acc_fds);
            nonblock(acclist[i]);
            break;
          }
        }
      }
      for (int i = 0; i < MAX_CLIENTS; i++){
        if(maxclientfd < acclist[i]){
          maxclientfd = acclist[i];
        }
      }

      b = select(maxclientfd + 1, &acc_fds, NULL, NULL, &time);

      if(b < 0){
        perror("relay server: select\n");
        exit(1);
      }

      for(int i = 0; i < MAX_CLIENTS; i++){
        if(FD_ISSET(acclist[i], &acc_fds)){
          r = recv(acclist[i], buf, sizeof (buf), 0);
          if(r < 0){
            perror("relay server: recv\n");
            exit(1);
          }
          // disconnect or EOF from client
          else if( r == 0){
            FD_CLR(acclist[i], &acc_fds);
            acclist[i] = -1;
            if(c == 1){
              printf("%s has disconnected...\n",inet_ntoa(their_addr.sin_addr));
            }
          }
          x = write(mwfd, buf, r);
          if(x < 0){
            perror("relay server: write to monitor\n");
            exit(1);
          }
            // send to other active client except myself
          for(int j = 0; j < MAX_CLIENTS; j++){
            if(acclist[j] != acclist[i] && acclist[j] > 0){
              s = send(acclist[j], buf, r, 0);
              if(s < 0){
                perror("relay server: send to clients\n");
                exit(1);
              }
            }
          }
        }
      }
      // printf("done write to server\n");
      if(FD_ISSET(mrfd, &read_fds)){
        // printf("data from keyboard avaialbe\n");
        rbytes = read(mrfd, buf, sizeof(buf));
        if(rbytes < 0){
          perror("relay server: read monitor\n");
          exit(1);
        }
        // server EOF
        else if(rbytes == 0){
          break;
        }
        //write to all actives clients
        for(int i = 0; i < MAX_CLIENTS; i++){
          // printf("acclist[%d] is %d\n", i, acclist[i]);
          // printf("Is in read_fds? %d\n", FD_ISSET(acclist[i],&read_fds));
          if(acclist[i] > 0){
            // printf("writing to client %d\n", acclist[i]);
            send(acclist[i], monitor, sizeof(monitor) + 3, 0);
            wbytes = send(acclist[i], buf, rbytes, 0);
            // printf("finish writing to client %d\n", acclist[i]);
            if(wbytes < 0){
              perror("relay server: write to clients\n");
              exit(1);
            }
          }
        }
      }
    }
    // close client sockets
    for(int i = 0; i < MAX_CLIENTS; i++){
      if(acclist[i] > 0){
        close(acclist[i]);
      }
    }
  }


int main(int argc, char **argv) {
  // implement me
  char opt;
  pid_t pid;
  int s2m[2], m2s[2];
  char* port;
  char* mon;
  int cflag = 0;
  int nickflag;

  if(argc == 1){
    perror("select an option -h -p -n -c \n");
    exit(1);
  }

  while((opt = getopt(argc, argv, "h:c:p:n")) != -1 ){
    switch(opt){
      case 'h':
      printf("usage: ./server [-h] [-p port #]\n");
      printf("-h - this help message\n");
      printf("-p # - the port to use when connecting to the server\n");
      break;
      case 'c':
      cflag = 1;
      // port = strdup(argv[optind]);
      break;
      case 'p':
      port = strdup(optarg);
      break;
      case 'n':
      // mon = "[Monitor]: ";
      nickflag = 1;
      break;
    }
  }
  if (nickflag == 1){
    mon = "[Monitor]: ";
  }
  if(pipe(m2s) < 0){
    perror("monitor: pipe\n");
    exit(1);
  }
  if(pipe(s2m) < 0){
    perror("relay server: pipe\n");
    exit(1);
  }
  pid = fork();
  if(pid < 0){
    perror("fork\n");
    exit(1);
  }
  else if (pid == 0){
    //child process : Monitor
    close(s2m[WFD]);
    close(m2s[RFD]);
    monitor(s2m[RFD],m2s[WFD]);
    exit(0);
  }
  else{
    //parent process : Relay server
    close(m2s[WFD]);
    close(s2m[RFD]);
    server(m2s[RFD], s2m[WFD], port, mon, cflag);
    wait(NULL);
  }
  return 0;
}
