
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void __print_memory_non_zero(char *p, int width, int pitch, int height)
{
    int i = 0, j = 0;
    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            if (p[i]) {
                printf("%02x ", p[i]);
            } else {
                printf("   ", p[i]);
            }
        }
        printf("\n");
        p += pitch;
    }
}

static char *load_bin_to_mem(char *filename)
{
    if (!filename) {
        printf("error: NULL params\n");
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("error: open(%s) fail\n", filename);
        return NULL;
    }

    fseek(file, 0x0, SEEK_END);
    long filesize = ftell(file);

    char *p = (char *)malloc(filesize);
    if (!p) {
        printf("error: not enough memory, request size %ld\n", filesize);
        return NULL;
    }

    fseek(file, 0x0, SEEK_SET);
    fread(p, 1, filesize, file);

    return p;
}

typedef struct {
    unsigned short offset;
    unsigned char width;
    unsigned char rev;
} s_font_map_index;

static s_font_map_index *read_font_index(char *p_font_index, char c)
{
    int index_offset = c - '0';
    if (index_offset < 12) {
        return (s_font_map_index *)(p_font_index + (index_offset << 2));
    }

    printf("error: out of boundary!\n");
    return (s_font_map_index *)(p_font_index);
}

int main(int argc, char **argv)
{
    char *p_font_index = load_bin_to_mem((char *)"font_index.bin");
    char *p_font_map = load_bin_to_mem((char *)"font_map.bin");

    int preset_pitch = 256;
    int preset_height = 16;

    if (!p_font_index || !p_font_map) {
        printf("no memory?\n");
        return (-1);
    }

    char *p_str = (char *)"0123456789:";
    int str_length = 11;

    if (argc > 1) {
        p_str = argv[1];
        str_length = strlen(p_str);
    }

    s_font_map_index *font_index;
    for (int i = 0; i < str_length; i ++) {
        s_font_map_index *font_index = read_font_index(p_font_index, p_str[i]);
        __print_memory_non_zero(p_font_map + font_index->offset, font_index->width, preset_pitch, preset_height);
    }

    free(p_font_index);
    free(p_font_map);

    return 0;
}

