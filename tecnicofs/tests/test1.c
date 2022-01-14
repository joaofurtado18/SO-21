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
    puts("testesgseges");

    int f;
    ssize_t r;
    pthread_t thread;
    puts("e");
    f = tfs_open(path, TFS_O_CREAT);
    puts("+++");
    arg.filehandle = f;
    strcpy(arg.string, str);
    arg.strlen = strlen(arg.string);
    pthread_create(&thread, NULL, write, &arg);

    f = tfs_close(f);

    tfs_open(path, 0);
    r = tfs_read(f, buffer, sizeof(buffer) - 1);

    buffer[r] = '\0';


    printf("Successful test.\n");

    tfs_copy_to_external_fs(path, "test1.txt");


    return 0;
}
