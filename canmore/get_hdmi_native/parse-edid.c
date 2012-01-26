#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: rewrite
// FIXME: cleanup 'static' variables

typedef unsigned char byte;
/* byte must be 8 bits */

/* int must be at least 16 bits */

/* long must be at least 32 bits */



#define DIE_MSG( x ) \
        { MSG( x ); exit( 1 ); }


#define UPPER_NIBBLE( x ) \
        (((128|64|32|16) & (x)) >> 4)

#define LOWER_NIBBLE( x ) \
        ((1|2|4|8) & (x))

#define COMBINE_HI_8LO( hi, lo ) \
        ( (((unsigned)hi) << 8) | (unsigned)lo )

#define COMBINE_HI_4LO( hi, lo ) \
        ( (((unsigned)hi) << 4) | (unsigned)lo )

const byte edid_v1_header[] = { 0x00, 0xff, 0xff, 0xff,
                                0xff, 0xff, 0xff, 0x00 };

const byte edid_v1_descriptor_flag[] = { 0x00, 0x00 };


#define EDID_LENGTH                             0x80

#define EDID_HEADER                             0x00
#define EDID_HEADER_END                         0x07

#define ID_MANUFACTURER_NAME                    0x08
#define ID_MANUFACTURER_NAME_END                0x09
#define ID_MODEL				0x0a

#define ID_SERIAL_NUMBER			0x0c

#define MANUFACTURE_WEEK			0x10
#define MANUFACTURE_YEAR			0x11

#define EDID_STRUCT_VERSION                     0x12
#define EDID_STRUCT_REVISION                    0x13

#define DPMS_FLAGS				0x18
#define ESTABLISHED_TIMING_1                    0x23
#define ESTABLISHED_TIMING_2                    0x24
#define MANUFACTURERS_TIMINGS                   0x25

#define DETAILED_TIMING_DESCRIPTIONS_START      0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE        18
#define NO_DETAILED_TIMING_DESCRIPTIONS         4



#define DETAILED_TIMING_DESCRIPTION_1           0x36
#define DETAILED_TIMING_DESCRIPTION_2           0x48
#define DETAILED_TIMING_DESCRIPTION_3           0x5a
#define DETAILED_TIMING_DESCRIPTION_4           0x6c



#define PIXEL_CLOCK_LO     (unsigned)dtd[ 0 ]
#define PIXEL_CLOCK_HI     (unsigned)dtd[ 1 ]
#define PIXEL_CLOCK        (COMBINE_HI_8LO( PIXEL_CLOCK_HI,PIXEL_CLOCK_LO )*10000)

#define H_ACTIVE_LO        (unsigned)dtd[ 2 ]

#define H_BLANKING_LO      (unsigned)dtd[ 3 ]

#define H_ACTIVE_HI        UPPER_NIBBLE( (unsigned)dtd[ 4 ] )

#define H_ACTIVE           COMBINE_HI_8LO( H_ACTIVE_HI, H_ACTIVE_LO )

#define H_BLANKING_HI      LOWER_NIBBLE( (unsigned)dtd[ 4 ] )

#define H_BLANKING         COMBINE_HI_8LO( H_BLANKING_HI, H_BLANKING_LO )




#define V_ACTIVE_LO        (unsigned)dtd[ 5 ]

#define V_BLANKING_LO      (unsigned)dtd[ 6 ]

#define V_ACTIVE_HI        UPPER_NIBBLE( (unsigned)dtd[ 7 ] )

#define V_ACTIVE           COMBINE_HI_8LO( V_ACTIVE_HI, V_ACTIVE_LO )

#define V_BLANKING_HI      LOWER_NIBBLE( (unsigned)dtd[ 7 ] )

#define V_BLANKING         COMBINE_HI_8LO( V_BLANKING_HI, V_BLANKING_LO )



#define H_SYNC_OFFSET_LO   (unsigned)dtd[ 8 ]
#define H_SYNC_WIDTH_LO    (unsigned)dtd[ 9 ]

#define V_SYNC_OFFSET_LO   UPPER_NIBBLE( (unsigned)dtd[ 10 ] )
#define V_SYNC_WIDTH_LO    LOWER_NIBBLE( (unsigned)dtd[ 10 ] )

#define V_SYNC_WIDTH_HI    ((unsigned)dtd[ 11 ] & (1|2))
#define V_SYNC_OFFSET_HI   (((unsigned)dtd[ 11 ] & (4|8)) >> 2)

#define H_SYNC_WIDTH_HI    (((unsigned)dtd[ 11 ] & (16|32)) >> 4)
#define H_SYNC_OFFSET_HI   (((unsigned)dtd[ 11 ] & (64|128)) >> 6)


#define V_SYNC_WIDTH       COMBINE_HI_4LO( V_SYNC_WIDTH_HI, V_SYNC_WIDTH_LO )
#define V_SYNC_OFFSET      COMBINE_HI_4LO( V_SYNC_OFFSET_HI, V_SYNC_OFFSET_LO )

#define H_SYNC_WIDTH       COMBINE_HI_4LO( H_SYNC_WIDTH_HI, H_SYNC_WIDTH_LO )
#define H_SYNC_OFFSET      COMBINE_HI_4LO( H_SYNC_OFFSET_HI, H_SYNC_OFFSET_LO )

#define H_SIZE_LO          (unsigned)dtd[ 12 ]
#define V_SIZE_LO          (unsigned)dtd[ 13 ]

#define H_SIZE_HI          UPPER_NIBBLE( (unsigned)dtd[ 14 ] )
#define V_SIZE_HI          LOWER_NIBBLE( (unsigned)dtd[ 14 ] )

#define H_SIZE             COMBINE_HI_8LO( H_SIZE_HI, H_SIZE_LO )
#define V_SIZE             COMBINE_HI_8LO( V_SIZE_HI, V_SIZE_LO )

#define H_BORDER           (unsigned)dtd[ 15 ]
#define V_BORDER           (unsigned)dtd[ 16 ]

#define FLAGS              (unsigned)dtd[ 17 ]

#define INTERLACED         (FLAGS&128)
#define SYNC_TYPE	   (FLAGS&3<<3)  /* bits 4,3 */
#define SYNC_SEPARATE	   (3<<3)
#define HSYNC_POSITIVE	   (FLAGS & 4)
#define VSYNC_POSITIVE     (FLAGS & 2)

#define MONITOR_NAME            0xfc
#define MONITOR_LIMITS          0xfd
#define UNKNOWN_DESCRIPTOR      -1
#define DETAILED_TIMING_BLOCK   -2


#define DESCRIPTOR_DATA         5
#define V_MIN_RATE              block[ 5 ]
#define V_MAX_RATE              block[ 6 ]
#define H_MIN_RATE              block[ 7 ]
#define H_MAX_RATE              block[ 8 ]

#define MAX_PIXEL_CLOCK         (((int)block[ 9 ]) * 10)
#define GTF_SUPPORT		block[10]

#define DPMS_ACTIVE_OFF		(1 << 5)
#define DPMS_SUSPEND		(1 << 6)
#define DPMS_STANDBY		(1 << 7)

char* myname;

void MSG( const char* x )
{
  fprintf( stderr, "%s: %s\n", myname, x ); 
}


int
parse_edid( byte* edid );


int
parse_timing_description( byte* dtd );


int
parse_monitor_limits( byte* block );

int
block_type( byte* block );

char*
get_monitor_name( byte const*  block );

char*
get_vendor_sign( byte const* block );

int
parse_dpms_capabilities( byte flags );

int
parse_edid( byte* edid )
{
  unsigned i;
  byte* block;
  char* monitor_name = NULL;
  char monitor_alt_name[100];
  byte checksum = 0;
  char *vendor_sign;
  int ret = 0;
  
  for( i = 0; i < EDID_LENGTH; i++ )
    checksum += edid[ i ];

  if (  checksum != 0  ) {
      MSG( "EDID checksum failed - data is corrupt. Continuing anyway." );
      ret = 1;
  }

  if ( strncmp( (const char*) edid+EDID_HEADER, (const char*) edid_v1_header, EDID_HEADER_END+1 ) )
    {
      MSG( "first bytes don't match EDID version 1 header" );
      MSG( "do not trust output (if any)." );
      ret = 1;
    }

  vendor_sign = get_vendor_sign( edid + ID_MANUFACTURER_NAME ); 

  block = edid + DETAILED_TIMING_DESCRIPTIONS_START;

  for( i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
	 block += DETAILED_TIMING_DESCRIPTION_SIZE )
    {

      if ( block_type( block ) == MONITOR_NAME )
	{
	  monitor_name = get_monitor_name( block );
	  break;
	}
    }

  if (!monitor_name) {
    /* Stupid djgpp hasn't snprintf so we have to hack something together */
    if(strlen(vendor_sign) + 10 > sizeof(monitor_alt_name))
      vendor_sign[3] = 0;
    
    sprintf(monitor_alt_name, "%s:%02x%02x",
	    vendor_sign, edid[ID_MODEL], edid[ID_MODEL+1]) ;
    monitor_name = monitor_alt_name;
  }

  block = edid + DETAILED_TIMING_DESCRIPTIONS_START;

  for( i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
	 block += DETAILED_TIMING_DESCRIPTION_SIZE )
    {

      if ( block_type( block ) == MONITOR_LIMITS )
	parse_monitor_limits( block );
    }

  parse_dpms_capabilities(edid[DPMS_FLAGS]);

  block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
 
  printf("<modes>\n");
  for( i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++,
	 block += DETAILED_TIMING_DESCRIPTION_SIZE )
    {

      if ( block_type( block ) == DETAILED_TIMING_BLOCK )
	parse_timing_description( block );
    }
  printf("</modes>\n");


  return ret;
}


int
parse_timing_description( byte* dtd )
{
  int htotal, vtotal;
  htotal = H_ACTIVE + H_BLANKING;
  vtotal = V_ACTIVE + V_BLANKING;
  
  printf( "<mode>\n\t<width>%d</width>\n\t<height>%d</height>\n", H_ACTIVE, V_ACTIVE );
  printf( "\t<vfreq>%.0f</vfreq>\n\t<hfreq>%.0f</hfreq>\n",
	  (double)PIXEL_CLOCK/((double)vtotal*(double)htotal),
	  (double)PIXEL_CLOCK/(double)(htotal*1000));

  if ( INTERLACED )
    printf( "\t<interlaced>true</interlaced>\n");
  else
    printf( "\t<interlaced>false</interlaced>\n");

  printf( "</mode>\n" );

  return 0;
}


int
block_type( byte* block )
{
  if ( !strncmp( (const char*) edid_v1_descriptor_flag, (const char*) block, 2 ) )
    {

      /* descriptor */

      if ( block[ 2 ] != 0 )
	return UNKNOWN_DESCRIPTOR;


      return block[ 3 ];
    } else {

      /* detailed timing block */

      return DETAILED_TIMING_BLOCK;
    }
}

char*
get_monitor_name( byte const* block )
{
  static char name[ 13 ];
  unsigned i;
  byte const* ptr = block + DESCRIPTOR_DATA;


  for( i = 0; i < 13; i++, ptr++ )
    {

      if ( *ptr == 0xa )
	{
	  name[ i ] = 0;
	  return name;
	}

      name[ i ] = *ptr;
    }


  return name;
}


char* get_vendor_sign( byte const* block )
{
  static char sign[4];
  unsigned short h;

  /*
     08h	WORD	big-endian manufacturer ID (see #00136)
		    bits 14-10: first letter (01h='A', 02h='B', etc.)
		    bits 9-5: second letter
		    bits 4-0: third letter
  */
  h = COMBINE_HI_8LO(block[0], block[1]);
  sign[0] = ((h>>10) & 0x1f) + 'A' - 1;
  sign[1] = ((h>>5) & 0x1f) + 'A' - 1;
  sign[2] = (h & 0x1f) + 'A' - 1;
  sign[3] = 0;
  return sign;
}

int
parse_monitor_limits( byte* block )
{
  return 0;
}

int
parse_dpms_capabilities(byte flags)
{
  return 0;
}
    
