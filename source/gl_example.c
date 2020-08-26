//#include <QCoreApplication>
//#include <QDebug>
//#include <QImage>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

struct _escontext
{
  /// Native System informations
  EGLNativeDisplayType native_display;
  EGLNativeWindowType native_window;
  uint16_t window_width, window_height;
  /// EGL display
  EGLDisplay  display;
  /// EGL context
  EGLContext  context;
  /// EGL surface
  EGLSurface  surface;

};

struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;

struct _escontext ESContext = {
  .native_display = NULL,
  .window_width = 0,
  .window_height = 0,
  .native_window  = 0,
  .display = NULL,
  .context = NULL,
  .surface = NULL
};

#define TRUE 1
#define FALSE 0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERRNO(...)  fprintf(stderr, "Error : %s\n", strerror(errno)); fprintf(stderr, __VA_ARGS__)

struct bitmapHeader_s {
    char aspbmpMagic[4];
    int    aspbhSize;
    char aspbhReserve[4];
    int    aspbhRawoffset;
    int    aspbiSize;
    int    aspbiWidth;
    int    aspbiHeight;
    int    aspbiCPP;
    int    aspbiCompMethd;
    int    aspbiRawSize;
    int    aspbiResoluH;
    int    aspbiResoluV;
    int    aspbiNumCinCP;
    int    aspbiNumImpColor;
};

void CreateNativeWindow(char *title, int width, int height) 
{

  region = wl_compositor_create_region(compositor);

  wl_region_add(region, 0, 0, width, height);
  wl_surface_set_opaque_region(surface, region);

  struct wl_egl_window *egl_window = wl_egl_window_create(surface, width, height);
    
  if (egl_window == EGL_NO_SURFACE) {
    LOG("No window !?\n");
    exit(1);
  }
  else {
    LOG("Window created !\n");
  }
  
  ESContext.window_width = width;
  ESContext.window_height = height;
  ESContext.native_window = egl_window;

}

EGLBoolean CreateEGLContext ()
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;
   EGLint fbAttribs[] =
   {
       EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
       EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
       EGL_RED_SIZE,        8,
       EGL_GREEN_SIZE,      8,
       EGL_BLUE_SIZE,       8,
       EGL_NONE
   };
   
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
   EGLDisplay display = eglGetDisplay(ESContext.native_display);
   
   if (display == EGL_NO_DISPLAY) {
      LOG("No EGL Display...\n");
      return EGL_FALSE;
   }

   // Initialize EGL
   if (!eglInitialize(display, &majorVersion, &minorVersion)) {
      LOG("No Initialisation...\n");
      return EGL_FALSE;
   }

   // Get configs
   if ((eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0)) {
      LOG("No configuration...\n");
      return EGL_FALSE;
   }

   // Choose config
   if ( (eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
   {
      LOG("No configuration...\n");
      return EGL_FALSE;
   }

   // Create a surface
   surface = eglCreateWindowSurface(display, config, ESContext.native_window, NULL);
   if ( surface == EGL_NO_SURFACE )
   {
      LOG("No surface...\n");
      return EGL_FALSE;
   }

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
   if ( context == EGL_NO_CONTEXT )
   {
      LOG("No context...\n");
      return EGL_FALSE;
   }

   // Make the context current
   if ( !eglMakeCurrent(display, surface, surface, context) )
   {
      LOG("Could not make the current window current !\n");
      return EGL_FALSE;
   }

   ESContext.display = display;
   ESContext.surface = surface;
   ESContext.context = context;
   return EGL_TRUE;
}

void shell_surface_ping
(void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
}

void shell_surface_configure
(void *data, struct wl_shell_surface *shell_surface, uint32_t edges,
 int32_t width, int32_t height) {
  struct window *window = data;
  wl_egl_window_resize(ESContext.native_window, width, height, 0, 0);
}

void shell_surface_popup_done(void *data, struct wl_shell_surface *shell_surface) {
}

static struct wl_shell_surface_listener shell_surface_listener = {
  &shell_surface_ping,
  &shell_surface_configure,
  &shell_surface_popup_done
};

EGLBoolean CreateWindowWithEGLContext(char *title, int width, int height) {
  CreateNativeWindow(title, width, height);
  return CreateEGLContext();
}

void draw() {
  glClearColor(0.5, 0.3, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

unsigned long last_click = 0;
void RefreshWindow() { eglSwapBuffers(ESContext.display, ESContext.surface); }

static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *intfname, uint32_t version) {

  LOG("Got a registry event for %s id %d\n", intfname, id);
  if (strcmp(intfname, "wl_compositor") == 0) {
    compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
  }
  else if (strcmp(intfname, "wl_shell") == 0) {
    shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
  }
}

static void global_registry_remover
(void *data, struct wl_registry *registry, uint32_t id) {
  LOG("Got a registry losing event for %d\n", id);
}

const struct wl_registry_listener listener = {
  global_registry_handler,
  global_registry_remover
};

static void get_server_references() {

  struct wl_display * display = wl_display_connect(NULL);
  if (display == NULL) {
    LOG("Can't connect to wayland display !?\n");
    exit(1);
  }
  LOG("Got a display !");

  struct wl_registry *wl_registry = wl_display_get_registry(display);
  
  wl_registry_add_listener(wl_registry, &listener, NULL);
  
  // This call the attached listener global_registry_handler
  wl_display_dispatch(display);
  wl_display_roundtrip(display);

  // If at this point, global_registry_handler didn't set the 
  // compositor, nor the shell, bailout !
  if (compositor == NULL || shell == NULL) {
    LOG("No compositor !? No Shell !! There's NOTHING in here !\n");
    exit(1);
  }
  else {
    LOG("Okay, we got a compositor and a shell... That's something !\n");
    ESContext.native_display = display;
  }
}

void destroy_window() {
  eglDestroySurface(ESContext.display, ESContext.surface);
  wl_egl_window_destroy(ESContext.native_window);
  wl_shell_surface_destroy(shell_surface);
  wl_surface_destroy(surface);
  eglDestroyContext(ESContext.display, ESContext.context);
}

static int bitmapHeaderSetup(struct bitmapHeader_s *ph, int clr, int w, int h, int dpi, int flen) 
{
    int rawoffset=0, totsize=0, numclrp=0, calcuraw=0, rawsize=0;
    float resH=0, resV=0, ratio=39.27, fval=0;

    if (!w) return -1;
    if (!h) return -2;
    if (!dpi) return -3;
    if (!flen) return -4;
    memset(ph, 0, sizeof(struct bitmapHeader_s));

    if (clr == 8) {
        numclrp = 256;
        rawoffset = 1078;
        //numclrp = 0;
        //rawoffset = 54;
        calcuraw = w * h;
    }
    else if (clr == 24) {
        numclrp = 0;
        rawoffset = 54;
        calcuraw = w * h * 3;
    } else {
        printf("[BMP] reset header ERROR!!! color bits is %d \n", clr);
        return -5;
    }

    if (calcuraw != flen) {
        printf("[BMP] WARNNING!!! raw size %d is wrong, should be %d x %d x %d= %d \n", flen, w, h, clr / 8, calcuraw);
        if (flen > calcuraw) {
            rawsize = calcuraw;
        } else {
            rawsize = flen;
        }
    } else {
        rawsize = calcuraw;
    }

    totsize = rawsize + rawoffset;
    
    fval = dpi;
    resH = fval * ratio;
    fval = dpi;
    resV = fval * ratio;
    
    ph->aspbmpMagic[2] = 'B';
    ph->aspbmpMagic[3] = 'M';       
    ph->aspbhSize = totsize; // file total size
    ph->aspbhRawoffset = rawoffset; // header size include color table 54 + 1024 = 1078
    ph->aspbiSize = 40;
    ph->aspbiWidth = w; // W
    ph->aspbiHeight = h; // H
    ph->aspbiCPP = 1;
    //ph->aspbiCPP = 0;
    ph->aspbiCPP |= clr << 16;  // 8 or 24
    ph->aspbiCompMethd = 0;
    ph->aspbiRawSize = rawsize; // size of raw
    ph->aspbiResoluH = (int)resH; // dpi x 39.27
    ph->aspbiResoluV = (int)resV; // dpi x 39.27
    ph->aspbiNumCinCP = numclrp;  // 24bit is 0, 8bit is 256
    ph->aspbiNumImpColor = 0;

    return 0;
}

static FILE *find_save(char *dst, char *tmple)
{
    int i;
    FILE *f;
    for (i =0; i < 1000; i++) {
        sprintf(dst, tmple, i);
        f = fopen(dst, "r");
        if (!f) {
            //printf("open file [%s]\n", dst);
            break;
        } else {
            //printf("open file [%s] succeed \n", dst);
            fclose(f);
        }
    }
    f = fopen(dst, "w");
    return f;
}

static int grapglbmp(unsigned char *ptr, int width, int height, int dpp, int len)
{
    char ptfilepath[256];
    static char ptfiledump[] = "/home/root/gldump_%.3d.bmp";
    FILE *dumpFile=0;

    struct bitmapHeader_s * bmpheader=0;
    char *ph=0;
    int hlen=0, ret=0;

    bmpheader = malloc(sizeof(struct bitmapHeader_s));
    memset(bmpheader, 0, sizeof(struct bitmapHeader_s));

    ph = &bmpheader->aspbmpMagic[2];

    bitmapHeaderSetup(bmpheader, dpp, width, height, 300, len);

    dumpFile = find_save(ptfilepath, ptfiledump);
    if (!dumpFile) {
        return -3;
    }

    hlen = sizeof(struct bitmapHeader_s) - 2;

    ret = fwrite(ph, 1, hlen, dumpFile);
    printf("[GL] write file %s size: %d ret: %d \n", ptfilepath, hlen, ret);
        
    ret = fwrite(ptr, 1, len, dumpFile);    
    printf("[GL] write file %s size: %d ret: %d \n", ptfilepath, len, ret);
    
    //sync();
    fflush(dumpFile);
    fclose(dumpFile);

}

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

int main() {

  get_server_references();

  surface = wl_compositor_create_surface(compositor);
  if (surface == NULL) {
    LOG("No Compositor surface ! Yay....\n");
    exit(1);
  }
  else LOG("Got a compositor surface !\n");

  shell_surface = wl_shell_get_shell_surface(shell, surface);
  wl_shell_surface_set_toplevel(shell_surface);

  CreateWindowWithEGLContext("Nya", 640, 360);

  while (1) {
    wl_display_dispatch_pending(ESContext.native_display);
    draw();
    RefreshWindow();
  }

  wl_display_disconnect(ESContext.native_display);
  LOG("Display disconnected !\n");
  
  exit(0);
}

#if 0
int main(int argc, char *argv[])
{
    #define CONTEXT_ES20
    EGLBoolean eglret = 0;
    
    printf("main() \n");

    EGLint ai32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    // Step 1 - Get the default display.
    EGLDisplay eglDisplay =0;
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    printf("main() line: %d eglDisplay: 0x%.8x\n", __LINE__, (unsigned int)eglDisplay);

    // Step 2 - Initialize EGL.
    EGLint majorVersion=0, minorVersion=0;
    eglret = eglInitialize(eglDisplay, &majorVersion, &minorVersion);
    printf("main() line: %d, minorVersion %d, minorVersion %d eglret: %d \n", __LINE__, majorVersion, minorVersion, eglret);
    
    // Step 3 - Make OpenGL ES the current API.

    printf("main() line: %d \n", __LINE__);
    
    //eglBindAPI(EGL_OPENGL_ES_API);
    //eglInitialize(EGLDisplay dpy, EGLint * major, EGLint * minor)
    
    EGLenum apiret = 0;
    apiret = eglQueryAPI ();
    
    printf("main() line: %d api: %d \n", __LINE__, apiret);
    
    // Step 4 - Specify the required configuration attributes.
    EGLint pi32ConfigAttribs[5];
    pi32ConfigAttribs[0] = EGL_SURFACE_TYPE;
    pi32ConfigAttribs[1] = EGL_WINDOW_BIT;
    pi32ConfigAttribs[2] = EGL_RENDERABLE_TYPE;
    pi32ConfigAttribs[3] = EGL_OPENGL_ES2_BIT;
    pi32ConfigAttribs[4] = EGL_NONE;

    EGLint egl_config_attr[] = {
        EGL_BUFFER_SIZE,    16,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,   0,
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_NONE
    };
    
    GLuint fboId = 0;
    GLuint renderBufferWidth = 1280;
    GLuint renderBufferHeight = 720;
    
    // Step 5 - Find a config that matches all requirements.
    int iConfigs = 0;
    EGLConfig eglConfig;
    eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &iConfigs);
    printf("main() line: %d \n", __LINE__);

    if (iConfigs != 1) {
        printf("Error: eglChooseConfig(): config not found iConfigs: %d \n", iConfigs);
        //exit(-1);
    }

    // Step 6 - Create a surface to draw to.
    EGLClientBuffer cbuffer=0;
    EGLSurface eglSurface=0;
    cbuffer = malloc(renderBufferWidth*renderBufferHeight*3);

    printf("main() line: %d \n", __LINE__);
    
    //eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)NULL, NULL);
    eglSurface = eglCreatePbufferFromClientBuffer(eglDisplay, EGL_RENDER_BUFFER, cbuffer, eglConfig, 0);
    
    printf("main() line: %d cbuffer: 0x%.8x eglSurface: 0x%.8x \n", __LINE__, cbuffer, (unsigned int)eglSurface);
    
    // Step 7 - Create a context.
    EGLContext eglContext=0;
    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, ai32ContextAttribs);
    
    printf("main() line: %d eglContext: 0x%.8x \n", __LINE__, (unsigned int)eglContext);
    
    // Step 8 - Bind the context to the current thread
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    printf("main() line: %d \n", __LINE__);
    
    // create a framebuffer object
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);

    // create a texture object
    GLuint textureId=0;
     glGenTextures(1, &textureId);
     
     printf("main() line: %d textureId: %d \n", __LINE__, textureId);
     
     glBindTexture(GL_TEXTURE_2D, textureId);
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                             
     //GL_LINEAR_MIPMAP_LINEAR
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_HINT, GL_TRUE); // automatic mipmap
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderBufferWidth, renderBufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
     glBindTexture(GL_TEXTURE_2D, 0);
     // attach the texture to FBO color attachment point
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
     //qDebug() << glGetError();
     GLuint renderBuffer=0;
     glGenRenderbuffers(1, &renderBuffer);

     printf("main() line: %d renderBuffer: %d \n", __LINE__, renderBuffer);
     
     glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
     //qDebug() << glGetError();
     glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, renderBufferWidth, renderBufferHeight);
     //qDebug() << glGetError();
     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBuffer);

      printf("main() line: %d \n", __LINE__);
      
      //qDebug() << glGetError();
      GLuint depthRenderbuffer=0;
      glGenRenderbuffers(1, &depthRenderbuffer);
      
     printf("main() line: %d depthRenderbuffer: %d \n", __LINE__, depthRenderbuffer);
     
      glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,  renderBufferWidth, renderBufferHeight);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

      // check FBO status
      GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if(status != GL_FRAMEBUFFER_COMPLETE) {
          printf("Problem with OpenGL framebuffer after specifying color render buffer: \n%x\n", status);
      } else {
          printf("FBO creation succedded\n");
      }

   printf("main() line: %d \n", __LINE__);
   
  glClearColor(1.0,0.0,0.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  //qDebug() << eglSwapBuffers(   eglDisplay, eglSurface);
  int size = 4 * renderBufferHeight * renderBufferWidth;
  printf("print size");
  printf("size %d", size);
  //qDebug() << size;

  unsigned char *data2 = (unsigned char *) malloc(size);

  printf("allocate memory size: %d addr: 0x%.8x \n", size, (unsigned int)data2);

  glReadPixels(0,0,renderBufferWidth,renderBufferHeight,GL_RGB, GL_UNSIGNED_BYTE, data2);

  grapglbmp(data2, renderBufferWidth, renderBufferHeight, 24, 3 * renderBufferHeight * renderBufferWidth);
  //shmem_dump(data2, size);

  //QImage image(data2, renderBufferWidth,  renderBufferHeight,renderBufferWidth*2, QImage::Format_RGB16);
  //image.save("result.png");
  //qDebug() << "done";
  //QCoreApplication a(argc, argv);

  return 0;
}
#endif

