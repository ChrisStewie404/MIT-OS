#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int fds1[2], fds2[2];

    if(pipe(fds1) < 0 || pipe(fds2) < 0){
      printf("pipe() failed\n");
      exit(1);
    }   

    int pid = fork(), curpid;
    if(pid == 0){
        char c1;
        curpid = getpid();
        if (read(fds1[0], &c1, 1) == 1){
            printf("%d: received ping\n", curpid);
        }
        write(fds2[1], "x", 1);
    } else if(pid < 0){
        printf("fork failed\n");
        exit(1);
    } else {
        char c2;
        curpid = getpid();
        write(fds1[1], "x", 1);
        if (read(fds2[0], &c2, 1) == 1){
            printf("%d: received pong\n", curpid);
        }
    }
    close(fds1[0]);
    close(fds1[1]);
    close(fds2[0]);
    close(fds2[1]);
    exit(0);
}