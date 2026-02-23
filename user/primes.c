#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
filter(int fd)
{

    int start;
    if (!read(fd, &start, 4)){
        return;
    }
    else{
        printf("prime %d\n", start);
    }

    int fds[2];
    if(pipe(fds) < 0){
      printf("pipe() failed\n");
      exit(1);
    }  
    
    int pid = fork();
    if(pid == 0){
        close(fd);
        close(fds[1]);
        filter(fds[0]);

    } else if(pid < 0){
        printf("fork failed\n");
        exit(1);
    } else {
        close(fds[0]);
        int i;
        while(read(fd, &i, 4) != 0) {
            if(i % start != 0){
                write(fds[1], &i, 4);
            }
        }
        close(fd);
        close(fds[1]); 

        wait(0);
    }  
}

int
main(int argc, char *argv[])
{
    int fds[2];
    if(pipe(fds) < 0){
      printf("pipe() failed\n");
      exit(1);
    }  

    int pid = fork();
    if(pid == 0){
        close(fds[1]);
        filter(fds[0]);
    } else if(pid < 0){
        printf("fork failed\n");
        exit(1);
    } else {
        close(fds[0]);
        for(int i= 2; i<=35; i++){
            write(fds[1], &i, 4);
        }
        close(fds[1]);

        wait(0);
    }
    exit(0);
}

