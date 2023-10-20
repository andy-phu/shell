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
    //printf("foreground handler %d\n",getpgid(pid));
    if (foreground_pid == -1 && getpgid(pid) == getpgid(foreground_pid)){
        printf("trying to kill the background with command\n");
    }
    else if (foreground_pid != -1){
        kill(foreground_pid, SIGINT);

    }
}

void suspend_handler(int signal){
    if (foreground_pid == -1 && getpgid(pid) == getpgid(foreground_pid)){
        printf("trying to suspend the background with command\n");
    }
    else if (foreground_pid != -1){
        for (int i = 0; i < job_id_counter; i++) {
            if (jobs[i].pid == foreground_pid) {
                jobs[i].state = 2;
            }
        }
        printf("child process with PID: %d suspended\n",foreground_pid);
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

void continue_handler(int signal){
    int status;
    printf("\ncontinue");
}

int main() {
    signal(SIGINT, foreground_handler);
    signal(SIGCHLD, background_handler);
    signal(SIGTSTP, suspend_handler);
    signal(SIGCONT, continue_handler);

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
        }else if (strcmp(command, "fg") == 0) {
            char percent = arg[0]; //converting percent from a char to a string 
            int jid = atoi(&arg[1]);

            if (percent == '%'){ //arg[0] would just be the %
                printf("percent: %c\n",percent);
                printf("jid: %d\n",jid);
                for (int i = 0; i < job_id_counter; i++) {
                    if (jid == i){ //checks to see if the jid that the user entered exists in the job array
                        jobs[jid].state = 0; //change state to 0, foreground
                        printf("hello\n");
                        int status;
                        if (kill(jobs[jid].pid, SIGCONT) == 0){
                            foreground_pid = jobs[jid].pid;
                            waitpid(jobs[jid].pid, &status, WUNTRACED);
                            foreground_pid = -1;
                            break; //gets out of the for loop once the foreground has been activated
                        }
                    }
                }
            }else{
                int temp_pid = atoi(arg);
                int status;
                for (int i = 0; i < job_id_counter; i++) {
                    if (jobs[i].pid == temp_pid) {
                        jobs[i].state = 0; //change state to 0, foreground
                        if (kill(jobs[i].pid, SIGCONT) == 0){
                            foreground_pid = jobs[i].pid;
                            waitpid(jobs[i].pid, &status, WUNTRACED);
                            foreground_pid = -1;
                        }
                    }
                }
            }
            }else if (strcmp(command, "bg") == 0) {
                char percent = arg[0]; //converting percent from a char to a string 
                int jid = atoi(&arg[1]);
                printf("percent: %c\n",percent);
                printf("jid: %d\n",jid);
                if (percent == '%'){ //arg[0] would just be the %
                    printf("BG HELLO\n");
                    int status;
                    if (kill(jobs[jid].pid, SIGCONT) == 0){ //continues the stopped prcocess
                        printf("suspended is now in the background\n");
                        jobs[jid].state = 1; //changes state to the background
                        setpgid(pid,foreground_pid); //sets the pgid to pid to put it in the background group
                    }
                }else{
                    int temp_pid = atoi(arg);
                    int status;
                    if (kill(temp_pid, SIGCONT) == 0){ //continues the stopped prcocess
                        for (int i = 0; i<job_id_counter; i++){
                            if (jobs[i].pid == temp_pid){ //finds the right bucket
                                jobs[jid].state = 1; //changes state to the background
                                setpgid(pid,foreground_pid); //sets the pgid to pid to put it in the background group
                                break;
                            }
                     
                        }

                    }
                }
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "jobs") == 0){
            for (int i = 1; i < job_id_counter; i++) {
                if (jobs[i].job_id > 0){
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
                int waitCondition = WUNTRACED | WCONTINUED;
                char cl[128];
                strcpy(cl, command);
                pid_t pid = fork();
                int status;
                if (arg != NULL && strcmp(arg, "&") == 0){//run the background task 
                    strcat(cl, " ");
                    strcat(cl, arg);
                    // print("background group id %d",getprp());
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
                        setpgid(pid,pid);
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
                        jobs[job_id_counter].pid = pid;
                        jobs[job_id_counter].job_id = job_id_counter;
                        jobs[job_id_counter].state = 0;
                        jobs[job_id_counter].command_line = cl;
                        job_id_counter++;
                        // Parent process
                        foreground_pid = pid;
                        waitpid(pid, &status, waitCondition);
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