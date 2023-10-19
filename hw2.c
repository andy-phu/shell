#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/wait.h>

pid_t pid; //signal for parent or child
pid_t foreground_pid = -1;
int job_id_counter = 1;

struct Job{
    pid_t pid;
    int job_id;
    int state; // 0 for Foreground/Running, 1 for Background/Running, 2 for Stopped
    char* command_line;
};

struct Job jobs[128];



void foreground_handler(int signal){
    if (foreground_pid != -1){
        kill(pid,SIGINT);
    }
}

void suspend_handler(int signal){
    if (foreground_pid != -1){
        for (int i = 0; i < job_id_counter; i++) {
            if (jobs[i].pid == foreground_pid) {
                jobs[i].state = 2;
            }
        }
        printf("child process with PID: %d suspended\n",pid);
        kill(foreground_pid,SIGTSTP);
        foreground_pid = -1;

    } 
}

void background_handler(int signal){
    int child_status;
    
    pid = waitpid(-1, &child_status, WNOHANG);
    if (pid > 0){
        for (int i = 0; i < job_id_counter; i++) {
            if (jobs[i].pid == pid) {
                jobs[i] = jobs[i+1];
            }
        }
        printf("child process with PID: %d terminated\n",pid);
    }
}

int main() {
    signal(SIGINT, foreground_handler);
    signal(SIGCHLD, background_handler);
    signal(SIGTSTP, suspend_handler);

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
        } else if (strcmp(command, "jobs") == 0){
            for (int i = 0; i < job_id_counter; i++) {
                if (jobs[i].state != 0) {
                    char *status;
                    if (jobs[i].state == 0 || jobs[i].state == 1){
                        status = "Running";
                    }else{
                        status = "Stopped";
                    }

                    printf("[%d] (%d) %s %s\n", jobs[i].job_id, jobs[i].pid, status, jobs[i].command_line);
                }
            }

        } else {
            if (access(command, X_OK) == 0) 
            {
                char cl[128];
                strcpy(cl, command);
                pid_t pid = fork();
                int status;
                if (arg != NULL && strcmp(arg, "&") == 0){//run the background task 
                    strcat(cl, " ");
                    strcat(cl, arg);
                    if (pid == 0) { // Child process
                        char *args[] = {command, NULL};
                        if (execv(command, args) == -1) {
                            perror("execv");
                            exit(1);
                        }
                    }else {
                        // Parent process
                        jobs[job_id_counter].pid = pid;
                        jobs[job_id_counter].job_id = job_id_counter;
                        jobs[job_id_counter].state = 1;
                        jobs[job_id_counter].command_line = cl;
                        job_id_counter++;
                        printf("Background process running\n");
                    }

                }
                else{
                    if (pid == 0) { //run the foreground task
                        // Child process
                        char *args[] = {command, NULL};
                        if (execv(command, args) == -1) {
                            perror("execv");
                            exit(1);
                        }
                    } else {
                        foreground_pid = getpid();
                        jobs[job_id_counter].pid = pid;
                        jobs[job_id_counter].job_id = job_id_counter;
                        jobs[job_id_counter].state = 0;
                        jobs[job_id_counter].command_line = cl;
                        job_id_counter++;
                        // Parent process
                        waitpid(pid, &status, 0);
                        foreground_pid = -1;
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