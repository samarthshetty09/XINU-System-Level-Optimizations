CC = gcc
CFLAGS = -std=c11 -O0 -g -I.
DEPS = h/defrag.h
SRC = defrag.c read_disk_img.c write_disk_img.c parse_sb.c read_inodes.c handle_dib.c handle_tib.c handle_ib.c reinit_free_blocks.c generate_file_name.c
OBJ = $(SRC:.c=.o)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

defrag: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o defrag *disk_defrag*
