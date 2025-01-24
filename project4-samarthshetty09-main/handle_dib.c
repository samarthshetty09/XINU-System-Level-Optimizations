#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

void handle_double_indirect_blocks(const char *input_path, const char *output_path, int double_indirect_offset, SB *superblock, int *free_block_index) {
    FILE *input = fopen(input_path, "rb");
    FILE *output = fopen(output_path, "r+b");
    if (!input || !output) {
        perror("Error: Unable to open input or output file");
        if (input) fclose(input);
        if (output) fclose(output);
        return -1;
    }

    int block_size = superblock->blocksize;
    int pointers_per_block = block_size / sizeof(int);
    int *double_indirect_data = malloc(block_size);

    if (!double_indirect_data) {
        perror("Error: Memory allocation failed");
        fclose(input);
        fclose(output);
        return -1;
    }

    fseek(output, double_indirect_offset, SEEK_SET);
    fread(double_indirect_data, sizeof(int), pointers_per_block, output);

    for (int index = 0; index < pointers_per_block; index++) {
        if (double_indirect_data[index] > -1) {
            int source_offset = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + 
                                (superblock->data_offset * block_size) + 
                                (double_indirect_data[index] * block_size);

            fseek(input, source_offset, SEEK_SET);

            char block_content[block_size];
            fread(block_content, block_size, 1, input);

            int destination_offset = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + 
                                     (superblock->data_offset * block_size) + 
                                     ((*free_block_index) * block_size);

            fseek(output, destination_offset, SEEK_SET);
            fwrite(block_content, block_size, 1, output);

            double_indirect_data[index] = *free_block_index;
            (*free_block_index)++;

            fflush(output);

            // Process indirect blocks linked from double indirect block
            handle_indirect_blocks(input_path, output_path, destination_offset, superblock, free_block_index);
        }
    }

    fseek(output, double_indirect_offset, SEEK_SET);
    fwrite(double_indirect_data, sizeof(int), pointers_per_block, output);
    fflush(output);

    free(double_indirect_data);
    fclose(input);
    fclose(output);
}