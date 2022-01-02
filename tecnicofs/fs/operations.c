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
        /*delete reference block*/
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for (int i = 0; i < 10; i++) {
                    data_block_free(inode->i_data_block[i]);
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

int calculate_blocks(int to_write, int i_size) {
    int remaining_block_bytes;
    if (i_size % DATA_BLOCKS == 0) {
        if (to_write % DATA_BLOCKS == 0)
            return to_write / DATA_BLOCKS;
        else
            return to_write / DATA_BLOCKS + 1;
    } else {
        remaining_block_bytes = DATA_BLOCKS - (i_size % DATA_BLOCKS);
        if (to_write <= remaining_block_bytes)
            return 0;
        else {
            if ((to_write - remaining_block_bytes) % DATA_BLOCKS == 0)
                return (to_write - remaining_block_bytes) / DATA_BLOCKS;
            else
                return (to_write - remaining_block_bytes) / DATA_BLOCKS + 1;
        }
    }
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    int i, j;
    int current_write = (int)to_write;
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL)
        return -1;

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL)
        return -1;

    /* Determine how many bytes to write */
    /*if (to_write + file->of_offset > BLOCK_SIZE) {
        to_write = BLOCK_SIZE - file->of_offset;
    }*/

    /*blocks_to_alloc = calculate_blocks((int)to_write, (int)inode->i_size);
    if (blocks_to_alloc + inode->allocated_blocks >
        DATA_BLOCK_VECTOR + BLOCK_SIZE / sizeof(int)) {
        return -1;
    } */

    if (inode->i_size % BLOCK_SIZE == 0)
        i = inode->allocated_blocks;
    else
        i = inode->allocated_blocks - 1;

    int offset = 0;
    int *reference_block;
    if (current_write > 0) {
        for (; current_write > 0; current_write -= BLOCK_SIZE) {
            void *block;
            /*direct references*/
            if (i < 10) {
                if (file->of_offset % BLOCK_SIZE == 0) {
                    inode->i_data_block[i] = data_block_alloc();
                    inode->allocated_blocks++;
                }
                if (inode->i_data_block[i] == -1)
                    return -1;
                    
                block = data_block_get(inode->i_data_block[i]);
            }
            /*indirect references*/
            else {
                j = i - 10;
                /*allocate reference block if it's not allocated yet*/
                if (inode->i_reference_block == -1) {
                    inode->i_reference_block = data_block_alloc();
                    if (inode->i_reference_block == -1)
                        return -1;
                }
                /*get reference block*/
                reference_block = data_block_get(inode->i_reference_block);

                /*references to blocks in the fs_data vector*/
                if (file->of_offset % BLOCK_SIZE == 0) {
                    reference_block[j] = data_block_alloc();
                    inode->allocated_blocks++;
                }
                if (reference_block[j] == -1)
                    return -1;

                block = &reference_block[j];
            }

            if (block == NULL)
                return -1;

            /* Perform the actual write */

            /* The offset associated with the file handle is
             * incremented accordingly */
            size_t number_of_bytes;
            if (current_write > 1024)
                number_of_bytes = BLOCK_SIZE;
            else
                number_of_bytes = (size_t)current_write;

            memcpy(block + file->of_offset, buffer + offset, number_of_bytes);
            file->of_offset += number_of_bytes;
            offset += (int)number_of_bytes;

            if (file->of_offset > inode->i_size)
                inode->i_size = file->of_offset;
            i++;
        }
    }
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    int current_block, current_read;
    int *reference_block;
    /*size_t to_read;*/
    if (file == NULL)
        return -1;

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL)
        return -1;

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len)
        to_read = len;
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
        if (current_block < 10) {
            block = data_block_get(inode->i_data_block[current_block]);
        } else {
            j = current_block - 10;
            reference_block = data_block_get(inode->i_reference_block);
            block = &reference_block[j];
        }
        if (block == NULL)
            return -1;

        /* Perform the actual read */

        /* The offset associated with the file handle is
         * incremented accordingly */
        size_t number_of_bytes = 0;
        if (current_read > 1024)
            number_of_bytes = BLOCK_SIZE;
        else
            number_of_bytes = (size_t)current_read;

        memcpy(buffer + offset, block + file->of_offset, number_of_bytes);
        file->of_offset += number_of_bytes;
        offset += (int)number_of_bytes;

        current_read -= BLOCK_SIZE;
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
        offset += (int)result;
        if (buffer != NULL) {
            free(buffer);
        }

    } while (result >= BLOCK_SIZE);

    fclose(fp);
    tfs_close(fhandle);
    return 0;
}