#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

FILE* write_disk_image(const char* filename, unsigned char* buffer, size_t size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    size_t bytes_written = fwrite(buffer, 1, size, file);
    if (bytes_written != size) {
        perror("Failed to write the entire disk image");
        exit(EXIT_FAILURE);
    }

    return file;
    //fclose(file);
}

