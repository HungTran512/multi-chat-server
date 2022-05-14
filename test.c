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
#include <ctype.h>

#define WFD 1
#define RFD 0
int x, y, z, b, c, d;
int cnt = 0;
int main(int argc, char **argv) {
 pid_t pid;

 char* buf[1024];
 int fd[2];
 // pthread_t id;
 //
 // pthread_create(&id, NULL, *void(main), *argv);
 pid = fork();
 pipe(fd);
 if (pid == 0){
   //child
   while(1){
     cnt ++;
     printf("guess!\n");
     y = read(STDIN_FILENO, buf, sizeof(buf));
     printf("y: %d\n", y);
     printf("x from child: %d\n", x);
     if(x == y){
       break;
     }
   }
   return 0;
 }
 else{
   //parent
   close(fd[1]);
   x = rand()%10;
   b = read(fd[RFD], buf, sizeof(buf));
   printf("x: %d\n", x);
   wait(NULL);
   printf("num guesses: %d\n", cnt);
 }
}
