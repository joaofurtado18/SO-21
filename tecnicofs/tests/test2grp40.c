#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct s_args{
    int filehandle;
    char string[30];
    size_t len;
} args;

void *read(void* ARGS){
    args *arg = (args*) ARGS;
    return (void*) tfs_read(arg->filehandle, arg->string, arg->len);
}


int main(){
    char *path = "/f1";
    char* str = "Teste 2 Grupo 40 Dois Reads.";
    char buffer1[30];
    pthread_t thread1, thread2;
    void *read1, *read2;
    args arg1, arg2;
    assert(tfs_init() != -1);
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, str, strlen(str)) == strlen(str));
    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);
    int g = tfs_open(path, 0);
    assert(f != -1);

    arg1.filehandle = f;
    arg2.filehandle = g;
    arg1.len = sizeof(buffer1)-1;
    arg2.len = sizeof(buffer1)-1;

    pthread_create(&thread2, NULL, read, &arg2);
    pthread_create(&thread1, NULL, read, &arg1);

    pthread_join(thread2, &read2);
    pthread_join(thread1, &read1);

    assert((size_t) read1 == strlen(str) && (size_t)read2 == strlen(str));

    arg1.string[28] = '\0';
    assert(strcmp(arg1.string, str) == 0);


    arg2.string[28] = '\0';
    assert(strcmp(arg2.string, str) == 0);
    
    puts("Successful Test!");
    return 0;
}