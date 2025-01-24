[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/W9FAzzFI)
# Project 4: A Defragmenter

## 1. Introduction

The old Unix file system can get decreasing performance because of fragmentation. File system defragmenters improve the performance by laying out all blocks of a file in a sequential order on disk, and we are going to implement one in this assignment.

### Disk structure ###

You will be given a disk image containing a file system, and it will be correct, i.e., no corruption. Two major data structures that are used to manage this disk are related to the defragmenter: the *superblock* and the *inode*. The superblock structure is given as follows:
```c
  struct superblock {
    int blocksize; /* size of blocks in bytes */
    int inode_offset; /* offset of inode region in blocks */
    int data_offset; /* data region offset in blocks */
    int swap_offset; /* swap region offset in blocks */
    int free_inode; /* head of free inode list */
    int free_block; /* head of free block list */
  }
```
On disk, the first 512 bytes contains the boot block, and its actual format is not relevant to us. The second 512 bytes contains the superblock, the structure of which is defined above. All offsets in the superblock are given as blocks. Thus, if `inode_offset` is 1 and `blocksize` is 1 KB, the inode region starts at address `1024 B + 1*1 KB = 2 KB` into the disk. Each region fills up the disk up to the next region, and the swap region fills the disk to the end.

The inode region is effectively a large array of inodes, and the inode structure is given as follows:
```c
  #define N_DBLOCKS 10 
  #define N_IBLOCKS 4  
  
  struct inode {  
      int next_inode; /* list for free inodes */  
      int protect;        /*  protection field */ 
      int nlink;  /* Number of links to this file */ 
      int size;  /* Number of bytes in file */   
      int uid;   /* Owner's user ID */  
      int gid;   /* Owner's group ID */  
      int ctime;  /* Time field */  
      int mtime;  /* Time field */  
      int atime;  /* Time field */  
      int dblocks[N_DBLOCKS];   /* Pointers to data blocks */  
      int iblocks[N_IBLOCKS];   /* Pointers to indirect blocks */  
      int i2block;     /* Pointer to doubly indirect block */  
      int i3block;     /* Pointer to triply indirect block */  
   };
```
An unused inode has zero in the `nlink` field and the `next_inode` field contains the index of the next free inode, and the last free inode contains -1 for the index. For inodes in use, the `next_inode` field is not used. The head of the free inode list is a index into the inode region. The inodes in the inode region are all contiguous. Independent of block boundaries, the whole region is viewed as one big array. So it is possible for there to be an inode that overlaps two blocks.

The size field of the inode is used to determine which data block pointers are valid. If the file is small enough to be fit in `N_DBLOCKS` blocks, the indirect blocks are not used. Note that there may be more than one indirect block. However, there is only one pointer to a double indirect block and one pointer to a triple indirect block. All block pointers are indexes relative to the start of the data block region.

The free block list is maintained as a linked list. The first four bytes of a free block contain an integer index to the next free block; the last free block contains -1 for the index. The head of the free block list is also a index into the data block region.


## 2. Goal

You should read in the disk image, find inodes containing valid files, and write out a new image containing the same set of files, with the same inode numbers, but **with all the blocks in a file laid out contiguously**. Thus, if a file originally contains blocks `{6,2,15,22,84,7}` and it is relocated to the beginning of the data section, then the new blocks would be `{0,1,2,3,4,5}`. If there are indirect pointer blocks, the block containing the pointers need to be before the blocks being pointed to, and this applies to all level of indirect pointers.

After defragmenting, your new disk image should contain the same boot block (just copy it), a new superblock with the same list of free inodes but a new list of free blocks sorted from lowest to highest (to prevent future fragmentation), new inodes for valid files, and data blocks at their new locations.

You do not need to copy/save the contents of free blocks when you create the defragmented disk. They can be left as they are, initialized to zeros, or anything else as long as the free list itself is well constructed, i.e., it is a valid linked list.

Your program should take one parameter, i.e., the name of the disk image (e.g., `images_frag/disk_frag_1`). The output disk image should be named as "`disk_defrag`". 

You will need to do binary file I/O to read in the data files. You can do this with the `fread()` C library function. Here is some sample code:
```c
   FILE * f;
   unsigned char * buffer;
   size_t bytes;
   buffer = malloc(10*1024*1024);
   f = fopen("disk_frag","r");
   bytes =  fread(buffer,10*1024*1024,1,f);
```
Please do not use this directly, as it does no error checking. If you want to find out how big a file is from within your program, you can use the `fstat()` function. You can change the type of the buffer if you see a fit.

To help you understand how to do cast in C and read the disk image, we provide the following code to show how to read the superblock:
```c
   superblock *pSB = (superblock*) &(buffer[512]);
   ... pSB->blocksize ...;
   ... pSB->inode_offset ...;
```
You can also use the following code:
```c
   superblock sb;
   sb.blocksize = *((int *) &(buffer[512]));
   sb.inode_offset = *((int *) &(buffer[512 + 4]));
```
Try to make sense of the code above, from which you should get an idea on how to read the disk image. Note the code above assuming the buffer is of the `char *` or `unsigned char *` type. If you choose to declare your buffer to something else, e.g., `int *`, you need to adjust the code above.

To avoid potential compiler-induced problems in the code above, please add "`-O0`" into your compiler flags. Also please check (1) the address buffer returned by `malloc` can be divided by four and (2) `sizeof(struct superblock)` is 24 and `sizeof(struct inode)` is 100. If a violation is found, please let the instructor/TA know.

A different and safer approach of reading a word from an arbitrary address is as following:
```c
   superblock sb;
   int readIntAt(unsigned char *p)
   {
      return *(p+3) * 256 * 256 * 256 + *(p+2) * 256 * 256 + *(p+1) * 256 + *p;
   }
   sb.blocksize = readIntAt(buffer + 512);
   sb.inode_offset = readIntAt(buffer + 512 + 4);
```

## 3. Disk images 
You can find two directories: `images_frag` and `images_defrag`. The `images_frag` directory contains test input images: `disk_frag_1`, `disk_frag_2`, and `disk_frag_3`. The expected outputs are in the `images_defrag` directory. That is, for an input image `disk_frag_x`, your output should be identical to `disk_defrag_x`. You can use `diff` command. 

The input images follow the following rules:
  * All unused inodes have -1 for the last four index-related pointer fields and 0 for other fields after next_inode. For inodes in use, index-related pointer fields that are not used should have values -1.
  * A data block used as an indirect pointer block will have -1 for all the unused pointers beyond file size.
  * A free data block will contain only 0 after the first four bytes for pointing to the next free data block. A data block in use will contain only 0 beyond the file size.
  * Unused bytes in the super block and the inode region are 0. For the inode region, if the total size is 1024 bytes, then the last 24 bytes are unused bytes, as each inode is 100 bytes and the inode region can hold 10 inodes

## 4. Sample `defrag.c`
The code below is just for an example purpose. This code assumes that a separate set of files, including `disk_image.h` and `.c`, implements the actual functionalities and defines the data structures. You do not have to follow this structure. In fact, you can implement everything in a single c file. 
```c
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "disk_image.h"

#define OUTPUT_FILE_NAME "disk_defrag"

/* Disk image created from input file */
diskImage di;
/* Pointer to defragmented disk image */
diskImage *defragged = NULL;

int main( int argc, char *argv[ ] )
{
    //Open the input file
    FILE *in;
    char *fileName = argv[ 1 ];
    in = fopen( fileName, "r" );

    //Check for errors opening the input file
    if ( !in )
    {
        //handle error: could not open file        
    }

    //Load a disk image from the input file
    readDiskImage( in, &di );
    fclose( in );

    //Defrag the disk image
    defragged = defragDiskImage( &di );

    //Create an output file
    FILE *out = fopen( OUTPUT_FILE_NAME, "w+" );

    //Check for errors creating the output file
    if ( !out )
    {
        //handle error: could not create file        
    }

    //Write defragged to an output file
    writeDiskImage( out, defragged );
    fclose( out );

    //Cleanup
    freeDiskImage( &di );
    freeDiskImage( defragged );
    free( defragged );
}

```

## 5. Turn-in Instructions

Turn in 1) your source code, 2) Makefile, and 3) readme.txt with instructions for compile and execution.  

  * You can name your source files any names you want, but the executable generated by Makefile should be `defrag`.

  * Ensure that your program can be compiled by running `make` from the project root directory.

  * Make sure that all the necessary changes have been staged (by using `git status`).
  
  * Then, commit and push:
    ```shell
    git commit -am 'Done'
    git push
    ```
    You can do commit and push whenever you want, but the final submission must be committed/pushed with the message `Done`. For a final check, go to your repository using a web browser, and check if
    your changes have been pushed.



## 6. Q & A:

* Q. How to make a Makefile?

  A: A very simple example:
  ```
  defrag: defrag.c
        gcc -std=c11 -O0 -g -o defrag defrag.c
  ```

* Q: How to determine whether an inode is being used?

  A: If its `nlink` has 0; you can also check whether it is in the free list, but this will be more complicated than checking `nlink`; since inodes not in use also have the size field as 0, you can not use it to determine whether an inode is used.

* Q: Should I do the defragmentation within the original data block region?

  A: You can, but you do not have to. A simpler way is you read data blocks from the old disk image and then write the data blocks, including those storing pointers, onto the new disk image.

* Q: In which order should we traverse the inodes?

  A: Treat the whole inode region as an array, start from index 0, and skip the ones that are free.


## 7. Grading Policy

* 40 pts for compiling successfully via `make` and running as `./defrag in_image`.
* 60 (=20*3) pts for each disk image (in `images_frag`) -- the program needs to finish the defragmentation without any exceptions and its output should be the same as the provided image (in `images_defrag`).
