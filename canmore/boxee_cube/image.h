//-----------------------------------------------------------------------------
// Copyright (c) 2008 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//------------------------------------------------------------------------------

#ifndef __image_h__
#define __image_h__

#include <stdint.h>


/* lines format */
enum {
    IMAGE_NORMAL,  /* store top-to-bottom */
    IMAGE_REVERSE  /* store bottom-to-top */
};

/* color format */
enum {
    IMAGE_NIL = 0, /* unspecified, use default or guess */

    IMAGE_RGB,
    IMAGE_RGBA,
    IMAGE_ARGB,
    IMAGE_BGR,
    IMAGE_BGRA,
    IMAGE_ABGR
};

struct imagebuffer_s {
    int height;
    int width;
    int lines_format;
    int color_format;
    uint8_t * buffer;
};

typedef struct imagebuffer_s imagebuffer_st;

int read_JPEG_file(imagebuffer_st *image, char *filename, uint8_t alpha_value);

void write_PPM_file(imagebuffer_st *image, char *filename);

#endif

