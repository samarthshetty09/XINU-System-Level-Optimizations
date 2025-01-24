#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

void copy_file_data(unsigned char* buffer, const struct superblock* sb, FileInfo* files, int file_count, int blocksize) {
    int data_region_start = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + (sb->data_offset * sb->blocksize);

    for (int i = 0; i < file_count; i++) {
        struct inode* inode = files[i].inode;
        int file_size = inode->size;
        int bytes_remaining = file_size;

        // Copy data from old blocks to new blocks
        int current_offset = 0;

        // Copy data for direct blocks
        for (int j = 0; j < N_DBLOCKS && bytes_remaining > 0; j++) {
            int old_block_index = inode->dblocks[j];
            int new_block_index = inode->dblocks[j];

            int old_block_offset = data_region_start + old_block_index * blocksize;
            int new_block_offset = data_region_start + new_block_index * blocksize;

            int bytes_to_copy = (bytes_remaining > blocksize) ? blocksize : bytes_remaining;
            memcpy(buffer + new_block_offset, buffer + old_block_offset, bytes_to_copy);
            bytes_remaining -= bytes_to_copy;
            current_offset += bytes_to_copy;
        }

        // Copy data for single indirect blocks
        for (int j = 0; j < N_IBLOCKS && bytes_remaining > 0; j++) {
            if (inode->iblocks[j] == -1) {
                break;
            }

            int old_indirect_block_index = inode->iblocks[j];
            int old_indirect_block_offset = data_region_start + old_indirect_block_index * blocksize;
            int pointers_per_block = blocksize / sizeof(int);

            // Read old indirect block pointers
            int* old_indirect_pointers = (int*)malloc(blocksize);
            for (int k = 0; k < pointers_per_block; k++) {
                old_indirect_pointers[k] = read_int(buffer + old_indirect_block_offset + k * 4);
            }

            int new_indirect_block_index = inode->iblocks[j];
            int new_indirect_block_offset = data_region_start + new_indirect_block_index * blocksize;

            // Read new indirect block pointers
            int* new_indirect_pointers = (int*)malloc(blocksize);
            for (int k = 0; k < pointers_per_block; k++) {
                new_indirect_pointers[k] = read_int(buffer + new_indirect_block_offset + k * 4);
            }

            for (int k = 0; k < pointers_per_block && bytes_remaining > 0; k++) {
                if (new_indirect_pointers[k] == -1) {
                    break;
                }

                int old_data_block_index = old_indirect_pointers[k];
                int new_data_block_index = new_indirect_pointers[k];

                int old_data_block_offset = data_region_start + old_data_block_index * blocksize;
                int new_data_block_offset = data_region_start + new_data_block_index * blocksize;

                int bytes_to_copy = (bytes_remaining > blocksize) ? blocksize : bytes_remaining;
                memcpy(buffer + new_data_block_offset, buffer + old_data_block_offset, bytes_to_copy);
                bytes_remaining -= bytes_to_copy;
                current_offset += bytes_to_copy;
            }

            free(old_indirect_pointers);
            free(new_indirect_pointers);
        }
    }
}

int read_int(const unsigned char* buffer) {
    return (buffer[0]) | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}
