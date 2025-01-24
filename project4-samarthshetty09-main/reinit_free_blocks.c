#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"


int reinit_free_blocks(const char *output_path, SB *sb, int *first_free_block) {
    FILE *file = fopen(output_path, "r+b");
    if (!file) {
        perror("Failed to open the output file");
        return -1;
    }

    int block_size = sb->blocksize;
    char *zero_buffer = malloc(block_size);
    if (!zero_buffer) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }
    memset(zero_buffer, 0, block_size);

    printf("Starting free block index: %d\n", sb->free_block);
    printf("Total free blocks: %d\n", *first_free_block);

    int current_block = *first_free_block;

    while (current_block <= sb->free_block) {
        int block_position = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + sb->data_offset * block_size + (current_block * block_size);

        // Erase the block content by filling it with zeroes
        fseek(file, block_position, SEEK_SET);
        fwrite(zero_buffer, block_size, 1, file);
        fflush(file);

        // Update the free block list
        int next_block = (current_block + 1 <= sb->free_block) ? (current_block + 1) : -1;
        fseek(file, block_position, SEEK_SET);
        fwrite(&next_block, sizeof(int), 1, file);
        fflush(file);

        // Move to the next block in the list
        current_block++;
    }

    free(zero_buffer);
    fclose(file);

    return 0;
}
