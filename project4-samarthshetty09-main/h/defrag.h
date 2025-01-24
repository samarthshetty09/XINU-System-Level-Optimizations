#ifndef DEFRAG_H
#define DEFRAG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOT_BLOCK_SIZE 512
#define SUPERBLOCK_SIZE 512
#define MAX_BL_SIZE 1024
#define INODE_SIZE 100
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define OUTPUT_FILE_PATH ""
#define OUTPUT_FILE_NAME "disk_defrag"

struct superblock {
    int blocksize;      /* size of blocks in bytes */
    int inode_offset;   /* offset of inode region in blocks */
    int data_offset;    /* data region offset in blocks */
    int swap_offset;    /* swap region offset in blocks */
    int free_inode;     /* head of free inode list */
    int free_block;     /* head of free block list */
};
typedef struct superblock SB;

struct inode {
    int next_inode;             /* list for free inodes */
    int protect;                /* protection field */
    int nlink;                  /* number of links to this file */
    int size;                   /* number of bytes in file */
    int uid;                    /* owner's user ID */
    int gid;                    /* owner's group ID */
    int ctime;                  /* time field */
    int mtime;                  /* time field */
    int atime;                  /* time field */
    int dblocks[N_DBLOCKS];     /* pointers to data blocks */
    int iblocks[N_IBLOCKS];     /* pointers to indirect blocks */
    int i2block;                /* pointer to doubly indirect block */
    int i3block;                /* pointer to triply indirect block */
};
typedef struct inode IN;

typedef struct {
    int inode_index;
    struct inode* inode;
    // You can add additional fields as needed, such as pointers to indirect blocks or data
} FileInfo;

/* Function Prototypes */

/* Writes the updated disk image to a file */
FILE* write_disk_image(const char* filename, unsigned char* buffer, size_t size);
unsigned char* read_disk_image(const char* filename, size_t* size, const char* file_output_path);
void parse_superblock(const unsigned char* buffer, struct superblock* sb);
void read_used_inodes(const char *input_file, const char *output_file, SB *superblock, int* free_blocks);
void handle_double_indirect_blocks(const char *input_file, const char *output_file, int double_indirect_offset, SB *superblock, int* free_blocks);
void handle_triple_indirect_blocks(const char *input_file, const char *output_file, int triple_indirect_offset, SB *superblock, int* free_blocks);
void handle_indirect_blocks(const char *input_file, const char *output_file, int target_block_offset, SB *superblock, int* free_blocks);
int reinit_free_blocks(const char *output_file, SB *superblock, int* free_blocks);
void print_inodes(const struct inode* inodes, int inode_count);
FileInfo* collect_used_inodes(struct inode* inodes, int inode_count, int* file_count);
void copy_file_data(unsigned char* buffer, const struct superblock* sb, FileInfo* files, int file_count, int blocksize);
int read_int(const unsigned char* buffer);
void generate_output_filename(const char *input_path, char *output_filename, size_t max_length);

#endif /* DEFRAG_H */
