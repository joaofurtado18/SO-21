#include "fs/operations.h"
#include <assert.h>
#include <string.h>

typedef struct s_args{
    int filehandle;
    char string[300];
    size_t strlen;
} args;

void *write(void* ARGS){
    args *arg = (args*) ARGS;
    printf("thread: fl: %d\nstr: %s\nlen: %ld\n", arg->filehandle, arg->string, arg->strlen);
    tfs_write(arg->filehandle, arg->string, arg->strlen);
    return NULL;
}
int main() {

    char *str = "6UPl5ldgtEBUGL2AAAAAAAAAHrjmDrpVwDcEb57s3hqCa4HW8zMPGSeZA4mjjqYTfU99hADCOJU7XhwgvkxPeEm4MoeomSUZTFjiuHYvKrtNVI5E6SACcx85IWbkqn118z2qtbeFHbx9f5YTHiq4uGNGj43Z33TP35ERTbNhu4kYMch9gRwmYUHjJ7FgPWwmPc3tA71hk5pCdULrzJcdlXitvreLNf2scKkHH5LwoAjpj0KNxVj4owJEWh0gklIPoRotyYOxBNz4weQMCaPYYr2qL8Lhrlo7v";
    char *path = "/f1";
    char buffer[300];
    args arg;

    assert(tfs_init() != -1);

    int f;
    ssize_t r;
    pthread_t thread, thread2;

    f = tfs_open(path, TFS_O_CREAT);
    arg.filehandle = f;
    puts("+++");
    strcpy(arg.string, str);
    arg.strlen = strlen(arg.string);
    pthread_create(&thread, NULL, write, &arg);
    int g = tfs_open(path, TFS_O_CREAT);
    arg.filehandle = g;
    puts("+++");
    strcpy(arg.string, "111111111111111111111111111111111111111111111111111111111111111111111111111");
    arg.strlen = strlen(arg.string);
    pthread_create(&thread2, NULL, write, &arg);
    /*assert(r == strlen(str));*/


    f = tfs_open(path, 0);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);

    buffer[r] = '\0';


    printf("Successful test.\n");

    tfs_copy_to_external_fs(path, "test1.txt");
    tfs_copy_to_external_fs("/f2", "test2.txt");


    return 0;
}
