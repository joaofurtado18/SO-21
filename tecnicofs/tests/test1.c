#include "fs/operations.h"
#include <assert.h>
#include <string.h>

int main() {

    char *str = "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";
    char *path = "/f1";
    char buffer[4100];

    assert(tfs_init() != -1);

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));
    printf("buffer: %s\nstring: %s\n", buffer, str);
    
    printf("r: %ld\n", r);
    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);
	
	tfs_copy_to_external_fs(path,"test1.txt");

    printf("Successful test.\n");

    return 0;
}
