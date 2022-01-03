#include "../fs/operations.h"
#include <assert.h>
#include <string.h>

#define COUNT 40
#define SIZE 256

/**
   This test fills in a new file up to 10 blocks via multiple writes, 
   each write always targeting only 1 block of the file, 
   then checks if the file contents are as expected
 */


int main() {

    char *path = "/f1";

    /* Writing this buffer multiple times to a file stored on 1KB blocks will 
       always hit a single block (since 1KB is a multiple of SIZE=256) */
    // char input[SIZE];
    // memset(input, 'A', SIZE);
    char* input = "TallR5FPGMGe74NGWjEiHepfiOZzUIsWvAIe74TSXUKHTLkpJUyIzJBvysiPdlT5b2YrtaXKKxgrarSBJ5IMBOIjWin2To2UO39KK8UFI1t5BWZZfVuRstr479NMxhUBPx8O3We16T5gCks5YMhKDHaOeiNpKD7SEasTxE9evG2NuG79tjEfS0kVY3f1ewWRprF4XB7qWcCoquZAwbAgc9yV0haYwUdzHiTlxtAWF6mx1wWVcVdJogpNbNorJDX";

    char output [SIZE];

    assert(tfs_init() != -1);

    /* Write input COUNT times into a new file */
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    for (int i = 0; i < COUNT; i++) {
        assert(tfs_write(fd, input, SIZE) == SIZE);
        //printf("i: %d\n", i);
    }
    assert(tfs_close(fd) != -1);

    /* Open again to check if contents are as expected */
    fd = tfs_open(path, 0);
    assert(fd != -1 );

    for (int i = 0; i < COUNT; i++) {
        //printf("i: %d\n",i);
        assert(tfs_read(fd, output, SIZE) == SIZE);
        // printf("output: %s\n",output);
        assert(memcmp(input, output, SIZE) == 0);
    }

    assert(tfs_close(fd) != -1);


    printf("Sucessful test\n");

    return 0;
}