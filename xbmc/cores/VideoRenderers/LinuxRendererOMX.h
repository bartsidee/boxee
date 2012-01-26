#ifndef LINUXRENDERERGL_RENDERER
#define LINUXRENDERERGL_RENDERER

#include "system.h"

#if defined(HAS_OPENMAX)

#include "../../../guilib/FrameBufferObject.h"
#include "../../../guilib/Shader.h"
#include "../ffmpeg/DllSwScale.h"
#include "../ffmpeg/DllAvCodec.h"
#include "../../settings/VideoSettings.h"
#include "RenderFlags.h"
#include "GraphicContext.h"
#include "BaseRenderer.h"
#include "utils/Event.h"

#include <OMX_Types.h>
  
class CBaseTexture;

#define NUM_BUFFERS 3

#define MAX_PLANES 3
#define MAX_FIELDS 3

#undef ALIGN
#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#if 0
typedef struct YV12Image
{
  BYTE *   plane[MAX_PLANES];
  unsigned stride[MAX_PLANES];
  unsigned width;
  unsigned height;
  unsigned flags;

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;
} YV12Image;

#endif

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */
#define IMAGE_FLAG_READY     0x10 /* image is ready to be uploaded to texture memory */
#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

enum EFIELDSYNC
{
  FS_NONE,
  FS_ODD,
  FS_EVEN
};

struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

struct YUVCOEF
{
  float r_up, r_vp;
  float g_up, g_vp;
  float b_up, b_vp;
};

enum RenderMethod
{
  RENDER_GLSL=0x01,
  RENDER_ARB=0x02,
  RENDER_SW=0x04,
  RENDER_VDPAU=0x08,
  RENDER_POT=0x10
};

enum RenderQuality
{
  RQ_LOW=1,
  RQ_SINGLEPASS,
  RQ_MULTIPASS,
  RQ_SOFTWARE
};

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

#define FIELD_FULL 0
#define FIELD_ODD 1
#define FIELD_EVEN 2

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

#define OMX_OVRLAY_MAX_BUFFER_CNT 20

struct OMX_BUFFERHEADERTYPE;
struct OmxRendererState;

enum EOMXRendererEvent {
    eOMXRendererEventStateChanged,
    eOMXRendererEventFlush,
    eOMXRendererEventPortDisable,
    eOMXRendererEventEndOfStream,
    eOMXRendererEventPortSettingsChanged,
    eOMXRendererEventFirstFrameDisplayed,
    eOMXRendererEventUndefined
};

typedef void (*ON_OMX_RENDERER_EMPTY_BUFFER_DONE)(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
typedef void (*ON_OMX_RENDERER_FILL_BUFFER_DONE)(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
typedef void (*ON_OMX_RENDERER_EVENT_COMPLETION)(void *pClientThis, OmxRendererState *pOmxRendererState, EOMXRendererEvent eOmxEvent, unsigned int uData1, unsigned int uData2);

struct OmxRendererCallBacks {
    ON_OMX_RENDERER_EMPTY_BUFFER_DONE OnOmxRendererEmptyBufferDone;
    ON_OMX_RENDERER_FILL_BUFFER_DONE  OnOmxRendererFillBufferDone;
    ON_OMX_RENDERER_EVENT_COMPLETION  OnOmxRendererEventCompletion;
};

struct OmxRendererState {
    OmxRendererCallBacks *pCallBacks;
    void         *pClientThis;
    void         *pNvEvent;
};

class CLinuxRendererOMX : public CBaseRenderer
{
public:
  CLinuxRendererOMX();  
  virtual ~CLinuxRendererOMX();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};

  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */

  virtual void AutoCrop(bool bCrop);
  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  // Feature support
  virtual bool SupportsBrightness();
  virtual bool SupportsContrast();
  virtual bool SupportsGamma();
  virtual bool SupportsMultiPassRendering();
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

  inline void SetRGB32Image(const char *image, int nHeight, int nWidth, int nPitch)
  {
  }

protected:
  virtual void Render(DWORD flags, int renderBuffer);
  
  void ChooseUpscalingMethod();
  bool IsSoftwareUpscaling();
  void InitializeSoftwareUpscaling();
  
  virtual void ManageTextures();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  virtual bool CreateYV12Texture(int index, bool clear=true);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  void LoadTextures(int source);
  void UpdateVideoFilter();

  // renderers
  void RenderMultiPass(int renderBuffer, int field);  // multi pass glsl renderer
  void RenderSinglePass(int renderBuffer, int field); // single pass glsl renderer

  int m_iYV12RenderBuffer;                // The index to current renderer buffer for the decoder
  int m_NumYV12Buffers;                   // number of total render buffers
  int m_iLastRenderBuffer;                // index of the previous buffer that was rendererd
  int m_iLastReleasedBuffer;              // variable used to pass the index of the most previous released buffer to the next FlipPage if using AUTO_SOURCE
  int m_RenderBufferQueue[NUM_BUFFERS];   // simple queue the size of number of YV12 buffers to help the render keep track of output order from the decoder
  
  bool m_bRenderFromAppThread;
  bool m_bConfigured;
  bool m_bValidated;
  bool m_bImageReady;
  unsigned m_iFlags;
  unsigned int m_flipindex; // just a counter to keep track of if a image has been uploaded
  bool m_StrictBinding;

  // Software upscaling.
  int m_upscalingWidth;
  int m_upscalingHeight;
  YV12Image m_imScaled;
  
  // Raw data used by renderer
  int m_currentField;
  int m_reloadShaders;

  struct YUVPLANE
  {
    GLuint id;
    CRect  rect;

    float  width;
    float  height;

    unsigned texwidth;
    unsigned texheight;

    unsigned flipindex;
  };

  typedef YUVPLANE           YUVPLANES[MAX_PLANES];

  struct YUVBUFFER
  {
    YV12Image image;
    unsigned  flipindex; /* used to decide if this has been uploaded */
    OMX_BUFFERHEADERTYPE *pOmxBuffer;
  };

  typedef YUVBUFFER          YUVBUFFERS[NUM_BUFFERS];

  YUVBUFFERS m_buffers;

  void LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                , unsigned width,  unsigned height
                , int stride, void* data );

  // clear colour for "black" bars
  float m_clearColour;

  HANDLE m_eventTexturesDone[NUM_BUFFERS];
  CRect m_crop;
  float m_aspecterror;
  
  /* OMX related stuff */
  static void OnOmxRendererEmptyBufferDone(void *pClientThis, OMX_BUFFERHEADERTYPE *pBuffer);
  static void OnOmxRendererEventCompletion(void *pClientThis, OmxRendererState *pOmxDecState, EOMXRendererEvent eOmxEvent, unsigned int uData1, unsigned int uData2);

  /* OMX helpers */
  int SetProfileMode(OMX_HANDLETYPE hComp,int bFlip);
  
  OMX_VERSIONTYPE m_vOMX;
  bool m_bOmxInitialized;
  bool m_bOmxTunnelMode;
  void* m_hComponentHandle;

  OmxRendererCallBacks m_OmxrendererCallBacks;
  
  // Decoder output buffer data...
  unsigned int m_uOmxRendererBufferCount;
  unsigned int m_uOmxRendererBufSize;
  OMX_BUFFERHEADERTYPE *m_pOmxRendererBuffers[OMX_OVRLAY_MAX_BUFFER_CNT];
  OmxRendererState m_OmxRendererState;
           
  CEvent m_oEventOmxSetStateDone;
  CEvent m_oEventOmxEmptyBufferDone;

};


inline int NP2( unsigned x ) {
#if defined(_LINUX) && !defined(__POWERPC__) && !defined(__PPC__) && !defined(_ARM)
  // If there are any issues compiling this, just append a ' && 0'
  // to the above to make it '#if defined(_LINUX) && 0'

  // Linux assembly is AT&T Unix style, not Intel style
  unsigned y;
  __asm__("dec %%ecx \n"
          "movl $1, %%eax \n"
          "bsr %%ecx,%%ecx \n"
          "inc %%ecx \n"
          "shl %%cl, %%eax \n"
          "movl %%eax, %0 \n"
          :"=r"(y)
          :"c"(x)
          :"%eax");
  return y;
#else
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
#endif
}
#endif

#endif
