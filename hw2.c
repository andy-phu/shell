#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h> 


int main() {
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
        } else {
            printf("Error: Invalid command.\n");
        }
    }
    //test to see if i can make changes
    printf("Shell terminated.\n");
    return 0;
}