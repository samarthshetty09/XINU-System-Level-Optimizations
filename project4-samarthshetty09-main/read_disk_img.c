#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"


unsigned char* read_disk_image(const char* filename, size_t* size, const char* file_output_path) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open disk image");
        exit(EXIT_FAILURE);
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    // Allocate buffer
    unsigned char* buffer = (unsigned char*)malloc(*size);
    if (!buffer) {
        perror("Failed to allocate memory for disk image");
        exit(EXIT_FAILURE);
    }

    // Read the file into the buffer
    size_t bytes_read = fread(buffer, 1, *size, file);
    if (bytes_read != *size) {
        perror("Failed to read the entire disk image");
        exit(EXIT_FAILURE);
    }
    
    fclose(file);
    return buffer;
}
