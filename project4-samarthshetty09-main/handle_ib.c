#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

void handle_indirect_blocks(const char *input_path, const char *output_path, int block_offset, SB *superblock, int *free_block_index) {
    FILE *input = fopen(input_path, "r+b");
    FILE *output = fopen(output_path, "r+b");
    if (!input || !output) {
        perror("Error: Unable to open input or output file");
        if (input) fclose(input);
        if (output) fclose(output);
        return -1;
    }

    int block_size = superblock->blocksize;
    int pointers_per_block = block_size / sizeof(int);

    fseek(output, block_offset, SEEK_SET);

    int *indirect_block_entries = malloc(block_size);
    if (!indirect_block_entries) {
        perror("Error: Memory allocation failed");
        fclose(input);
        fclose(output);
        return -1;
    }

    fread(indirect_block_entries, sizeof(int), pointers_per_block, output);

    for (int idx = 0; idx < pointers_per_block; idx++) {
        if (indirect_block_entries[idx] > -1) {
            int source_offset = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + 
                                (superblock->data_offset * block_size) + 
                                (indirect_block_entries[idx] * block_size);

            fseek(input, source_offset, SEEK_SET);

            void *data_buffer = malloc(block_size);
            if (!data_buffer) {
                perror("Error: Memory allocation failed");
                free(indirect_block_entries);
                fclose(input);
                fclose(output);
                return -1;
            }

            fread(data_buffer, block_size, 1, input);

            int destination_offset = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + 
                                     (superblock->data_offset * block_size) + 
                                     ((*free_block_index) * block_size);

            fseek(output, destination_offset, SEEK_SET);
            fwrite(data_buffer, block_size, 1, output);

            indirect_block_entries[idx] = *free_block_index;
            (*free_block_index)++;

            free(data_buffer);
        }
    }

    // Write updated indirect block back to the file
    fseek(output, block_offset, SEEK_SET);
    fwrite(indirect_block_entries, sizeof(int), pointers_per_block, output);
    fflush(output);

    free(indirect_block_entries);
    fclose(output);
    fclose(input);
}
