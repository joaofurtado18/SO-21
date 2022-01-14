#include "fs/operations.h"
#include <assert.h>
#include <string.h>

typedef struct s_args {
    char name[30];
    int flags;
} args;

void *open(void *ARGS) {
    args *arg = (args *)ARGS;
    int f = tfs_open(arg->name, arg->flags);
    int *result = malloc(sizeof(int));
    *result = f;
    return (void *)result;
}

int main() {
    assert(tfs_init() != -1);

    args arg;
    pthread_t thread1;
    pthread_t thread2;

    strcpy(arg.name, "/f1");
    arg.flags = TFS_O_CREAT;

    if (pthread_create(&thread1, NULL, open, &arg) == 0)
        puts("Primeira tarefa criada");
    else {
        puts("Erro na criação da primeira tarefa");
        exit(1);
    }

    if (pthread_create(&thread2, NULL, open, &arg) == 0)
        puts("Segunda tarefa criada");
    else {
        puts("Erro na criação da segunda tarefa");
        exit(1);
    }

    int *ret_val1;
    int *ret_val2;
    pthread_join(thread1, (void **)&ret_val1);
    pthread_join(thread2, (void **)&ret_val2);

    assert(*ret_val1 != *ret_val2);

    free(ret_val1);
    free(ret_val2);

    printf("Successful test.\n");

    return 0;
}