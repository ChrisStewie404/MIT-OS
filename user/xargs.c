#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define BUFSIZ 32
int
main(int argc, char *argv[])
{
    char *eargs[MAXARG + 1];
    int base_argc = argc - 1;
    
    for(int i = 1; i < argc; i++){
        eargs[i-1] = argv[i];
    }

    char new_args[MAXARG][BUFSIZ];
    int arg_idx = 0;
    int char_idx = 0;
    char c;

    while(read(0, &c, 1) == 1){
        if(c == ' ' || c == '\n'){
            if(char_idx > 0){
                new_args[arg_idx][char_idx] = '\0';
            }

            eargs[base_argc + arg_idx] = new_args[arg_idx];
            arg_idx++;
            char_idx = 0;

            if(c == '\n'){
                eargs[base_argc + arg_idx] = 0;
                if(arg_idx > 0){
                    int pid = fork();
                    if(pid == 0){
                        exec(eargs[0], eargs);
                        exit(1);
                    } else if(pid > 0){
                        wait(0);
                    } else{
                        printf("fork failed\n");
                        exit(1);                        
                    }
                }
                arg_idx = 0;
            }
        } else{
            new_args[arg_idx][char_idx++] = c;
        }
    }

    exit(0);
}