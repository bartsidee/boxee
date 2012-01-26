/*
   BeatForce
   ringbuffer.c  - ring buffer's
   
   Copyright (c) 2001, Patrick Prasse (patrick.prasse@gmx.net)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public Licensse as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <stdio.h>
#include <errno.h>

#include "ringbuffer.h"

#ifdef _cplusplus
extern "C"
{
#endif

/* call before output thread is active !!!!! */
int rb_init (struct OutRingBuffer **rb, int size)
{
    struct OutRingBuffer *ring;

    if(rb==NULL || size <= 1024)
    {
        return 0;
    }
    
    ring = (struct OutRingBuffer* )malloc (sizeof (struct OutRingBuffer));
    if(ring == NULL)
    {
        fprintf(stderr, "Not enough memory");
        return 0;
    }
    memset (ring, 0, sizeof (struct OutRingBuffer));

    ring->size = 1;
    while(ring->size <= size)
        ring->size <<= 1;


    ring->rd_pointer = 0;
    ring->wr_pointer = 0;
    ring->buffer=(char* )malloc(sizeof(char)*(ring->size));
    
    *rb = ring;

    return 1;
}

void rb_destroy(struct OutRingBuffer *rb)
{
  if (rb && rb->buffer)
    free(rb->buffer);

  if (rb)
    free(rb);
}

int rb_write (struct OutRingBuffer *rb, unsigned char * buf, int len)
{
    int total;
    int i;

    /* total = len = min(space, len) */
    total = rb_free(rb);
    if(len > total)
        len = total;
    else
        total = len;

    i = rb->wr_pointer;
    if(i + len > rb->size)
    {
        memcpy(rb->buffer + i, buf, rb->size - i);
        buf += rb->size - i;
        len -= rb->size - i;
        i = 0;
    }
    memcpy(rb->buffer + i, buf, len);
    rb->wr_pointer = i + len;
    return total;

        
}

int rb_free (struct OutRingBuffer *rb)
{
    return (rb->size - 1 - rb_data_size(rb));
}

int rb_peek (OutRingBuffer* rb, unsigned char* buf, int max)
{
    int total;
    int i;
    total = rb_data_size(rb);

    if(max > total)
        max = total;
    else
        total = max;

    i = rb->rd_pointer;
    if(i + max > rb->size)
    {
        memcpy(buf, rb->buffer + i, rb->size - i);
        buf += rb->size - i;
        max -= rb->size - i;
        i = 0;
    }
    memcpy(buf, rb->buffer + i, max);
    //rb->rd_pointer = i + max;

    return total;
}

int rb_read (struct OutRingBuffer *rb, unsigned char * buf, int max)
{
    int total;
    int i;
    /* total = len = min(used, len) */
    total = rb_data_size(rb);

    if(max > total)
        max = total;
    else
        total = max;

    i = rb->rd_pointer;
    if(i + max > rb->size)
    {
        memcpy(buf, rb->buffer + i, rb->size - i);
        buf += rb->size - i;
        max -= rb->size - i;
        i = 0;
    }
    memcpy(buf, rb->buffer + i, max);
    rb->rd_pointer = i + max;

    return total;

}

int rb_data_size (struct OutRingBuffer *rb)
{
    return ((rb->wr_pointer - rb->rd_pointer) & (rb->size-1));
}


int rb_clear (struct OutRingBuffer *rb)
{
    memset(rb->buffer,0,rb->size);
    rb->rd_pointer=0;
    rb->wr_pointer=0;

    return 0;
}

#ifdef _cplusplus
}
#endif

