
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "file_buffer.h"

#define DEFAULT_BUFFER_SIZE 16777216

/*
 *  file_buffer *new_file_buffer(FILE *f)
 *
 *  Allocate a new file_buffer.
 *  Returns NULL if the memory allocation fails.
 */

file_buffer *new_file_buffer(FILE *f)
{
    file_buffer *fb;

    fb = (file_buffer *) malloc(sizeof(file_buffer));
    if (fb != NULL) {
        fb->file = f;
        fb->bookmark = NULL;
        fb->line_number = 0;  // XXX Maybe more natural to start at 1?
        fb->current_pos = 0;
        fb->last_pos = 0;
        fb->reached_eof = 0;
        fb->buffer_size = DEFAULT_BUFFER_SIZE;
        fb->buffer = malloc(fb->buffer_size);
    }
    else {
        /*  XXX Temporary print statement. */
        printf("new_file_buffer: out of memory\n");
    }
    return fb;
}

void del_file_buffer(file_buffer *fb)
{
    free(fb->bookmark);
    free(fb->buffer);
    free(fb);
}

/*
 *  Print the file buffer fields.
 */

void fb_dump(file_buffer *fb)
{
    printf("fb->file        = %p\n", (void *) fb->file);
    printf("fb->line_number = %d\n", fb->line_number);
    printf("fb->current_pos = %lld\n", fb->current_pos);
    printf("fb->last_pos    = %lld\n", fb->last_pos);
    printf("fb->reached_eof = %d\n", fb->reached_eof);
}


void set_bookmark(file_buffer *fb)
{
    if (fb->bookmark) {
        free(fb->bookmark);
    }
    fb->bookmark = (fpos_t *) malloc(sizeof(fpos_t));
    fgetpos(fb->file, fb->bookmark);
}


void goto_bookmark(file_buffer *fb)
{
    if (fb->bookmark == NULL) {
        fprintf(stderr, "error: bookmark has not been set.\n");
    }
    else {
        fsetpos(fb->file, fb->bookmark);
    }
}


/*
 *  int _fb_load(file_buffer *fb)
 *
 *  Get data from the file into the buffer.
 *
 */

int _fb_load(file_buffer *fb)
{
    char *buffer = fb->buffer;

    if (!fb->reached_eof && (fb->current_pos == fb->last_pos || fb->current_pos+1 == fb->last_pos)) {
        size_t num_read;
        /* k will be either 0 or 1. */
        int k = fb->last_pos - fb->current_pos;
        if (k) {
            buffer[0] = buffer[fb->current_pos];
        }
        
        num_read = fread(&(buffer[k]), 1, fb->buffer_size - k, fb->file);

        fb->current_pos = 0;
        fb->last_pos = num_read + k;
        if (num_read < fb->buffer_size - k)
            if (feof(fb->file))
                fb->reached_eof = 1;
            else
                return FB_ERROR;
    }
    return 0;
}


/*
 *  int fetch(file_buffer *fb)
 *
 *  Get a single character from the buffer, and advance the buffer pointer.
 *
 *  Returns FB_EOF when the end of the file is reached.
 *  The sequence '\r\n' is treated as a single '\n'.  That is, when the next
 *  two bytes in the buffer are '\r\n', the buffer pointer is advanced by 2
 *  and '\n' is returned.
 *  When '\n' is returned, fb->line_number is incremented.
 */

int fetch(file_buffer *fb)
{
    char c;
    char *buffer = fb->buffer;
    
    _fb_load(fb);

    if (fb->current_pos == fb->last_pos)
        return FB_EOF;

    if (fb->current_pos + 1 < fb->last_pos && buffer[fb->current_pos] == '\r'
          && buffer[fb->current_pos + 1] == '\n') {
        c = '\n';
        fb->current_pos += 2;
    } else {
        c = buffer[fb->current_pos];
        fb->current_pos += 1;
    }
    if (c == '\n') {
        fb->line_number++;
    }
    return c;
}


/*
 *  int next(file_buffer *fb)
 *
 *  Returns the next byte in the buffer, but does not advance the pointer.
 */

int next(file_buffer *fb)
{
    _fb_load(fb);
    if (fb->current_pos+1 >= fb->last_pos)
        return FB_EOF;
    else
        return fb->buffer[fb->current_pos];
}


/*
 *  skipline(file_buffer *fb)
 *
 *  Read bytes from the buffer until a newline or the end of the file is reached.
 */

void skipline(file_buffer *fb)
{
    while (next(fb) != '\n' && next(fb) != FB_EOF) {
        fetch(fb);
    }
    if (next(fb) == '\n') {
        fetch(fb);
    }
}
