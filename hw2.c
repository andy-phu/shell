#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/wait.h>

pid_t pid; //signal for parent or child
pid_t foreground_pid = -1;

void foreground_handler(int signal){
    if (foreground_pid != -1){
        kill(pid,SIGINT);
    }
}

void background_handler(int signal){
    int child_status;
    
    pid = waitpid(-1, &child_status, WNOHANG);
    if (pid > 0){
        printf("child process with PID: %d terminated\n",pid);
    }
}

int main() {
    signal(SIGINT, foreground_handler);
    signal(SIGCHLD, background_handler);

    char input[128];
    
    while (1) {

        printf("prompt > ");
        
        if (fgets(input, 128, stdin) == NULL) {
            perror("Failed to read input");
            exit(1);
        }

        char *command = strtok(input, " \n");
        char *arg = strtok(NULL, " \n"); //will check if there is an &

        if (command == NULL) {
            continue;
        }
        
        if (strcmp(command, "cd") == 0) {
            //removed the strtok arg here because we need to check for & for stage 3 
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

        } else {
            if (access(command, X_OK) == 0) 
            {
                pid_t pid = fork();
                int status;
                if (arg != NULL && strcmp(arg, "&") == 0){//run the background task 
                    if (pid == 0){
                        // Child process
                        char *args[] = {command, NULL};
                        if (execv(command, args) == -1) {
                            perror("execv");
                            exit(1);
                        }
                    } 
                    
                    printf("background process running\n");

                }
                else{
                    if (pid == 0) {
                        // Child process
                        char *args[] = {command, NULL};
                        if (execv(command, args) == -1) {
                            perror("execv");
                            exit(1);
                        }
                    } else {
                        // Parent process
                        waitpid(pid, &status, 0);
                    }
                }
            } else {
                printf("Error: Invalid command.\n");
            }
        }
    }
    printf("Shell terminated.\n");
    return 0;
}