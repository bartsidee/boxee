//-----------------------------------------------------------------------------
// Copyright (c) 2008 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "jpeglib.h"

#include <stdint.h>
#include "image.h"

// flip image upside down after we read it
// Open GL draws from bottom to top
#define UPSIDE_DOWN


// extent jpeg_error_mgr structure to include setjmp info
typedef struct jpeg_error_mgr_jmp_s {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
} jpeg_error_mgr_jmp_st;


/**
 * jpeglib calls error_exit() on error
 * override with readjpeg_error_exit() so we can recover gracefully
 * this function just catches the error, prints an error message,
 * and returns control to the setjmp location
 * the argument to the function is actually a jpeg_error_mgr_jmp_st *
 */
void readjpeg_error_exit (j_common_ptr dinfo)
{
    jpeg_error_mgr_jmp_st * err = (jpeg_error_mgr_jmp_st *) dinfo->err;

    /* display error message */
    (*dinfo->err->output_message) (dinfo);

    /* return to setjmp */
    longjmp(err->setjmp_buffer, 1);
}

/**
 * Read the JPEG image file
 */
int read_JPEG_file (imagebuffer_st * image, char *filename, uint8_t alpha_value)
{
    struct jpeg_decompress_struct dinfo;
    jpeg_error_mgr_jmp_st read_jpeg_err;
    FILE * infile;
    JSAMPARRAY buffer;
    int row_stride;
    int color_depth;
    int bytes, rowbytes;

    switch (image->color_format) {
        case IMAGE_RGB:  color_depth = 3; break;
        case IMAGE_BGR:  color_depth = 3; break;
        case IMAGE_RGBA: color_depth = 4; break;
        case IMAGE_BGRA: color_depth = 4; break;
        case IMAGE_ARGB: color_depth = 4; break;
        case IMAGE_ABGR: color_depth = 4; break;
        default: return 0; /* unknown color format */
    }

    /* open input file */
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        return 0;
    }

    /* setup normal JPEG error handling */
    dinfo.err = jpeg_std_error(&read_jpeg_err.pub);
    /* call readpeg_error_exit() instead of default error_exit */
    read_jpeg_err.pub.error_exit = readjpeg_error_exit;
    if (setjmp(read_jpeg_err.setjmp_buffer)) {
        /* readjpeg_error_exit() will jump to this point */
        /* cleanup and return failure */
        jpeg_destroy_decompress(&dinfo);
        fclose(infile);
        return 0;
    }

    /* initialize jpeg decompression object */
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, infile);

    /* get image info */
    jpeg_read_header(&dinfo, TRUE);

    /* determine output height/width of jpeg file */
    jpeg_calc_output_dimensions(&dinfo);
    //if (flag.verbosity>1) print_jpeg_info(resource, &dinfo);

    /* fill in resource struct and allocate buffer space */
    image->width    = dinfo.output_width;
    image->height   = dinfo.output_height;
    rowbytes = image->width * color_depth;
    bytes    = image->width * image->height * color_depth;
    if ((image->buffer = (uint8_t *) malloc(bytes)) == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        jpeg_destroy_decompress(&dinfo);
        fclose(infile);
        return 0;
    }

    /* we want RGB output */
    dinfo.out_color_space = JCS_RGB;

    /* prepare to read one row at a time */
    row_stride = dinfo.output_width * dinfo.output_components;
    /* jpeglib will automatically deallocate buffer */
    buffer = (*dinfo.mem->alloc_sarray)
             ((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);

    /* decompress jpeg data from file */
    jpeg_start_decompress(&dinfo);
    while (dinfo.output_scanline < dinfo.output_height) {
        int i;
        uint8_t *p = NULL; /* ptr to start of line in buffer */

        if (image->lines_format == IMAGE_REVERSE) {
            /* store image lines bottom-to-top */
            int rownum = dinfo.output_scanline + 1;
            int offset = bytes - rowbytes * rownum;
            p = image->buffer + offset;
        }
        if (image->lines_format == IMAGE_NORMAL) {
            /* store image lines top-to-bottom */
            int offset = rowbytes * dinfo.output_scanline;
            p = image->buffer + offset;
        }

        jpeg_read_scanlines(&dinfo, buffer, 1);

        if (p != NULL)
        {
            if (image->color_format == IMAGE_RGB)
            {
                memcpy(p, buffer[0], row_stride);
            }
            else if (image->color_format == IMAGE_RGBA)
            {
                for (i = 0; i < row_stride; i += 3) {
                    *p++ = buffer[0][i+0]; // RED
                    *p++ = buffer[0][i+1]; // GREEN
                    *p++ = buffer[0][i+2]; // BLUE
                    *p++ = alpha_value;    // ALPHA
                }
            }
            else if (image->color_format == IMAGE_ARGB)
            {
                for (i = 0; i < row_stride; i += 3) {
                    *p++ = alpha_value;    // ALPHA
                    *p++ = buffer[0][i+0]; // RED
                    *p++ = buffer[0][i+1]; // GREEN
                    *p++ = buffer[0][i+2]; // BLUE
                }
            }
            else if (image->color_format == IMAGE_BGR)
            {
                for (i = 0; i < row_stride; i += 3) {
                    *p++ = buffer[0][i+2]; // BLUE
                    *p++ = buffer[0][i+1]; // GREEN
                    *p++ = buffer[0][i+0]; // RED
                }
            }
            else if (image->color_format == IMAGE_BGRA)
            {
                for (i = 0; i < row_stride; i += 3) {
                    *p++ = buffer[0][i+2]; // BLUE
                    *p++ = buffer[0][i+1]; // GREEN
                    *p++ = buffer[0][i+0]; // RED
                    *p++ = alpha_value;    // ALPHA
                }
            }
            else if (image->color_format == IMAGE_ABGR)
            {
                for (i = 0; i < row_stride; i += 3) {
                    *p++ = alpha_value;    // ALPHA
                    *p++ = buffer[0][i+2]; // BLUE
                    *p++ = buffer[0][i+1]; // GREEN
                    *p++ = buffer[0][i+0]; // RED
                 }
            }
            else
            {
                fprintf(stderr,"readjpeg: unknown lines_format: %d\n",
                    image->lines_format);
                break; /* cleanup and quit */
            }
        }
    }

    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);
    fclose(infile);

    return 1;
}


