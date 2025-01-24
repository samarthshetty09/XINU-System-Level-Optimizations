#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/defrag.h"

void generate_output_filename(const char *input_path, char *output_filename, size_t max_length) {
    // find the last occurrence of a slash to extract the filename
    const char *filename_start = strrchr(input_path, '/');
    filename_start = filename_start ? filename_start + 1 : input_path;

    // copy the filename into the output buffer
    strncpy(output_filename, filename_start, max_length - 1);
    output_filename[max_length - 1] = '\0'; // ensure null termination

    // search "frag" in the filename
    char *frag_location = strstr(output_filename, "frag");
    if (frag_location) {
        // Replace "frag" with "defrag" while preserving text after "frag"
        size_t offset_after_frag = frag_location - output_filename + strlen("frag");
        char after_frag[max_length];
        strncpy(after_frag, output_filename + offset_after_frag, max_length - offset_after_frag);
        after_frag[max_length - offset_after_frag - 1] = '\0';

        // replace "frag" with "defrag"
        strncpy(frag_location, "defrag", strlen("defrag"));
        frag_location[strlen("defrag")] = '\0';

        strncat(output_filename, after_frag, max_length - strlen(output_filename) - 1);
    }
}