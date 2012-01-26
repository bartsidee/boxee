//-----------------------------------------------------------------------------
// Copyright (c) 2006-2010 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//
// Decription:
//     This sample demonstrates use of GDL_PD_RECV_HDMI_EDID_BLOCK command
//     availalbe via gdl_port_recv interface.
//-----------------------------------------------------------------------------

#include "libgdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRY(rc, label, foo, args)                 \
        if ( (rc = foo args) != GDL_SUCCESS )     \
        {                                         \
            printf("Failed in %s: %s\n", #foo, gdl_get_error_string(rc)); \
            goto label;                           \
        }


int parse_edid( unsigned char* edid );

bool read_edid(unsigned char** edid_data, int* len)
{
  gdl_ret_t rc = GDL_SUCCESS;
  unsigned int n;
  gdl_hdmi_edid_block_t eb;
  eb.index = 0;

  *edid_data = NULL;
  *len = 0;

  // Read first block
  if (gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_EDID_BLOCK, (void *) &eb,
      sizeof(gdl_hdmi_edid_block_t)) != GDL_SUCCESS)
  {
    printf("EDID can not be read\n");
    return false;
  }

  // Determine total number of blocks
  n = eb.data[126] + 1;

  *edid_data = (unsigned char*) malloc(n * 128);

  // Read and print all EDID blocks
  for (eb.index = 0; eb.index < n; eb.index++)
  {
    rc = gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_EDID_BLOCK,
        (void *) &eb, sizeof(gdl_hdmi_edid_block_t));

    if (rc == GDL_SUCCESS)
    {
      memcpy(*edid_data + *len, eb.data, 128);
      *len += 128;
    }

    if (rc != GDL_SUCCESS)
      break;
  }

//  printf("\nTotal of %d EDID blocks were read successfully\n", eb.index);

  return true;
}

//------------------------------------------------------------------------------
// main()
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  gdl_ret_t rc = GDL_SUCCESS;
  unsigned int j, n;

  // Validation successful, time to init GDL
  TRY(rc, exit, gdl_init, (0));

  unsigned char* edid_buffer;
  int edid_len;

  if (read_edid(&edid_buffer, &edid_len))
  {
#if 0
    for (j = 0; j < edid_len; j++)
    {
      printf("%s", (j % 0x10) ? "" : "\n");
      printf("%02X ", edid_buffer[j]);
    }
    printf("\n");
#endif

    if (parse_edid(edid_buffer) != 0)
    {
      TRY(rc, exit, gdl_close, ());
      exit(1);
    }
  }
  else
  {
    TRY(rc, exit, gdl_close, ());
    exit(1);
  }

exit:
  TRY(rc, exit, gdl_close, ());
}
