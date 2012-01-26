//-----------------------------------------------------------------------------
// Copyright (c) 2008-2009 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//------------------------------------------------------------------------------
/*------------------------------------------------------------------------------
 * This is an OpenGL ES 2.0 sample program using GDL and EGL. It shows how to set
 * up GDL and EGL for OpenGL ES 2.0 program. Also it demonstrates how to 
 * set vertex attributes such as vertex positions, normals, color values and  
 * texture coordinates. Shows how to initialize uniform variables in the 
 * application. In addition, it shows how to use texture mapping.
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "libgdl.h"

#include "matrix.h"
#include "image.h"

// Plane size and position
#define ORIGIN_X    0
#define ORIGIN_Y    0
#define WIDTH       640
#define HEIGHT      480
#define ASPECT ((GLfloat)16 / (GLfloat)9)
#define SQUARE_SIZE 25     // half of the cube side length
#define MSG_LEN     1024   // error message length
#define TRUE 1
#define FALSE 0

/* All the matrices are initialized to identity matrix */
static GLfloat projection[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static GLfloat modelview[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static GLfloat mvp[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};  

/**
 * Load an JPEG image for texture mapping
 */
GLuint read_jpeg_image(imagebuffer_st * image, char *filename, int alpha_value)
{
    GLuint ret = TRUE;    

    if (image != NULL
        && filename != NULL)
    {
        fprintf(stdout,"readjpg(%s)\n",filename);

        image->color_format = IMAGE_RGBA;
        image->lines_format = IMAGE_REVERSE;

        if (read_JPEG_file(image, filename, alpha_value))
        {
            fprintf(stdout,"Loaded: %s (%dx%d)\n", filename, image->width, image->height);
        }
        else
        {
            fprintf(stderr,"Cannot load %s\n", filename);
            ret = FALSE;
        }
    }
    else
    {
        ret = FALSE;
        fprintf(stderr, "read_jpeg_image() error: null input parameters.\n");
    }
    return ret;
}


/**
 * Read the shader files
 */
char * readShaderText(const char * fileName, int * len)
{
    char *pText = NULL;  
    if (fileName != NULL)
    { 
        FILE *pFile = fopen(fileName, "r");
        if (pFile == NULL)
        {
            fprintf(stderr, "File not found! '%s'\n", fileName);
        }
        else
        {
            fseek(pFile, 0, SEEK_END);
            *len = ftell(pFile);
            rewind(pFile);

            if (*len > 0 )
            {
                pText = (char*)malloc(*len+1);
                if (pText != NULL)
                {
                    *len = (int)fread(pText, sizeof(char), *len, pFile);
                    pText[*len] = '\0';
                }
            }
            fclose(pFile);
        }
    }
    else 
    {
        fprintf(stderr,"readShaderText() error: file name can not be empty.\n");
    }
    return pText;
}

/**
 * Create the shaders
 */
unsigned int createGLShader(const char * fileName, int shaderType)
{
    GLuint shaderId = 0;       // vertex or fragment shader Id
    char pInfoLog[MSG_LEN+1];    // error message
    int shaderStatus, infoLogLength;    //shader's status and error information length
    int shaderTexLen = 0;      // shader text length
    const char* pShaderText = readShaderText(fileName, &shaderTexLen);
    if (pShaderText != NULL)
    {
        if( 1 == shaderType )
        {
            shaderId = glCreateShader(GL_VERTEX_SHADER);
        }
        else 
        {
            shaderId = glCreateShader(GL_FRAGMENT_SHADER);
        }
        glShaderSource(shaderId, 1, (const char **)&pShaderText, &shaderTexLen);
        glCompileShader(shaderId);

        free((void*)pShaderText);
        glGetShaderiv( shaderId, GL_COMPILE_STATUS, &shaderStatus);
        if (shaderStatus != GL_TRUE)
        {
            if( 1 == shaderType )
            {
                fprintf(stderr,"Error: Failed to compile GL vertex Shader\n");
            }
            else
            {    
                fprintf(stderr,"Error: Failed to compile GL fragment Shader\n");
            }
            glGetShaderInfoLog( shaderId, MSG_LEN, &infoLogLength, pInfoLog);
            if (infoLogLength > MSG_LEN)
            {
                 pInfoLog[MSG_LEN] = '\0';
            }
            else
            {
                pInfoLog[infoLogLength] = '\0';
            }
            fprintf(stderr, "%s",pInfoLog);

        }
    }
    return shaderId;
}

/**
 * Initializes a plane for the graphics to be rendered to
 */
gdl_ret_t setup_plane(gdl_plane_id_t plane)
{
    gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
    gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
    gdl_rectangle_t srcRect;
    gdl_rectangle_t dstRect;
    gdl_ret_t rc = GDL_SUCCESS;

    dstRect.origin.x = ORIGIN_X;
    dstRect.origin.y = ORIGIN_Y;
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
        fprintf(stderr,"GDL configuration failed! GDL error code is 0x%x\n", rc);
    }
  
    return rc;
}


/**
 * Initializes EGL, creates a rendering context, and creates a
 *  window surface to render to
 */
int setup_egl(gdl_plane_id_t plane, EGLDisplay* display, EGLSurface* surface, EGLContext* context)
{
    int ret = TRUE; 

    NativeWindowType window = (NativeWindowType)plane;
    EGLint configAttrs[] =
    {
        EGL_BUFFER_SIZE,       EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS,    1,   /* multisample is set */
        EGL_SAMPLES,           4,   /* number of samples per pixel */
        EGL_DEPTH_SIZE,        16,
        EGL_RED_SIZE,          8,
        EGL_GREEN_SIZE,        8,
        EGL_BLUE_SIZE,         8,
        EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT, 
        EGL_NONE
    };
    EGLint contextAttrs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2, // Specifies to use OpenGL ES 2.0
        EGL_NONE
    };
    EGLConfig config = 0;
    EGLint numconfigs = 0;
    EGLint major = 0, minor = 0;
    EGLBoolean ok = EGL_FALSE;
    
    if (display != NULL
        && surface != NULL
        && context != NULL)
    {
        *display = eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY);
        if (*display != EGL_NO_DISPLAY)
        {
            ok = eglInitialize(*display, &major, &minor);
        }
        else
        {
            ok = EGL_FALSE;
        }
        fprintf(stdout,"EGL %d.%d initialized\n", major, minor);

        if (ok)
        {
            ok = eglChooseConfig(*display, configAttrs, &config, 1, &numconfigs);
        }

        if (ok && numconfigs > 0)
        {
            *surface = eglCreateWindowSurface(*display, config, window, NULL);
        }
        else if (numconfigs <= 0)
        {
            ok = EGL_FALSE;
        }

        if (*surface != EGL_NO_SURFACE)
        {
            ok = eglBindAPI(EGL_OPENGL_ES_API);
        }

        if (ok)
        {
            *context = eglCreateContext(*display, config, NULL, contextAttrs);
        }

        if (*context != EGL_NO_CONTEXT)
        {
            ok = eglMakeCurrent(*display, *surface, *surface, *context);
        }

        if (*display == EGL_NO_DISPLAY
            || *surface == EGL_NO_SURFACE
            || *context == EGL_NO_CONTEXT
            || ok != EGL_TRUE)
        {
            fprintf(stderr,"EGL initialization failed!\n");
            fprintf(stderr,"display = 0x%x  surface = 0x%x  context = 0x%x\n",
                    *display, *surface, *context);
            ret = FALSE;
        }
    }
    else
    {
        ret = FALSE;
        fprintf(stderr, "Error: pointer values should not be empty.\n");
    }

    return ret;
}

/**
 * clean up the egl
 */
void close_egl(EGLDisplay display, EGLSurface surface, EGLContext context)
{
    if (context != EGL_NO_CONTEXT)
    {
        eglDestroyContext(display, context);
    }
    if (surface != EGL_NO_SURFACE)
    {
        eglDestroySurface(display, surface);
    }

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (display != EGL_NO_DISPLAY)
    {
        eglTerminate(display);
    }
}

/**
 * Initializes the vertex positions, colors and texture coordinates.
 * Initializes OpenGL by setting up projection matrix, enabling necessary state 
 * to pass positions, colors and texture coordinates to OpenGL. Get index for
 * uniform variables used in vertex and fragment shaders.
 */
int setup_gl(GLuint* programId, GLuint* vertexShaderId, GLuint* fragmentShaderId, int * mvp_loc)
{
    GLuint status, ret;           /* gl error status and return value for this setup */
    GLuint tex_loc;               /* for the texture coordinate attribute index */
    int texture_pos = 0;          /* for the texture unit id uniform variable */
    
    int shaderStatus = GL_FALSE;  // shader status 
    int infoLen;                  // error checking info length
    char infoLog[MSG_LEN+1];        // error checking info log
 
    ret = TRUE;                   // set return value to be true 
    static GLfloat verts[] =      // vertex position x, y and z values
    {
        -SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        
        SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
 
        -SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,

        SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,

        -SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,

        SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        -SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,

        -SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,

        SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,

        SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,

        SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,
        SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,

        -SQUARE_SIZE, -SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,

        -SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE,
        -SQUARE_SIZE, SQUARE_SIZE, -SQUARE_SIZE,
        -SQUARE_SIZE, -SQUARE_SIZE, -SQUARE_SIZE,
    };

    static GLfloat colors[] =           // vertex color  r, g, b and alpha values
    {
        1.0f, 0.0f, 0.0f, 0.5f,
        1.0f, 0.0f, 0.0f, 0.5f,
        1.0f, 0.0f, 0.0f, 0.5f,
        1.0f, 0.0f, 0.0f, 0.5f,  
        1.0f, 0.0f, 0.0f, 0.5f,  
        1.0f, 0.0f, 0.0f, 0.5f,  
        
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,
        0.0f, 1.0f, 0.0f, 0.5f,

        1.0f, 1.0f, 0.0f, 0.5f,
        1.0f, 1.0f, 0.0f, 0.5f,
        1.0f, 1.0f, 0.0f, 0.5f,
        1.0f, 1.0f, 0.0f, 0.5f,
        1.0f, 1.0f, 0.0f, 0.5f,
        1.0f, 1.0f, 0.0f, 0.5f,

        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,
        0.0f, 0.0f, 1.0f, 0.5f,

        0.6f, 0.6f, 0.0f, 0.5f,
        0.6f, 0.6f, 0.0f, 0.5f,
        0.6f, 0.6f, 0.0f, 0.5f,
        0.6f, 0.6f, 0.0f, 0.5f,
        0.6f, 0.6f, 0.0f, 0.5f,
        0.6f, 0.6f, 0.0f, 0.5f,
   
        0.0f, 0.6f, 0.6f, 0.5f,
        0.0f, 0.6f, 0.6f, 0.5f,
        0.0f, 0.6f, 0.6f, 0.5f,
        0.0f, 0.6f, 0.6f, 0.5f,
        0.0f, 0.6f, 0.6f, 0.5f,
        0.0f, 0.6f, 0.6f, 0.5f
    };
    
    static GLfloat texCoord[] =            // texture coordinates for each vertex 
    {
        0,0, 1,0, 0,1,
        1,0, 1,1, 0,1, 
        0,0, 1,0, 0,1,
        1,0, 1,1, 0,1, 
        1,1, 0,1, 0,0,
        0,0, 1,0, 1,1,
        0,0, 1,0, 1,1,
        1,1, 0,1, 0,0,
        0,0, 1,0, 0,1,
        1,0, 1,1, 0,1,
        1,0, 1,1, 0,0,
        1,1, 0,1, 0,0
    };

    if ( programId != NULL
         && vertexShaderId != NULL
         && fragmentShaderId != NULL
         && mvp_loc != NULL )
    {
        imagebuffer_st image;         // image structure for reading jpg from JPEG file
        if (read_jpeg_image(&image,"sky.jpg", 0xFF) == TRUE)
        {    
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.buffer);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
            // Free the memory for the JPEG file after it was copied into texture memory
            if (image.buffer)
            {
                free(image.buffer);
            }

            // Create the Shader programs    
            *vertexShaderId = createGLShader("cube_vertshader.txt", 1);
            *fragmentShaderId = createGLShader("cube_fragshader.txt", 2);
     
            if( (*vertexShaderId) && (*fragmentShaderId) )
            { 
                // Create Programs
                *programId = glCreateProgram();

                glAttachShader(*programId, *vertexShaderId);
                glAttachShader(*programId, *fragmentShaderId);

                glBindAttribLocation(*programId, 0, "position");
                glBindAttribLocation(*programId, 1, "inputcolor");

                glLinkProgram(*programId);
       
                glGetProgramiv(*programId, GL_LINK_STATUS, &shaderStatus);
                if (shaderStatus != GL_TRUE)
                {
                    fprintf(stderr,"Error: Failed to link GLSL program\n");
                    glGetProgramInfoLog(*programId, MSG_LEN, &infoLen, infoLog);
                    if (infoLen > MSG_LEN)
                    {
                        infoLog[MSG_LEN] = '\0';
                    }
                    fprintf(stderr,"%s\n",infoLog);
                    ret = FALSE;
                } 
                else  // (shaderStatus == GL_TRUE)
                {
                    glValidateProgram(*programId); 
                    glGetProgramiv(*programId, GL_VALIDATE_STATUS, &shaderStatus);
                    if (shaderStatus != GL_TRUE)
                    {
                        fprintf(stderr,"Error: Failed to validate GLSL program\n");
                        glGetProgramInfoLog(*programId, MSG_LEN, &infoLen, infoLog);
                        if (infoLen > MSG_LEN)
                        {
                            infoLog[MSG_LEN] = '\0';
                        }
                        fprintf(stderr,"%s\n",infoLog);
                        ret = FALSE;
                    }
                }
       
                if (shaderStatus == GL_TRUE)
                {
                    *mvp_loc = glGetUniformLocation(*programId, "mvp");
                    texture_pos = glGetUniformLocation(*programId, "texture");
        
                    glUseProgram(*programId);
                    glUniform1i(texture_pos, 0);

                    fprintf(stdout,"Vertex and fragment shaders created. \n");
                    glClearColor(0.0f, 0.0f, 0.0f,0.0f);
                    glEnable(GL_DEPTH_TEST);
                    glEnableVertexAttribArray(0);
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, verts);
                    glVertexAttribPointer(1, 4, GL_FLOAT, 0, 0, colors);
        
                    tex_loc = glGetAttribLocation(*programId, "inputtexture");
                    glVertexAttribPointer(tex_loc, 2, GL_FLOAT, 0, 0, texCoord);
                    glEnableVertexAttribArray(tex_loc);

                    fprintf(stdout, "OpenGL version is : %s\n", glGetString(GL_VERSION));
        
                    myIdentity(projection);

                    myFrustum(projection, -100*ASPECT,100*ASPECT, -100, 100, 175, 300); 
            
                    status = glGetError();   /* check openGL error */
                    if (status != GL_NO_ERROR)
                    {
                        fprintf(stderr, "GL_ERROR = %x", status);
                        ret = FALSE;
                    }

                    glViewport(1220, 680, 800, 500);
                }
            }
            else
            {
                fprintf(stderr, "Error: Could not set up openGL properly\n");
                ret = FALSE;
            }
        }
        else  /* failed to read jpeg file successfully */ 
        {
            ret = FALSE;
        }
    }
    else
    {
        fprintf(stderr,"setup_gl() error: null input parameters.\n");
        ret = FALSE;
    }
    return ret;
}

/**
 *clean up OpenGL
 */
void close_gl(GLuint programId, GLuint vertexShaderId, GLuint fragmentShaderId)
{
    if (vertexShaderId)
    {
        glDetachShader(programId, vertexShaderId);
    }
    if (fragmentShaderId)
    {
        glDetachShader(programId, fragmentShaderId);
    }
    if (programId)
    {
        glDeleteProgram(programId);
    }
    if (vertexShaderId)
    {
        glDeleteShader(vertexShaderId);
    }
    if (fragmentShaderId)
    {
        glDeleteShader(fragmentShaderId);
    }
}

/**
 * Renders the cube 
 */
void render(GLfloat rot_x, GLfloat rot_y, int mvp_loc)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myIdentity(modelview);
    myTranslate(modelview, 0,0,-SQUARE_SIZE*10);
    myRotate(modelview, rot_x, 1, 0, 0);
    myRotate(modelview, rot_y, 0, 1, 0);
    
    myMultMatrix(mvp, projection, modelview);
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp);

    glDrawArrays(GL_TRIANGLES, 0, 36 );
}

void usage(char * name)
{
    fprintf(stdout, "\nUsage: %s [ frames ]\n", name);
    fprintf(stdout, "where:\n\n");
    fprintf(stdout, "  frames:  a positive number for the number of frames to run.\n");
}

#if 0
/** 
 * There should be a command line argument for the number of frames 
 * it will run.  
 */
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
    
    if (argc == 2)
    {
        frame_count = (unsigned int)atoi(argv[1]);
        if (frame_count <= 0)
        {
            usage(argv[0]);
            return 0;
        }
    }
    else 
    {
        usage(argv[0]);

        return 0;
    }
    
    gdl_plane_id_t plane = GDL_PLANE_ID_UPP_C;

    gdl_init(0);

    status = setup_plane(plane);
    if (status == GDL_SUCCESS)
    {
        status = setup_egl(plane, &display, &surface, &context);
        if (status == GL_TRUE)
        {
            status = setup_gl(&programId, &vertexShaderId, &fragmentShaderId, &mvpLoc);
            if (status == GL_TRUE)
            {    
                while (frame_count > 0)
                { 
                    render(rotation_x, rotation_y, mvpLoc);
                    eglSwapBuffers(display, surface);

                    rotation_x += 1.0f;
                    if (rotation_x > 360.0f)
                    {
                        rotation_x = 0.0f;
                    }
            
                    rotation_y += 0.1f;
                    if (rotation_y > 360.0f)
                    {
                        rotation_y = 0.0f;
                    }

                    frame_count--;
                }
            }
            close_gl(programId, vertexShaderId, fragmentShaderId);
        }
        close_egl(display, surface, context);
    }

    gdl_close();

    return 0;
}
#endif
