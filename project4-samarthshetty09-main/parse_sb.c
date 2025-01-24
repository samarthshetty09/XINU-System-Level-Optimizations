#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"


void parse_superblock(const unsigned char* buffer, struct superblock* sb) {
    // The superblock starts at offset BOOT_BLOCK_SIZE
    const unsigned char* sb_ptr = buffer + BOOT_BLOCK_SIZE;
    sb->blocksize = *((int*)(sb_ptr));
    sb->inode_offset = *((int*)(sb_ptr + 4));
    sb->data_offset = *((int*)(sb_ptr + 8));
    sb->swap_offset = *((int*)(sb_ptr + 12));
    sb->free_inode = *((int*)(sb_ptr + 16));
    sb->free_block = *((int*)(sb_ptr + 20));
    print_superblock(sb);
}
void print_superblock(const struct superblock* sb) {
    printf("Superblock Information:\n");
    printf("=========================\n");
    printf("Block Size:       %d bytes\n", sb->blocksize);
    printf("Inode Offset:     %d blocks\n", sb->inode_offset);
    printf("Data Offset:      %d blocks\n", sb->data_offset);
    printf("Swap Offset:      %d blocks\n", sb->swap_offset);
    printf("Free Inode Head:  %d\n", sb->free_inode);
    printf("Free Block Head:  %d\n", sb->free_block);
    printf("=========================\n");
}

void view_superblock(const unsigned char* buffer, struct superblock* sb) {
    const unsigned char* sb_ptr = buffer + BOOT_BLOCK_SIZE;

    printf("Superblock raw bytes:\n");
    for (int i = 0; i < 24; i++) {
        printf("%02x ", sb_ptr[i]);
    }
    printf("\n");
}