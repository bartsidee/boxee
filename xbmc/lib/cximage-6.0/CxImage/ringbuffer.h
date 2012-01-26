/*
   BeatForce
   ringbuffer.h  - ring buffer (header)
   
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

#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#define RINGBUFFER_MAGIC  0x72627546    /* 'rbuF' */


#define rb_magic_check(var,err)  {if(var->magic!=RINGBUFFER_MAGIC) return err;}

typedef struct OutRingBuffer
{
//    int *vrb_buf;
//    vrb_p vrb_buf;
    char *buffer;
    int wr_pointer;
    int rd_pointer;
    long magic;
    int size;
} OutRingBuffer;


#ifdef _cplusplus
extern "C"
{
#endif
/* ring buffer functions */
int rb_init (struct OutRingBuffer **, int);
int rb_write (struct OutRingBuffer *, unsigned char *, int);
int rb_free (struct OutRingBuffer *);
int rb_read (struct OutRingBuffer *, unsigned char *, int);
int rb_peek (OutRingBuffer *, unsigned char *, int);
int rb_data_size (struct OutRingBuffer *);
int rb_clear (struct OutRingBuffer *);
void rb_destroy(struct OutRingBuffer *);
#ifdef _cplusplus
}
#endif
#endif
