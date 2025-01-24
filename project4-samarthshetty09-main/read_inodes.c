#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

void read_used_inodes(const char *input_file, const char *output_file, SB *superblock, int* free_blocks) {
    FILE *input = fopen(input_file, "rb");
    FILE *output = fopen(output_file, "r+b");
    if (!input || !output) {
        perror("Failed to open input or output file");
        if (input) fclose(input);
        if (output) fclose(output);
        exit(EXIT_FAILURE);
    }

    int inode_start = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + (superblock->inode_offset * superblock->blocksize);
    int data_start = BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + (superblock->data_offset * superblock->blocksize);
    int total_inodes = (data_start - inode_start) / INODE_SIZE;

    printf("Total Inodes: %d...\n", total_inodes);

    for (int i = 0; i < total_inodes; i++) {
        fseek(input, inode_start + (i * INODE_SIZE), SEEK_SET);
        IN inode;
        fread(&inode, INODE_SIZE, 1, input);

        if (inode.nlink > 0) {
            // Handle direct blocks
            for (int j = 0; j < N_DBLOCKS; ++j) {
                if (inode.dblocks[j] > -1) {
                    int src_offset = data_start + (inode.dblocks[j] * superblock->blocksize);
                    fseek(input, src_offset, SEEK_SET);

                    char block_content[superblock->blocksize];
                    fread(block_content, superblock->blocksize, 1, input);

                    int dest_offset = data_start + ((*free_blocks) * superblock->blocksize);
                    fseek(output, dest_offset, SEEK_SET);
                    fwrite(block_content, superblock->blocksize, 1, output);

                    inode.dblocks[j] = (*free_blocks);
                    (*free_blocks)++;
                }
            }

            // Handle single indirect blocks
            for (int j = 0; j < N_IBLOCKS; ++j) {
                if (inode.iblocks[j] > -1) {
                    int src_offset = data_start + (inode.iblocks[j] * superblock->blocksize);
                    fseek(input, src_offset, SEEK_SET);

                    char block_content[superblock->blocksize];
                    fread(block_content, superblock->blocksize, 1, input);

                    int dest_offset = data_start + ((*free_blocks) * superblock->blocksize);
                    fseek(output, dest_offset, SEEK_SET);
                    fwrite(block_content, superblock->blocksize, 1, output);

                    inode.iblocks[j] = (*free_blocks);
                    (*free_blocks)++;

                    fflush(output);
                    handle_indirect_blocks(input_file, output_file, dest_offset, superblock, free_blocks);
                }
            }

            // Handle double indirect blocks
            if (inode.i2block > -1) {
                int src_offset = data_start + (inode.i2block * superblock->blocksize);
                fseek(input, src_offset, SEEK_SET);

                char block_content[superblock->blocksize];
                fread(block_content, superblock->blocksize, 1, input);

                int dest_offset = data_start + ((*free_blocks) * superblock->blocksize);
                fseek(output, dest_offset, SEEK_SET);
                fwrite(block_content, superblock->blocksize, 1, output);

                inode.i2block = (*free_blocks);
                (*free_blocks)++;

                fflush(output);
                handle_double_indirect_blocks(input_file, output_file, dest_offset, superblock, free_blocks);
            }

            // Handle triple indirect blocks
            if (inode.i3block > -1) {
                int src_offset = data_start + (inode.i3block * superblock->blocksize);
                fseek(input, src_offset, SEEK_SET);

                char block_content[superblock->blocksize];
                fread(block_content, superblock->blocksize, 1, input);

                int dest_offset = data_start + ((*free_blocks) * superblock->blocksize);
                fseek(output, dest_offset, SEEK_SET);
                fwrite(block_content, superblock->blocksize, 1, output);

                inode.i3block = (*free_blocks);
                (*free_blocks)++;

                fflush(output);
                handle_triple_indirect_blocks(input_file, output_file, dest_offset, superblock, free_blocks);
            }

            // Write updated inode back to the output file
            fseek(output, inode_start + (i * INODE_SIZE), SEEK_SET);
            fwrite(&inode, INODE_SIZE, 1, output);
        }
        //print_inode(&inode, i);
    }
    fclose(input);
    fclose(output);
}

// helper funcs to vizualize inodes
void print_all_inodes(const IN* inodes, int inode_count) {
    for (int i = 0; i < inode_count; i++) {
        print_inode(&inodes[i], i);
    }
}

void print_inode(const IN* inode, int index) {
    printf("Inode %d:\n", index);
    printf("====================\n");
    printf("Next Inode:        %d\n", inode->next_inode);
    printf("Protection:        %d\n", inode->protect);
    printf("Link Count:        %d\n", inode->nlink);
    printf("Size:              %d bytes\n", inode->size);
    printf("UID:               %d\n", inode->uid);
    printf("GID:               %d\n", inode->gid);
    printf("Creation Time:     %d\n", inode->ctime);
    printf("Modification Time: %d\n", inode->mtime);
    printf("Access Time:       %d\n", inode->atime);

    // Print direct blocks
    printf("Direct Blocks:     ");
    for (int i = 0; i < N_DBLOCKS; i++) {
        printf("%d ", inode->dblocks[i]);
    }
    printf("\n");

    // Print indirect blocks
    printf("Indirect Blocks:   ");
    for (int i = 0; i < N_IBLOCKS; i++) {
        printf("%d ", inode->iblocks[i]);
    }
    printf("\n");

    // Print double and triple indirect blocks
    printf("Double Indirect:   %d\n", inode->i2block);
    printf("Triple Indirect:   %d\n", inode->i3block);
    printf("====================\n\n");
}