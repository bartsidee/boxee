#include <queue>
#include <libgdl.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <pthread.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef HAS_EMBEDDED
#include <ismd_core.h>
#include <ismd_viddec.h>
#include <ismd_vidrend.h>
#include <ismd_vidpproc.h>
#include <ismd_audio.h>
#endif
}

extern "C"
{
int setup_egl(gdl_plane_id_t plane, EGLDisplay* display, EGLSurface* surface, EGLContext* context);
int setup_gl(GLuint* programId, GLuint* vertexShaderId, GLuint* fragmentShaderId, int * mvp_loc);
void render(GLfloat rot_x, GLfloat rot_y, int mvp_loc);
}

#define WIDTH 1920
#define HEIGHT 1080

/**
 * Initializes a plane for the graphics to be rendered to
 */
gdl_ret_t setup_plane(gdl_plane_id_t plane, bool gfx)
{
  gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
  gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
  gdl_rectangle_t srcRect;
  gdl_rectangle_t dstRect;
  gdl_ret_t rc = GDL_SUCCESS;

  dstRect.origin.x = 0;
  dstRect.origin.y = 0;
  dstRect.width = WIDTH;
  dstRect.height = HEIGHT;

  srcRect.origin.x = 0;
  srcRect.origin.y = 0;
  srcRect.width = WIDTH;
  srcRect.height = HEIGHT;

  rc = gdl_plane_reset(plane);
  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_begin(plane);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
  }

  if (gfx)
  {
    int alpha = 255;
    gdl_plane_set_attr(GDL_PLANE_ALPHA_GLOBAL, &alpha);
  }
  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_end(GDL_FALSE);
  }
  else
  {
    gdl_plane_config_end(GDL_TRUE);
  }

  if (GDL_SUCCESS != rc)
  {
    fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n", rc);
  }

  return rc;
}

int timeval_subtract (struct timeval* result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }

  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int main(int argc, char *argv[])
{
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLSurface surface = EGL_NO_SURFACE;
  EGLContext context = EGL_NO_CONTEXT;
  GLfloat rotation_x = 0;       /* angle for rotation in x direction */
  GLfloat rotation_y = 0;       /* angle for rotation in y direction */

  GLuint vertexShaderId = 0;     /* vertex shader id */
  GLuint fragmentShaderId = 0;   /* fragment shader id */
  GLuint programId = 0;          /* program id */
  int mvpLoc = 0;               /* for the uniform varible index value of  mvp matrix */
  int status;                    /* function call's return value */
  unsigned int frame_count = 0;

  gdl_init(0);

  setup_plane(GDL_PLANE_ID_UPP_B, true);
  status = setup_egl(GDL_PLANE_ID_UPP_B, &display, &surface, &context);
  if (status != GL_TRUE)
  {
    printf("error setting egl\n");
    return 1;
  }

  status = setup_gl(&programId, &vertexShaderId, &fragmentShaderId, &mvpLoc);
  if (status != GL_TRUE)
  {
    printf("error setting gl\n");
    return 1;
  }

  int frame = 0;

  struct timeval start;
  gettimeofday(&start, NULL);
  struct timeval now;
  struct timeval sub;
  int duration;

  while (1)
  {
    render(rotation_x, rotation_y, mvpLoc);
    eglSwapBuffers(display, surface);
    frame++;

    rotation_x += 5.0f;
    if (rotation_x > 360.0f)
    {
      rotation_x = 0.0f;
    }

    rotation_y += 2.0f;
    if (rotation_y > 360.0f)
    {
      rotation_y = 0.0f;
    }

    gettimeofday(&now, NULL);
 
    timeval_subtract(&sub, &now, &start);
    duration = (sub.tv_usec + sub.tv_sec*1000000) / 1000;

    if (duration > 5000)
    {    
        printf("CUBE FPS = %d\n", duration / frame);
        frame = 0;
        start = now;
    }
  }
}

