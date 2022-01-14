#include "fs/operations.h"
#include <assert.h>
#include <string.h>

typedef struct s_args{
    int filehandle;
    char string[300];
    size_t len;
} args;

typedef struct openfile_args_s{
    char path[10];
    int flag;
} openfile_args;

void *write(void* ARGS){
    args *arg = (args*) ARGS;
    printf("thread: fl: %d\nstr: %s\nlen: %ld\n", arg->filehandle, arg->string, arg->len);
    return void* tfs_write(arg->filehandle, arg->string, arg->len);
}
void *read(void* ARGS){
    args *arg = (args*) ARGS;
    return (void*) tfs_read(arg->filehandle, arg->string, arg->len);
}

void *open(void* ARGS){
    openfile_args *arg = (openfile_args*) ARGS;
    return (void*) tfs_open(arg->path, arg->flag);
}
int main() {

    char *str = "6UPl5ldgtEBUGL2AAAAAAAAAHrjmDrpVwDcEb57s3hqCa4HW8zMPGSeZA4mjjqYTfU99hADCOJU7XhwgvkxPeEm4MoeomSUZTFjiuHYvKrtNVI5E6SACcx85IWbkqn118z2qtbeFHbx9f5YTHiq4uGNGj43Z33TP35ERTbNhu4kYMch9gRwmYUHjJ7FgPWwmPc3tA71hk5pCdULrzJcdlXitvreLNf2scKkHH5LwoAjpj0KNxVj4owJEWh0gklIPoRotyYOxBNz4weQMCaPYYr2qL8Lhrlo7v";
    char *path = "/f1";
    char buffer[300];
    args arg;
    assert(tfs_init() != -1);

    void* f;
    ssize_t r;
    pthread_t thread, thread2;
    openfile_args open_args;
    open_args->path = "/f1";
    open_args->flag = TFS_O_CREAT;
    pthread_create(&thread, NULL, write, &open_args);
    pthread_join(thread, &)
    f = (int)
    
    /*f = tfs_open(path, TFS_O_CREAT);
    arg.filehandle = f;
    strcpy(arg.string, str);
    arg.len = strlen(arg.string);
    pthread_create(&thread, NULL, write, &arg);
    pthread_create(&thread2, NULL, read, &arg);*/


    /*f = tfs_close(f);

    tfs_open(path, 0);
    r = tfs_read(f, buffer, sizeof(buffer) - 1);

    buffer[r] = '\0';*/


    printf("Successful test.\n");

    tfs_copy_to_external_fs(path, "test1.txt");


    return 0;
}
