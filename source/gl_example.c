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
#include <unistd.h>
#include <time.h>
#define GL_GLEXT_PROTOTYPES 1
//#include <GLES/gl.h>
//#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
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

#define WINDOW_WIDTH (1280 / 2)
#define WINDOW_HEIGHT (720 / 2)

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

extern int dbgBitmapHeader(struct bitmapHeader_s *ph, int len);

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
   EGLint ret=0;
   EGLint numConfigs=0;
   EGLint majorVersion=0;
   EGLint minorVersion=0;
   EGLContext context=0;
   EGLSurface surface=0;
   EGLConfig *config=0;
   EGLint cofigsize=0;
   #if 1
   EGLint fbAttribs[] =
   {
       EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
       EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
       EGL_RED_SIZE,        8,
       EGL_GREEN_SIZE,      8,
       EGL_BLUE_SIZE,       8,
       EGL_NONE
   };
   #else
   static const EGLint fbAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
    };
    #endif
    
   //EGLConfig fbConfigs[30] = {0};
   //config = fbConfigs;
   
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
   EGLDisplay display = eglGetDisplay(ESContext.native_display);
   
   if (display == EGL_NO_DISPLAY) {
      LOG("No EGL Display...ret: %d\n", ret);
      return EGL_FALSE;
   }

   // Initialize EGL
   ret = eglInitialize(display, &majorVersion, &minorVersion); 
   if (!ret) {
      LOG("eglInitialize failed...ret: %d\n", ret);
      return EGL_FALSE;
   } 

   LOG("EGL Version \"%s\"\n", eglQueryString(display, EGL_VERSION));
   LOG("EGL Vendor \"%s\"\n", eglQueryString(display, EGL_VENDOR));
   LOG("EGL Extensions \"%s\"\n", eglQueryString(display, EGL_EXTENSIONS));

   ret = eglBindAPI(EGL_OPENGL_ES_API);
   if (!ret) {
       LOG("failed to bind api EGL_OPENGL_ES_API ret: %d\n", ret);
       return EGL_FALSE;
   }
	
   // Get configs
   ret = eglGetConfigs(display, NULL, 0, &numConfigs);
   if ((ret != EGL_TRUE) || (numConfigs == 0)) {
      LOG("eglGetConfigs failed...ret: %d\n", ret);
      return EGL_FALSE;
   }

   LOG("eglGetConfigs numConfigs: %d, ret: %d\n", numConfigs, ret);
   
   ret = eglChooseConfig(display, fbAttribs, 0, 0, &numConfigs);
   if ( (ret != EGL_TRUE) || (numConfigs == 0))
   {
      LOG("eglChooseConfig get failed...ret: %d, numConfigs: %d, \n", ret, numConfigs);
      return EGL_FALSE;
   }
   
   LOG("eglChooseConfig get numConfigs: %d, config: 0x%.8x, ret: %d\n", numConfigs, (uint32_t)config, ret);
   cofigsize = numConfigs;
   config = malloc(sizeof(EGLConfig) * cofigsize);
   // Choose config
   ret = eglChooseConfig(display, fbAttribs, config, cofigsize, &numConfigs);
   if ( (ret != EGL_TRUE) || (numConfigs == 0) || (config == 0))
   {
      LOG("eglChooseConfig set failed...ret: %d, numConfigs: %d, config: 0x%.8x\n", ret, numConfigs, (uint32_t)config);
      return EGL_FALSE;
   }
   LOG("eglChooseConfig set numConfigs: %d, config: 0x%.8x, ret: %d\n", numConfigs, (uint32_t)config, ret);

   /*
   for(int ix=0; ix < numConfigs; ix++) {       
      LOG("[%d] 0x%.8x \n", ix, config[ix]);
   }
   */
   
   // Create a surface
   surface = eglCreateWindowSurface(display, *config, ESContext.native_window, NULL);
   if (surface == EGL_NO_SURFACE)
   {
      LOG("eglCreateWindowSurface failed...ret: 0x%x\n", (uint32_t)surface);
      return EGL_FALSE;
   }

   // Create a GL context
   context = eglCreateContext(display, *config, EGL_NO_CONTEXT, contextAttribs);
   if (context == EGL_NO_CONTEXT)
   {
      LOG("eglCreateContext failed...ret: %d\n", ret);
      return EGL_FALSE;
   }

   // Make the context current
   ret = eglMakeCurrent(display, surface, surface, context);
   if (!ret)
   {
      LOG("Could not make the current window current !ret: %d\n", ret);
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

    return 0;
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

static int gleGetBmpFile(char **retbuf, char **retraw, FILE *f)
{
    int ret=0, size=0, err=0, offset=0;
    char *buff=0, *rawbuf=0;
    struct bitmapHeader_s *head=0;
    
    if (!f) {
        err = -1;
        goto end;
    }

    ret |= fseek(f, 0, SEEK_END);

    size = ftell(f);

    ret |= fseek(f, 0, SEEK_SET);

    if (ret) {
        err = -2;
        goto end;
    }

    //size = sizeof(struct bitmapHeader_s);
    head = (struct bitmapHeader_s *)malloc(size + 4); 
    if (!head) {
        err = -3;
        goto end;
    }

    //size -= 2;
    buff = &head->aspbmpMagic[2];
    ret = fread(buff, 1, size, f);
    if ((ret < 0) || (ret != size)) {
        err = -4;
        goto end;
    }

    dbgBitmapHeader(head, sizeof(struct bitmapHeader_s) - 2);

    size = head->aspbiRawSize;
    rawbuf = malloc(size);

    offset = head->aspbhRawoffset;

    #if 1
    fseek(f, offset, SEEK_SET);
    #else
    fseek(f, 0, SEEK_SET);

    ret = fread(rawbuf, 1, offset, f);
    if ((ret < 0) || (ret != offset)) {
        err = -5;
        goto end;
    }
    #endif
    
    ret = fread(rawbuf, 1, size, f);
    if ((ret < 0) || (ret != size)) {
        err = -6;
        goto end;
    }
    

    end:

    if (err) {
        if (head) free(head);
        if (rawbuf) free(rawbuf);
        
        head = 0;
        rawbuf = 0;
    }

    *retbuf = (char *)head;
    *retraw = rawbuf;
   
    return err;
}

static int gleGetImage(char **dat, char **raw, int idx)
{
    FILE *f;
    char filepath[256];
    char bnotebmp[128]="/home/root/banknote/full_H%.3d.bmp";
    //char bnotebmp[128]="/home/root/banknote/char/%d.bmp";
    int ret=0;

    sprintf(filepath, bnotebmp, idx);
    f = fopen(filepath, "r");

    if (!f) {
        printf("get file [%s] failed!! \n", filepath);
        return -1;
    }

    ret = gleGetBmpFile(dat, raw, f);

    fclose(f);

    return ret;
}

#if 0
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
#endif

#if 1
    const GLchar* vertexShaderCode = {
            "precision mediump float;\n"
            "attribute vec4 a_position;\n"
            "attribute vec2 a_textureCoordinate;    \n"
            "varying vec2 v_textureCoordinate;     \n"
            "uniform vec2 u_Translate;\n" 
            "uniform float u_Scale;\n" 
            "uniform float u_Rotate;\n" 
            "uniform float u_Ratio;\n" 
            "\n" 
            "void main() {\n" 
            "   v_textureCoordinate = a_textureCoordinate;    \n"
            "   vec4 p = a_position;\n" 
            "   p.y = p.y / u_Ratio;\n" 
            "   mat4 translateMatrix = mat4(1.0, 0.0, 0.0, 0.0,\n" 
            "                              0.0, 1.0, 0.0, 0.0,\n" 
            "                              0.0, 0.0, 1.0, 0.0,\n" 
            "                              u_Translate.x, u_Translate.y, 0.0, 1.0);\n" 
            "   mat4 scaleMatrix = mat4(u_Scale, 0.0, 0.0, 0.0,\n" 
            "                        0.0, u_Scale, 0.0, 0.0,\n" 
            "                        0.0, 0.0, 1.0, 0.0,\n" 
            "                        0.0, 0.0, 0.0, 1.0);\n" 
            "   mat4 rotateMatrix = mat4(cos(u_Rotate), sin(u_Rotate), 0.0, 0.0,\n" 
            "                         -sin(u_Rotate), cos(u_Rotate), 0.0, 0.0,\n" 
            "                         0.0, 0.0, 1.0, 0.0,\n" 
            "                         0.0, 0.0, 0.0, 1.0);\n" 
            "    p = translateMatrix * rotateMatrix * scaleMatrix * p;\n" 
            "    p.y = p.y * u_Ratio;\n" 
            "    gl_Position = p;  \n"
            "}                                \n"};

    const GLchar* fragmentShaderCode = {
                "precision mediump float;\n"
                "varying vec2 v_textureCoordinate;      \n"
                "uniform sampler2D u_texture;             \n"
                "void main() {                                      \n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    vec4 swap; \n"
                "    swap.r = color.r; \n"
                "    swap.g = color.g; \n"
                "    swap.b = color.b; \n"
                "    gl_FragColor = swap;        \n" // texture2D(u_texture, v_textureCoordinate);\n
                "}                                \n"};
#elif 1 /* draw bmp */
    const GLchar* vertexShaderCode = {
                "precision mediump float;\n"
                "attribute vec4 a_position;                  \n"
                "attribute vec2 a_textureCoordinate;    \n"
                "varying vec2 v_textureCoordinate;     \n"
                "void main() {                                      \n"
                "    v_textureCoordinate = a_textureCoordinate;    \n"
                "    gl_Position = a_position;           \n"
                "}                 \n"};

    const GLchar* fragmentShaderCode = {
                "precision mediump float;\n"
                "varying vec2 v_textureCoordinate;      \n"
                "uniform sampler2D u_texture;             \n"
                "void main() {                                      \n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    vec4 swap = color;    \n"
                "    swap.r = color.b;       \n"
                "    swap.b = color.r;       \n"
                "    gl_FragColor = swap;        \n" // texture2D(u_texture, v_textureCoordinate);\n
                "}                                \n"};
#elif 1 /*ex3  + vec4 (0.0, 0.2, 0.0, 0.0); */
const GLchar* vertexSource =
    "attribute vec4 position;    \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = vec4(position.xyz, 1.0) + vec4 (0.0, 0.0, 0.0, 0.0);  \n"
    "}                            \n";
const GLchar* fragmentSource =
    "precision mediump float;\n"
    "void main()                                  \n"
    "{                                            \n"
    "  gl_FragColor = vec4 (0.0, 1.0, 0.0, 1.0 );\n"
    "}                                            \n";
#else /*ex1*/
const GLchar* vertexSource =
    "attribute vec4 position;    \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = vec4(position.xyz, 1.0);  \n"
    "}                            \n";
const GLchar* fragmentSource =
    "precision mediump float;\n"
    "void main()                                  \n"
    "{                                            \n"
    "  gl_FragColor = vec4 (0.0, 0.0, 1.0, 1.0 );\n"
    "}                                            \n";
#endif

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

#if 1
int main(int argc, char *argv[])
{
    #define CONTEXT_ES20
    EGLBoolean eglret = 0;
    int screenwidth  = 800, screenheight = 480;
    int ret=0, imgidx=0;
    char *img=0, *raw=0;
    struct bitmapHeader_s *header=0;
    
    printf("main() argc: %d \n", argc);

    if (argc == 2) {
        imgidx = atoi(argv[1]);
        
        printf("main() argv[1]: %d (%s) \n", imgidx, argv[1]);
    } else {
        imgidx = 23;
    }

    ret = gleGetImage(&img, &raw, imgidx);

    if (ret) {
        printf("main() Error!!! can't load bmp ret: %d\n", ret);
        return -1;
    }

    shmem_dump(img, 32);
    shmem_dump(raw, 32);
    
    //raw = img + 1078;
    header = (struct bitmapHeader_s *)img;
    dbgBitmapHeader(header, sizeof(struct bitmapHeader_s) - 2);

    screenwidth = header->aspbiWidth / 2;
    screenheight = abs(header->aspbiHeight) / 2;
    
    get_server_references();

    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        LOG("No Compositor surface ! Yay....\n");
        exit(1);
    }
    else LOG("Got a compositor surface !\n");

    shell_surface = wl_shell_get_shell_surface(shell, surface);
    wl_shell_surface_set_toplevel(shell_surface);

    eglret = CreateWindowWithEGLContext("Nya", screenwidth, screenheight);
    
    printf("main() line: %d CreateWindowWithEGLContext ret: 0x%.8x\n", __LINE__, (unsigned int)eglret);
  
    // Step 1 - Get the default display.
    EGLDisplay eglDisplay =0;
    //eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglDisplay = ESContext.display;

    printf("main() line: %d eglDisplay: 0x%.8x\n", __LINE__, (unsigned int)eglDisplay);

    // Step 2 - Initialize EGL.
    //EGLint majorVersion=0, minorVersion=0;
    //eglret = eglInitialize(eglDisplay, &majorVersion, &minorVersion);
    //printf("main() line: %d, minorVersion %d, minorVersion %d eglret: %d \n", __LINE__, majorVersion, minorVersion, eglret);
    
    // Step 3 - Make OpenGL ES the current API.

    //printf("main() line: %d \n", __LINE__);
    
    //eglBindAPI(EGL_OPENGL_ES_API);
    //eglInitialize(EGLDisplay dpy, EGLint * major, EGLint * minor)
    
    //EGLenum apiret = 0;
    //apiret = eglQueryAPI ();
    //printf("main() line: %d api: %d \n", __LINE__, apiret);
    
    // Step 4 - Specify the required configuration attributes.
    /*
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
    */
    
    GLuint fboId = 0;
    GLuint renderBufferWidth = screenwidth;
    GLuint renderBufferHeight = screenheight;
    
    // Step 5 - Find a config that matches all requirements.

    // Step 6 - Create a surface to draw to.
    EGLClientBuffer cbuffer=0;
    EGLSurface eglSurface=0;
    //cbuffer = malloc(renderBufferWidth*renderBufferHeight*3);

    printf("main() line: %d \n", __LINE__);
    
    eglSurface = ESContext.surface;
    
    //printf("main() line: %d cbuffer: 0x%.8x eglSurface: 0x%.8x \n", __LINE__, (uint32_t)cbuffer, (unsigned int)eglSurface);
    
    // Step 7 - Create a context.
    EGLContext eglContext=0;
    eglContext = ESContext.context;
    
    printf("main() line: %d eglContext: 0x%.8x \n", __LINE__, (unsigned int)eglContext);
    
    // Step 8 - Bind the context to the current thread
    //eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

   printf("main() line: %d \n", __LINE__);

    #if 1
    GLfloat vertexData[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    GLfloat textureCoordinateData[] = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
    GLuint shaderProgram = glCreateProgram();
    GLint   compileStatus;    
    
    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileStatus);
    printf("main() line: %d compile vertex shader status: %d \n", __LINE__, compileStatus); 
    if(compileStatus == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(vertexShader, sizeof(messages), 0,messages);
        printf("main() line: %d compile log: %s \n", __LINE__, messages);
    }
   
    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);
    printf("main() line: %d compile fragment shader status: %d \n", __LINE__, compileStatus); 
    if(compileStatus == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(fragmentShader, sizeof(messages), 0,messages);
        printf("main() line: %d compile log: %s \n", __LINE__, messages);
    }
    
   printf("main() line: %d vshader: 0x%.8x, fshader: 0x%.8x, programid: 0x%.8x\n", __LINE__, vertexShader, fragmentShader, shaderProgram);

    // Link the vertex and fragment shader into a shader program
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    // glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);

    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    printf("main() line: %d link program status: %d \n", __LINE__, linkStatus); 
    if (linkStatus == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog( shaderProgram, sizeof(messages), 0, messages);
        printf("main() line: %d link error log: %s \n", __LINE__, messages);
    }
            
    glUseProgram(shaderProgram);
    
    // Get the location of a_position in the shader
    //GLint aLocation = glGetAttribLocation(shaderProgram, "position");
    //GLint asLocation = glGetAttribLocation(shaderProgram, "s_position");
    //printf("main() line: %d location vertex test: %d asLocation: %d \n", __LINE__, aLocation, asLocation);
   
    GLint aPositionLocation = glGetAttribLocation(shaderProgram, "a_position");
    // Enable the parameter of the location
    glEnableVertexAttribArray(aPositionLocation);
    // Specify the data of a_position
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, false,0, vertexData);

    // Get the location of a_textureCoordinate in the shader
    GLint aTextureCoordinateLocation = glGetAttribLocation(shaderProgram, "a_textureCoordinate");
    // Enable the parameter of the location
    glEnableVertexAttribArray(aTextureCoordinateLocation);
    // Specify the data of a_textureCoordinate
    glVertexAttribPointer(aTextureCoordinateLocation, 2, GL_FLOAT, false,0, textureCoordinateData);
    
    printf("main() line: %d aPositionLocation: %d aTextureCoordinateLocation: %d \n", __LINE__, aPositionLocation, aTextureCoordinateLocation);
    
    // Get the location of translate in the shader
    GLint uTranslateLocation = glGetUniformLocation(shaderProgram, "u_Translate");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uTranslateLocation);
    // Specify the vertex data of translate
    glUniform2f(uTranslateLocation, 0.0f, 0.0f);

    // Get the location of u_Scale in the shader
    GLint uScaleLocation = glGetUniformLocation(shaderProgram, "u_Scale");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uScaleLocation);
    // Specify the vertex data of u_Scale
    glUniform1f(uScaleLocation, 1.0f);

    printf("main() line: %d uTranslateLocation: %d uScaleLocation: %d \n", __LINE__, uTranslateLocation, uScaleLocation);
    
    // Get the location of u_Rotate in the shader
    GLint uRotateLocation = glGetUniformLocation(shaderProgram, "u_Rotate");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRotateLocation);
    // Specify the vertex data of u_Rotate
    glUniform1f(uRotateLocation, (10.0f * 3.1415f) / 180.0f);

    // Get the location of u_Rotate in the shader
    GLint uRatioLocation = glGetUniformLocation(shaderProgram, "u_Ratio");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRatioLocation);
    // Specify the vertex data of u_Ratio
    glUniform1f(uRatioLocation, screenwidth * 1.0f / screenheight);

    printf("main() line: %d uRotateLocation: %d uRatioLocation: %d \n", __LINE__, uRotateLocation, uRatioLocation);

    #if 1
    // Create texture
    GLint textures[0];
    glGenTextures(1, textures);
    GLint imageTexture = textures[0];

    // Set texture parameters
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Decode the image and load it into texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_RGB, GL_UNSIGNED_BYTE, raw);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8_ALPHA8_EXT, header->aspbiWidth, header->aspbiHeight, 0, GL_LUMINANCE8_ALPHA8_EXT, GL_UNSIGNED_BYTE, raw);
    //GLint uTextureLocation = glGetAttribLocation(shaderProgram, "u_texture");
    //glActiveTexture(GL_TEXTURE0);
    //glUniform1i(uTextureLocation, 0);
    #endif

    //printf("main() line: %d textureid: %d, texturelocation: %d \n", __LINE__, imageTexture, uTextureLocation);
   
    #elif 1
    // Create Vertex Array Object
    GLuint vao;
    //glGenVertexArraysOES(1, &vao);
    //glBindVertexArrayOES(vao);

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    //glGenBuffers(1, &vbo);

    GLfloat vertices[] = {0.0f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f};
    //GLfloat vertices[] = {-0.5f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f};

    //glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    // glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    
    printf("main() line: %d vshader: 0x%.8x, fshader: 0x%.8x, programid: 0x%.8x\n", __LINE__, vertexShader, fragmentShader, shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);

    printf("main() line: %d location vertex: 0x%.8x \n", __LINE__, posAttrib);
    
    #endif

    while (1) {
        wl_display_dispatch_pending(ESContext.native_display);
        //draw();
        // Clear the screen to black        
        glClearColor(0.7f, 0.9f, 0.8f, 1.0f);        
        glClear(GL_COLOR_BUFFER_BIT);       

        glViewport(0, 0, renderBufferWidth, renderBufferHeight);
        
        // Draw a triangle from the 3 vertices        
        #if 1
        glDrawArrays(GL_TRIANGLES, 0, 6);
        #else
        glDrawArrays(GL_TRIANGLES, 0, 3);
        #endif
    
        RefreshWindow();
        
        #if 1
        break;
        #endif
    }

    sleep(2);
    
  //qDebug() << eglSwapBuffers(   eglDisplay, eglSurface);
  int size = 4 * renderBufferHeight * renderBufferWidth;
  printf("print size");
  printf("size %d", size);
  //qDebug() << size;

  unsigned char *data2 = (unsigned char *) malloc(size);

  printf("allocate memory size: %d addr: 0x%.8x \n", size, (unsigned int)data2);

  glReadPixels(0,0,renderBufferWidth,renderBufferHeight,GL_RGB, GL_UNSIGNED_BYTE, data2);
  //glReadPixels(0,0,renderBufferWidth,renderBufferHeight,GL_LUMINANCE8_ALPHA8_EXT, GL_UNSIGNED_BYTE, data2);

  grapglbmp(data2, renderBufferWidth, renderBufferHeight, 24, 3 * renderBufferHeight * renderBufferWidth);
  //grapglbmp(data2, renderBufferWidth, renderBufferHeight, 8, 1 * renderBufferHeight * renderBufferWidth);
  //shmem_dump(data2, size);

  //QImage image(data2, renderBufferWidth,  renderBufferHeight,renderBufferWidth*2, QImage::Format_RGB16);
  //image.save("result.png");
  //qDebug() << "done";
  //QCoreApplication a(argc, argv);

  return 0;
}
#endif

