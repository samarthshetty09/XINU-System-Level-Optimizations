#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk_image>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct superblock sb;
    const char *input_file_path = argv[1];
    int free_blocks = 0;
    char output_filename[1024];
    size_t disk_size;
    char final_output_path[1024];

    generate_output_filename(input_file_path, output_filename, sizeof(output_filename));
    snprintf(final_output_path, 1024, "%s", output_filename);


    unsigned char* buffer = read_disk_image(input_file_path, &disk_size, final_output_path);
    FILE* output_file = write_disk_image(final_output_path, buffer, disk_size);
    
    parse_superblock(buffer, &sb);

    free(buffer);

    read_used_inodes(input_file_path, final_output_path, &sb, &free_blocks);

    reinit_free_blocks(final_output_path, &sb, &free_blocks);

    sb.free_block = free_blocks;

    fseek(output_file, 512, SEEK_SET);
    fwrite(&sb, sizeof(struct superblock), 1, output_file);

    fclose(output_file);

    return 0;
}

