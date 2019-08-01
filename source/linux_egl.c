// 08-JAN-2017
// $ gcc gles_linux.c -lm -lSDL2 -lEGL -lGLESv1_CM -o gles_linux

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// GLES
#include <GLES/gl.h>
#include <GLES/glext.h>

// EGL
#include <GLES/egl.h>

// SDL2
//#include <SDL2/SDL.h> // For Events
//#include <SDL2/SDL_syswm.h>

EGLDisplay  glDisplay;
EGLConfig   glConfig;
EGLContext  glContext;
EGLSurface  glSurface;

const char  *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;

//SDL_Window *glesWindow = NULL;

float mCubeRotation = 0.0f;

float vertices[] = {
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f
};

float colors[] = {
    0.0f,  1.0f,  0.0f,  1.0f,
    0.0f,  1.0f,  0.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  1.0f,  1.0f,
    1.0f,  0.0f,  1.0f,  1.0f
};

char indices[] = {
    0, 4, 5, 0, 5, 1,
    1, 5, 6, 1, 6, 2,
    2, 6, 7, 2, 7, 3,
    3, 7, 4, 3, 4, 0,
    4, 7, 6, 4, 6, 5,
    3, 0, 1, 3, 1, 2
};

static int shmem_dump(char *src, int size)
{
    char str[128];
    int inc;
    if (!src) return -1;

    inc = 0;
    sprintf(str, "memdump[0x%.8x] sz%d: \n", (uint32_t)src, size);
    printf("%s", str);
    while (inc < size) {
        sprintf(str, "%.2x ", *src);
        printf("%s", str);

        if (!((inc+1) % 16)) {
            sprintf(str, " %d \n", inc+1);
            printf("%s", str);
        }
        inc++;
        src++;
    }

    sprintf(str, "\n");
    printf("%s", str);

    return inc;
}

void emulateGLUperspective(GLfloat fovY, GLfloat aspect, GLfloat zNear,
                           GLfloat zFar)
{
    GLfloat fW, fH;
    fH = tan(fovY / 180 * M_PI) * zNear / 2;
    fW = fH * aspect;
    glFrustumf(-fW, fW, -fH, fH, zNear, zFar);
}

void init_GLES(void)
{
    int screenwidth  = 800;
    int screenheight = 480;
    int screenbpp    =  16;
    int fullscreen   =   0;

    EGLint egl_config_attr[] = {
        EGL_BUFFER_SIZE,    16,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,   0,
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLint numConfigs=0, majorVersion=0, minorVersion=0;
    //glesWindow = SDL_CreateWindow("LOR_GLES_DEMO", 0, 0, screenwidth, screenheight,
    //                              fullscreen ? (SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN) : SDL_WINDOW_OPENGL);
    glDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    printf("%s line: %d, glDisplay: %d \n", __FUNCTION__, __LINE__, (unsigned int)glDisplay);
    
    eglInitialize(glDisplay, &majorVersion, &minorVersion);
    
    printf("%s line: %d, majorVersion: %d minorVersion: %d\n", __FUNCTION__, __LINE__, majorVersion, minorVersion);
    
    numConfigs = 0;
    
    eglChooseConfig(glDisplay, egl_config_attr, &glConfig, 1, &numConfigs);
    
    printf("%s line: %d, glConfig: 0x%.8x, numConfigs: 0x%.8x \n", __FUNCTION__, __LINE__, (unsigned int)glConfig, (unsigned int)numConfigs);

    EGLint *value = malloc(512);
    memset(value, 0, 512);

    eglGetConfigAttrib(glDisplay, glConfig, EGL_VERSION, value);
    printf("%s line: %d, value: %d \n", __FUNCTION__, __LINE__, *value);
    shmem_dump((char *)value, 512);

    eglGetConfigAttrib(glDisplay, glConfig, EGL_VENDOR, value);
    printf("%s line: %d, value: %d \n", __FUNCTION__, __LINE__, *value);
    shmem_dump((char *)value, 512);
    
    //SDL_SysWMinfo sysInfo;
    //SDL_VERSION(&sysInfo.version); // Set SDL version
    //SDL_GetWindowWMInfo(glesWindow, &sysInfo);
    glContext = eglCreateContext(glDisplay, glConfig, EGL_NO_CONTEXT, NULL);

    printf("%s line: %d, glContext: 0x%.8x \n", __FUNCTION__, __LINE__, (unsigned int)glContext);
    
    glSurface = eglCreatePbufferSurface(glDisplay, glConfig, 0);

    printf("%s line: %d, glSurface: 0x%.8x \n", __FUNCTION__, __LINE__, (unsigned int)glSurface);
    
    eglMakeCurrent(glDisplay, glSurface, glSurface, glContext);
    
    eglSwapInterval(glDisplay, 1);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepthf(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glViewport(0, 0, screenwidth, screenheight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    emulateGLUperspective(45.0f, (float) screenwidth / (float) screenheight, 0.1f,
                          100.0f);
    glViewport(0, 0, screenwidth, screenheight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void draw_frame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -10.0f);
    glRotatef(mCubeRotation, 1.0f, 1.0f, 1.0f);
    glFrontFace(GL_CW);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glLoadIdentity();
    mCubeRotation -= 0.50f;
}

int main(void)
{
    int loop = 1;
    //SDL_Event event;
    init_GLES();

    /*
    while (loop) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    loop = 0;
                    break;
                }
            }
        }

        draw_frame();
        eglSwapBuffers(glDisplay, glSurface);
    }
    */
    
    draw_frame();

    // Cleaning
    eglDestroySurface(glDisplay, glSurface);
    eglDestroyContext(glDisplay, glContext);
    eglTerminate(glDisplay);
    //SDL_DestroyWindow(glesWindow);

    return 0;
}
