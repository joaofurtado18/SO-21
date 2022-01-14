#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct s_args{
    int filehandle;
    char string[300];
    size_t len;
} args;

void *write(void* ARGS){
    args *arg = (args*) ARGS;
    return (void*) tfs_write(arg->filehandle, arg->string, arg->len);
}


int main(){
    char *path = "/f1";
    char str1[5] = "1111";
    char str2[6] = "22222";
    char buffer[6];
    pthread_t thread1, thread2;
    void *write1, *write2;
    args arg1, arg2;
    assert(tfs_init() != -1);
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    arg1.filehandle = f;
    arg2.filehandle = f;
    arg1.len = strlen(str1);
    arg2.len = strlen(str2);
    strcpy(arg1.string, str1);
    strcpy(arg2.string, str2);

    pthread_create(&thread1, NULL, write, &arg1);
    pthread_create(&thread2, NULL, write, &arg2);

    pthread_join(thread1, &write1);
    pthread_join(thread2, &write2);

    assert((ssize_t) write1 == 4 && (ssize_t)write2 == 5);

    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);

    tfs_read(f, buffer, sizeof(buffer)-1);
    assert(strlen(buffer) == 5);
    assert(buffer[5] == '\0');

    char first_char = buffer[0]; 

    for(int i = 0; i< strlen(buffer)-1; i++){
        assert(buffer[i] == first_char);
    }
    assert(buffer[4] == '2');
    puts("Successful Test!");
    return 0;
}