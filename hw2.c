#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h> 
#include <signal.h>

pid_t pid; //signal for parent or child

void sigIntHandler(){
    kill(pid,SIGINT);
}

int main() {
    signal(SIGINT, sigIntHandler);
    
    char input[128];
    
    while (1) {

        printf("prompt > ");
        
        if (fgets(input, 128, stdin) == NULL) {
            perror("Failed to read input");
            exit(1);
        }

        char *command = strtok(input, " \n");
        if (command == NULL) {
            continue;
        }

        if (strcmp(command, "cd") == 0) {
            char *arg = strtok(NULL, " \n");
            if (arg == NULL) {
                printf("cd: missing directory\n");
            } else {
                if (chdir(arg) != 0) {
                    perror("cd");
                }
            }
        } else if (strcmp(command, "pwd") == 0) {
            char currentDir[128];
            if (getcwd(currentDir, sizeof(currentDir)) != NULL) {
                printf("%s\n", currentDir);
            } else {
                perror("pwd");
            }
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "counter") == 0){
            pid = fork(); //checks if it is a parent of a child
            int child_status; //signals when the child is done executing

            if (pid == 0){ //child 
                signal(SIGINT, SIG_DFL);
                printf("hello from child\n");
                unsigned int i = 0;
                while(1){
                    printf("Counter: %d\n", i);
                    i++;
                    sleep(1);
                }

            }
            else{ //parent
                printf("hello from parent\n");
                waitpid(pid,&child_status,0);

                printf("child is done executing\n");

            }

        } else {
            printf("Error: Invalid command.\n");
        }
    }
    //test to see if i can make changes
    printf("Shell terminated.\n");
    return 0;
}