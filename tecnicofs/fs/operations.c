#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for(int i = 0; i <= 10; i++){
                    if (i < 10)
                        data_block_free(inode->i_data_block[i]);
                    else{
                        for(int* last_block = data_block_get(inode->i_data_block[i]); last_block; last_block += DATA_BLOCKS){
                            data_block_free(*last_block);
                        }
                        break;
                    }
                }
                inode->i_size = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}

int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    int blocks_to_alloc, allocated_blocks, i, j, buffer_offset, file_offset;
    int *last_block;
    size_t current_write = to_write;
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to write */
    /*if (to_write + file->of_offset > BLOCK_SIZE) {
        to_write = BLOCK_SIZE - file->of_offset;
    }*/
    /*blocks_to_alloc = (int)to_write / DATA_BLOCKS + 1;
    if (blocks_to_alloc > DATA_BLOCK_VECTOR - 1 + BLOCK_SIZE / sizeof(int)) {
        return -1;
    }*/

    if (to_write > 0) {
        if (inode->i_size == 0) {
            /* If empty file, allocate new block */
            file_offset = 0;
            if (to_write % DATA_BLOCKS == 0)
                blocks_to_alloc = (int)to_write / DATA_BLOCKS;
            else
                blocks_to_alloc = (int)to_write / DATA_BLOCKS + 1;
        }
        
        else{
            file_offset = inode->i_size % DATA_BLOCKS;
            int remaining_space = DATA_BLOCKS-file_offset;

            if(file_offset == 0){
                if (to_write % DATA_BLOCKS == 0)
                    blocks_to_alloc = (int)to_write / DATA_BLOCKS;
                else
                    blocks_to_alloc = (int)to_write / DATA_BLOCKS + 1;
            }
            else if(remaining_space >= to_write){
                blocks_to_alloc = 0;
            }

            else{
                if (to_write % DATA_BLOCKS == 0)
                    blocks_to_alloc = ((int)to_write - remaining_space) / DATA_BLOCKS;
                else
                    blocks_to_alloc = ((int)to_write - remaining_space) / DATA_BLOCKS + 1;
            }
        }


        /*memory allocation*/
        allocated_blocks = inode->i_allocated_blocks;
        int count = 0;
        for (i = allocated_blocks; i < blocks_to_alloc + allocated_blocks; i++) {
            j = i - 9;
            last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR-1]);
            printf("i: %d\n", i);
            if (i < 10) {
                inode->i_data_block[i] = data_block_alloc();
                inode->i_allocated_blocks++;
                count++;
            }
            else if (i >= 10 && !last_block){
                inode->i_data_block[i] = data_block_alloc();
                puts("i > 10 e last block nao existe");
                last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR-1]);
                if (!last_block) {
                    return -1;
                }
                last_block[j] = data_block_alloc();
                inode->i_allocated_blocks++;
            }
            else if (i >= 10 && last_block){
                puts("last block existe");
                last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR-1]);
                last_block[j] = data_block_alloc();
                inode->i_allocated_blocks++;
            }

            
        }
    } else{
        puts("return -1");
        return -1;
    }



    /*perform write*/

    buffer_offset = 0;
    if (file_offset == 0){
        i = allocated_blocks;
    }

    else{
        i = allocated_blocks-1;
    }

    j = 0;
    for (; i < (allocated_blocks + blocks_to_alloc); i++) {
        printf("iteracao %d\n", i);

        printf("current write: %ld\n", current_write);
        if (current_write <= 0 || (i < 10 && !inode->i_data_block[i]))
            break;
        void *block;
        if (i < 10)
            block = data_block_get(inode->i_data_block[i]);
        else {
            last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR - 1]);
            block = &last_block[j];
            j++;
        }
        
        if (block == NULL) {
            return -1;
        }

        /* Perform the actual write */

        /* The offset associated with the file handle is
         * incremented accordingly */
        size_t number_of_bytes;
        if (current_write > 1024)
            number_of_bytes = BLOCK_SIZE;
        else
            number_of_bytes = current_write;
        
        memcpy(block + file->of_offset, buffer + buffer_offset, number_of_bytes);
        file->of_offset += number_of_bytes;
        buffer_offset += (int)number_of_bytes;

        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        /*printf("isize %ld\n",inode->i_size);*/
        
        current_write -= DATA_BLOCKS;
        /*printf("END\n\n");*/
    }


    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    int current_block, current_read;
    int *last_block;

    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    /*verificação do offset*/
    /*if (file->of_offset + to_read >= BLOCK_SIZE) {
        return -1;
    }*/

    current_block = (int)file->of_offset / BLOCK_SIZE;
    current_read = (int)to_read;
    int offset = (int)file->of_offset - BLOCK_SIZE * current_block;
    int j;

    while (current_read > 0) {
        void *block;
        j = current_block - 10;

        if (current_block < 10)
            block = data_block_get(inode->i_data_block[current_block]);
        else {
            last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR - 1]);
            block = &last_block[j];
        }

        if (block == NULL) {
            return -1;
        }

        /* Perform the actual read */

        /* The offset associated with the file handle is
         * incremented accordingly */
        size_t number_of_bytes;
        if (current_read > 1024)
            number_of_bytes = BLOCK_SIZE;
        else
            number_of_bytes = (size_t)current_read;
        
        memcpy(buffer + offset, block + file->of_offset, number_of_bytes);
        file->of_offset += number_of_bytes;
        offset += (int)number_of_bytes;

        current_read -= DATA_BLOCKS;
        current_block++;
    }

    return (ssize_t)to_read;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) {
    FILE *fp;
    ssize_t result;
    char *buffer;
    fp = fopen(dest_path, "w");

    if (!fp)
        return -1;

    int fhandle = tfs_open(source_path, 0);

    if (fhandle == -1)
        return -1;

    do {
        buffer = malloc(sizeof(char) * DATA_BLOCKS);
        result = tfs_read(fhandle, buffer, DATA_BLOCKS);

        if (result == -1)
            return -1;

        fwrite(buffer, 1, (size_t)result, fp);
        free(buffer);

    } while (result >= BLOCK_SIZE);

    fclose(fp);
    tfs_close(fhandle);
    return 0;
}
