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
            if (current_write > BLOCK_SIZE)
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
    /*if (file->of_offset + to_read >= inode->i_size) {
        return -1;
    }*/

    int offset = 0;
    current_block = (int)file->of_offset / BLOCK_SIZE;
    current_read = (int)to_read;
    int j;
    while (current_read > 0) {
        void *block;
        if (current_block < 10)
            block = data_block_get(inode->i_data_block[current_block]);
        else {
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
        if (current_read > BLOCK_SIZE)
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

typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block[DATA_BLOCK_VECTOR];
    int i_reference_block;
    int allocated_blocks;
    /* in a real FS, more fields would exist here */
} inode_t;