#pragma once

#include "../../settings/VideoSettings.h"
#include "RenderFlags.h"
#include "GraphicContext.h"
#include "BaseRenderer.h"
#include "utils/Thread.h"

#define NUM_BUFFERS 2

#define MAX_PLANES 3
#define MAX_FIELDS 3

#undef ALIGN
#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */
#define IMAGE_FLAG_READY     0x16 /* image is ready to be uploaded to texture memory */
#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)

enum EFIELDSYNC
{
  FS_NONE,
  FS_TOP,
  FS_BOT
};

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

struct VideoVertex
{
  float x, y, z;
  float y1, y2;
  float u1, u2;
  float v1, v2;
};

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

#define FIELD_FULL 0
#define FIELD_ODD 1
#define FIELD_EVEN 2

typedef unsigned char*    YUVMEMORYPLANES[MAX_PLANES];
typedef YUVMEMORYPLANES   YUVMEMORYBUFFERS[NUM_BUFFERS];

class CRenderCapture;

class CNullRenderer : public CBaseRenderer
{
public:
  CNullRenderer();  
  virtual ~CNullRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};

  // Player functions
  virtual bool         Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, CRect &rect);
  virtual bool         IsConfigured() { return true; }
  virtual int          GetImage(YV12Image *image, double pts, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         FlipPage(int source) { }
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual void         SetSpeed(int speed);
  virtual void         Flush();

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  // Feature support
  virtual bool SupportsBrightness();
  virtual bool SupportsContrast();
  virtual bool SupportsGamma();

  // Feature support
  virtual bool SupportsMultiPassRendering() { return false; }
  virtual bool Supports(ERENDERFEATURE feature) { return false; }
  virtual bool Supports(EINTERLACEMETHOD method) { return false; }
  virtual bool Supports(ESCALINGMETHOD method) { return false; }
  bool RenderCapture(CRenderCapture* capture) { return false; }
  bool IsTimed() { return true; }
};
