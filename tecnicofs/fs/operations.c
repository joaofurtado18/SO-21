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
        /*TODO*/
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                if (data_block_free(inode->i_data_block[0]) == -1) {
                    return -1;
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
    int blocks_to_alloc, i, j;
    size_t current_write = to_write;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    int* last_block;

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

    blocks_to_alloc = (int)to_write / DATA_BLOCKS + 1;
    if (blocks_to_alloc > DATA_BLOCK_VECTOR-1 + BLOCK_SIZE/sizeof(int)){
        return -1;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            /* If empty file, allocate new block */
            for (i = 0; i < blocks_to_alloc; i++) {
                if (i < 10){
                    inode->i_data_block[i] = data_block_alloc();
                }
                else {
                    inode->i_data_block[i] = data_block_alloc();
                    last_block = data_block_get(inode->i_data_block[i]);
                    if(!last_block){
                        return -1;
                    }
                    for(j = 1; j < blocks_to_alloc - 10; j++){
                        last_block[j] = data_block_alloc();
                    }
                    break;
                }
            }
        }
    }

    int offset = 0;
    for (i = 0; i < blocks_to_alloc; i++) {
        j = i - 10;

        if (current_write <= 0 || (i < 10 && !inode->i_data_block[i]))
            break;

        void* block;
        if (i < 10)
            block = data_block_get(inode->i_data_block[i]);
        else{
            last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR-1]);
            block = &last_block[j];

        }
        if (block == NULL) {
            return -1;
        }

        /* Perform the actual write */

        /* The offset associated with the file handle is
         * incremented accordingly */
        if (current_write > 1024)
            {
                memcpy(block + file->of_offset, buffer + offset, BLOCK_SIZE);
                file->of_offset += BLOCK_SIZE;
                offset += BLOCK_SIZE;
            }
        else
            {
                memcpy(block + file->of_offset, buffer + offset, current_write);
                file->of_offset += current_write;
                offset += (int) current_write;
            }

        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        current_write -= DATA_BLOCKS;
    }

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    int current_block, current_read;
    int* last_block;
    /*size_t to_read;*/
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

    /*int blocks_to_read = (int) to_read/DATA_BLOCKS + 1;*/
    current_block = (int) file->of_offset/BLOCK_SIZE;
    current_read = (int)to_read;
    int offset = (int) file->of_offset - BLOCK_SIZE*current_block;
    int j;
    while (current_read > 0) {
        void* block;
        j = current_block - 10;
        if (current_block < 10)
            block = data_block_get(inode->i_data_block[current_block]);
        else{
            last_block = data_block_get(inode->i_data_block[DATA_BLOCK_VECTOR-1]);
            block = &last_block[j];
        }
        if (block == NULL) {
            return -1;
        }

        /* Perform the actual read */

        /* The offset associated with the file handle is
         * incremented accordingly */
        if (current_read > 1024){
            memcpy(buffer + offset, block + file->of_offset, BLOCK_SIZE);
            file->of_offset += BLOCK_SIZE;
            offset += BLOCK_SIZE;
        }
        else{
            memcpy(buffer + offset, block + file->of_offset, (size_t)current_read);
            file->of_offset += (size_t)current_read;
            offset += current_read;
        }

        current_read -= DATA_BLOCKS;
        current_block++;
    }

    return (ssize_t)to_read;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) {
    FILE *fp;
    ssize_t result;
    char *buffer;
    int offset = 0;
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
        offset += (int) result;
        free(buffer);

    } while (result >= BLOCK_SIZE);

    fclose(fp);
    tfs_close(fhandle);
    return 0;
}
