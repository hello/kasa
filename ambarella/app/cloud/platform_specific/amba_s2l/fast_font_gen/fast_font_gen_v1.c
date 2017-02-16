
#include <stdlib.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

static int font_size_width = 20, font_size_height = 20;
static int fixed_size_width = 14, fixed_size_height = 16;
static int fixed_pitch = 256;
static unsigned char color_y = 128, color_cb = 128, color_cr = 210;
static char font_filename[512] = "gbsn00lp.ttf";

static void print_memory_non_zero(unsigned char *p, int width, int pitch, int height)
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

static void print_memory(unsigned char *p, int width, int pitch, int height)
{
    int i = 0, j = 0;
    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            printf("%02x ", p[i]);
        }
        printf("\n");
        p += pitch;
    }
}

static void print_memory_non_zero_skip_odd(unsigned char *p, int width, int pitch, int height)
{
    int i = 0, j = 0;
    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            if (p[2 * i]) {
                printf("%02x ", p[2 * i]);
            } else {
                printf("   ", p[2 * i]);
            }
        }
        printf("\n");
        p += pitch;
    }
}

static void print_memory_non_zero_skip_even(unsigned char *p, int width, int pitch, int height)
{
    int i = 0, j = 0;
    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            if (p[2 * i + 1]) {
                printf("%02x ", p[2 * i + 1]);
            } else {
                printf("   ", p[2 * i + 1]);
            }
        }
        printf("\n");
        p += pitch;
    }
}

static void copy_data_central(unsigned char *p_buffer, unsigned char *p_buffer_1, int width, int height, int buffer_width, int buffer_height)
{
    int i = 0;
    unsigned char *p_src = NULL;
    unsigned char *p_dst = NULL;
    p_dst = p_buffer_1 + ((buffer_height - height) / 2) * (buffer_width) + (buffer_width - width) / 2;
    p_src = p_buffer;

    for (i = 0; i < height; i ++) {
        memcpy(p_dst, p_src, width);
        p_src += width;
        p_dst += buffer_width;
    }
}

static void copy_rect_data(unsigned char *p_src, unsigned char *p_dst, int width, int src_pitch, int dst_pitch, int height)
{
    int i = 0;

    for (i = 0; i < height; i ++) {
        memcpy(p_dst, p_src, width);
        p_src += src_pitch;
        p_dst += dst_pitch;
    }
}

static int load_ascii_char(FT_Face face, char c, unsigned char *p, int &width, int &height)
{
    int err = 0;

    int index = FT_Get_Char_Index(face, c);

    err = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
    if (err) {
        printf("error: FT_Load_Glyph() fail\n");
        return (-1);
    }

    FT_GlyphSlot slot = face->glyph;

    if (FT_GLYPH_FORMAT_BITMAP != face->glyph->format) {
        err = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (err) {
            printf("error: FT_Render_Glyph() fail\n");
            return (-2);
        }
    }

    width = slot->bitmap.width;
    height = slot->bitmap.rows;

    if (slot->bitmap.pitch == slot->bitmap.width) {
        memcpy(p, slot->bitmap.buffer, width * height);
    } else {
        int i = 0;
        unsigned char *p_src = slot->bitmap.buffer;
        for (; i < height; i ++) {
            memcpy(p, p_src, width);
            p += width;
            p_src += slot->bitmap.pitch;
        }
    }

    return 0;
}

static int load_font_file(FT_Library *library, FT_Face *face, char *font_file, int size_width, int size_height)
{
    int err = FT_Init_FreeType(library);
    if (err) {
        printf("error: FT_Init_FreeType fail, ret %d\n", err);
        return (-1);
    }

    err = FT_New_Face(*library, font_file, 0, face);
    if (err) {
        printf("error: FT_New_Face fail, ret %d\n", err);
        return (-2);
    }

    err = FT_Set_Pixel_Sizes(*face, size_width, size_height);
    if (err) {
        printf("error: FT_Set_Pixel_Sizes fail, ret %d\n", err);
        return (-3);
    }

    return 0;
}

typedef struct {
    short offset;
    unsigned char width;
    unsigned char rev;
} s_font_index;

static int generate_binary_font_list_fontmap(FT_Face face, int buffer_width, int buffer_height, int pitch, char begin_char, char end_char, char *fontindex_bin_filename, char *fontmap_bin_filename)
{
    int ret = 0;

    if (!face || !fontmap_bin_filename || !fontindex_bin_filename) {
        printf("error: NULL params, face %p, bin_filename %p, fontindex_bin_filename %p\n", face, fontmap_bin_filename, fontindex_bin_filename);
        return (-1);
    }

    if ((buffer_width < 1) || (buffer_height < 1) || (pitch < 16) || (pitch < buffer_width)) {
        printf("error: buffer_width %d, buffer_height %d, pitch %d\n", buffer_width, buffer_height, pitch);
        return (-2);
    }

    FILE *file = fopen(fontmap_bin_filename, "wb");
    if (!file) {
        printf("error: open output file fail(%s)\n", fontmap_bin_filename);
        return (-3);
    }

    FILE *file_indexbin = fopen(fontindex_bin_filename, "wb");
    if (!file_indexbin) {
        printf("error: open output file fail(%s)\n", fontindex_bin_filename);
        return (-4);
    }

    unsigned char *p_buffer = (unsigned char *) malloc(buffer_width * buffer_height);
    unsigned char *p_buffer_1 = (unsigned char *) malloc(buffer_width * buffer_height);
    unsigned char *p_buffer_file = (unsigned char *) malloc(pitch * buffer_height);

    s_font_index font_index;

    if ((!p_buffer) || (!p_buffer_1) || (!p_buffer_file)) {
        printf("error: not enough memory\n");
        return (-5);
    }

    char t = begin_char;

    int width = 0, height = 0;

    unsigned char *p_buffer_file_t = p_buffer_file;
    font_index.offset = 0;
    font_index.width = 0;
    font_index.rev = 0;

    for (t = begin_char; t <= end_char; t ++, p_buffer_file_t += buffer_width) {

        memset(p_buffer_1, 0x0, buffer_width * buffer_height);
        ret = load_ascii_char(face, t, p_buffer, width, height);
        if (ret) {
            printf("error: load_ascii_char(%c) fail\n", t);
            return (-6);
        }

        font_index.width = buffer_width;
        printf("[print index]: font_index.offset %hd, width %d\n", font_index.offset, font_index.width);
        fwrite(&font_index, 1, sizeof(s_font_index), file_indexbin);
        font_index.offset += buffer_width;

        printf("\t\t[print char %c]: (width %d, height %d)\n", t, width, height);
        print_memory_non_zero(p_buffer, width, width, height);

        copy_data_central(p_buffer, p_buffer_1, width, height, buffer_width, buffer_height);
        copy_rect_data(p_buffer_1, p_buffer_file_t, buffer_width, buffer_width, pitch, buffer_height);
    }

    fwrite(p_buffer_file, 1, pitch * buffer_height, file);

    fclose(file);
    fclose(file_indexbin);

    return 0;
}

static int generate_simple_clut(char *clut_bin_filename, unsigned char y, unsigned char cb, unsigned char cr)
{
    FILE *file = fopen(clut_bin_filename, "wb");
    if (!file) {
        printf("error: open output file fail(%s)\n", clut_bin_filename);
        return (-3);
    }

    int t = 0;
    unsigned char c = 0;

    for (t = 0; t < 256; t ++) {
        c = (unsigned char) t;
        fwrite(&cr, 1, 1, file);
        fwrite(&cb, 1, 1, file);
        fwrite(&y, 1, 1, file);
        fwrite(&c, 1, 1, file);
    }

    fclose(file);

    return 0;
}

static int init_params(int argc, char **argv)
{
    int i = 0;
    int val = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp("--fontfile", argv[i])) {
            if ((i + 1) < argc) {
                int length = strlen(argv[i + 1]);
                if (length > 511) {
                    printf("'--fontfile, filename too long, %d'\n", length);
                    return (-1);
                }
                strcpy(font_filename, argv[i + 1]);
                font_filename[length] = 0x0;
                printf("'--fontfile: %s'\n", font_filename);
                i ++;
            } else {
                printf("'--fontfile, should followed by font filename'\n");
                return (-1);
            }
        } else if (!strcmp("--fontwidth", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                font_size_width = val;
                printf("'--fontwidth %d'\n", font_size_width);
                i ++;
            } else {
                printf("'--fontwidth, should followed by font width'\n");
                return (-1);
            }
        } else if (!strcmp("--fontheight", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                font_size_height = val;
                printf("'--fontheight %d'\n", font_size_height);
                i ++;
            } else {
                printf("'--fontheight, should followed by font height'\n");
                return (-2);
            }
        } else if (!strcmp("--fixedwidth", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                fixed_size_width = val;
                printf("'--fixedwidth %d'\n", fixed_size_width);
                i ++;
            } else {
                printf("'--fixedwidth, should followed by fixed width'\n");
                return (-3);
            }
        } else if (!strcmp("--fixedheight", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                fixed_size_height = val;
                printf("'--fixedheight %d'\n", fixed_size_height);
                i ++;
            } else {
                printf("'--fixedheight, should followed by fixed height'\n");
                return (-4);
            }
        }  else if (!strcmp("--fixedpitch", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                fixed_pitch = val;
                printf("'--fixed_pitch %d'\n", fixed_pitch);
                i ++;
            } else {
                printf("'--fixed_pitch, should followed by fixed pitch'\n");
                return (-4);
            }
        } else if (!strcmp("--coloryuv", argv[i])) {
            if ((i + 3) < argc) {
                if (1 == sscanf(argv[i + 1], "%d", &val)) {
                    color_y = val;
                } else {
                    printf("'--coloryuv, should followed by three interger: y cb cr'\n");
                    return (-9);
                }

                if (1 == sscanf(argv[i + 2], "%d", &val)) {
                    color_cb = val;
                } else {
                    printf("'--coloryuv, should followed by three interger: y cb cr'\n");
                    return (-10);
                }

                if (1 == sscanf(argv[i + 3], "%d", &val)) {
                    color_cr = val;
                } else {
                    printf("'--coloryuv, should followed by three interger: y cb cr'\n");
                    return (-11);
                }

                printf("'--coloryuv %d %d %d'\n", color_y, color_cb, color_cr);
                i += 3;
            } else {
                printf("'--colorrgb, should followed by three interger: y cb cr'\n");
                return (-12);
            }
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    FT_Library library = NULL;
    FT_Face face = NULL;

    int ret = load_font_file(&library, &face, font_filename, font_size_width, font_size_height);

    if (ret) {
        printf("load_font_file(%s) fail, ret %d\n", font_filename, ret);
        return ret;
    }

    if (fixed_size_width & 0x1) {
        fixed_size_width ++;
    }

    if (fixed_size_height & 0x1) {
        fixed_size_height ++;
    }

    ret = generate_binary_font_list_fontmap(face, fixed_size_width, fixed_size_height, fixed_pitch, '0', ':', (char *)"font_index.bin", (char *)"font_map.bin");
    if (ret) {
        printf("generate_binary_font_list fail, ret %d\n", ret);
        return ret;
    }

    ret = generate_simple_clut((char *)"clut.bin", color_y, color_cb, color_cr);
    if (ret) {
        printf("generate_binary_font_list fail, ret %d\n", ret);
        return ret;
    }

    return 0;
}



