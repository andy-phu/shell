#include <stdio.h>
#include <unistd.h>


int main(){
    FILE *hello = fopen("hello.txt", "w");

    if (hello == NULL){
        perror("fopen");
    }

    for(int i = 0; i < 30; i++){
        fprintf(hello, "%d\n",i);
        sleep(1);
    }
    fclose(hello);
    return 0;
}