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
#include <math.h>
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

#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)

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
       //EGL_ALPHA_SIZE,     8,
       //EGL_DEPTH_SIZE,     16,
       EGL_NONE,
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

static int asgltime_diff(struct timespec *s, struct timespec *e, int unit)
{
    unsigned long long cur, tnow, lnow, past, tbef, lpast, gunit;
    int diff;

    gunit = unit;
    //clock_gettime(CLOCK_REALTIME, &curtime);
    cur = s->tv_sec;
    tnow = s->tv_nsec;
    lnow = cur * 1000000000+tnow;
    
    //clock_gettime(CLOCK_REALTIME, &curtime);
    past = e->tv_sec;
    tbef = e->tv_nsec;      
    lpast = past * 1000000000+tbef; 

    if (lpast < lnow) {
        diff = -1;
    } else {
        diff = (lpast - lnow)/gunit;
    }

    if (diff == 0) {
        diff = 1;
    }

    return diff;
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

#define I(_i, _j) ((_j)+ 4*(_i))

static void func_multiplyMM(float* r, float* lhs, float* rhs)
{
    for (int i=0 ; i<4 ; i++) {
        const float rhs_i0 = rhs[ I(i,0) ];
        register float ri0 = lhs[ I(0,0) ] * rhs_i0;
        register float ri1 = lhs[ I(0,1) ] * rhs_i0;
        register float ri2 = lhs[ I(0,2) ] * rhs_i0;
        register float ri3 = lhs[ I(0,3) ] * rhs_i0;
        for (int j=1 ; j<4 ; j++) {
            register const float rhs_ij = rhs[ I(i,j) ];
            ri0 += lhs[ I(j,0) ] * rhs_ij;
            ri1 += lhs[ I(j,1) ] * rhs_ij;
            ri2 += lhs[ I(j,2) ] * rhs_ij;
            ri3 += lhs[ I(j,3) ] * rhs_ij;
        }
        r[ I(i,0) ] = ri0;
        r[ I(i,1) ] = ri1;
        r[ I(i,2) ] = ri2;
        r[ I(i,3) ] = ri3;
    }
}

static void multiplyMM(float* r, int roffset, float* lhs, int lhoffset, float* rhs, int rhoffset)
{
    float *rlt=0, *lhsoff=0, *rhsoff=0;

    rlt = r + roffset;
    lhsoff = lhs + lhoffset;
    rhsoff = rhs + rhoffset;

    func_multiplyMM(rlt, lhsoff, rhsoff);
}

    /**
     * Computes the length of a vector
     *
     * @param x x coordinate of a vector
     * @param y y coordinate of a vector
     * @param z z coordinate of a vector
     * @return the length of a vector
     */
    static float length(float x, float y, float z) {
        return (float) sqrt(x * x + y * y + z * z);
    }
    
    /**
     * Translates matrix m by x, y, and z in place.
     * @param m matrix
     * @param mOffset index into m where the matrix starts
     * @param x translation factor x
     * @param y translation factor y
     * @param z translation factor z
     */
    static void translateM(float *m, int mOffset, float x, float y, float z) 
    {
        for (int i=0 ; i<4 ; i++) {
            int mi = mOffset + i;
            m[12 + mi] += m[mi] * x + m[4 + mi] * y + m[8 + mi] * z;
        }
    }


    /**
     * Define a projection matrix in terms of six clip planes
     * @param m the float array that holds the perspective matrix
     * @param offset the offset into float array m where the perspective
     * matrix data is written
     * @param left
     * @param right
     * @param bottom
     * @param top
     * @param near
     * @param far
     */
    static int frustumM(float *m, int offset, float left, float right, float bottom, float top, float near, float far) {
        if (left == right) {
            return -1;
        }
        if (top == bottom) {
            return -2;
        }
        if (near == far) {
            return -3;
        }
        if (near <= 0.0f) {
            return -4;
        }
        if (far <= 0.0f) {
            return -5;
        }
        
        const float r_width  = 1.0f / (right - left);
        const float r_height = 1.0f / (top - bottom);
        const float r_depth  = 1.0f / (near - far);
        const float x = 2.0f * (near * r_width);
        const float y = 2.0f * (near * r_height);
        const float A = 2.0f * ((right + left) * r_width);
        const float B = (top + bottom) * r_height;
        const float C = (far + near) * r_depth;
        const float D = 2.0f * (far * near * r_depth);
        m[offset + 0] = x;
        m[offset + 5] = y;
        m[offset + 8] = A;
        m[offset +  9] = B;
        m[offset + 10] = C;
        m[offset + 14] = D;
        m[offset + 11] = -1.0f;
        m[offset +  1] = 0.0f;
        m[offset +  2] = 0.0f;
        m[offset +  3] = 0.0f;
        m[offset +  4] = 0.0f;
        m[offset +  6] = 0.0f;
        m[offset +  7] = 0.0f;
        m[offset + 12] = 0.0f;
        m[offset + 13] = 0.0f;
        m[offset + 15] = 0.0f;

        return 0;
    }

    /**
     * Define a viewing transformation in terms of an eye point, a center of
     * view, and an up vector.
     *
     * @param rm returns the result
     * @param rmOffset index into rm where the result matrix starts
     * @param eyeX eye point X
     * @param eyeY eye point Y
     * @param eyeZ eye point Z
     * @param centerX center of view X
     * @param centerY center of view Y
     * @param centerZ center of view Z
     * @param upX up vector X
     * @param upY up vector Y
     * @param upZ up vector Z
     */
    static void setLookAtM(float *rm, int rmOffset,
            float eyeX, float eyeY, float eyeZ,
            float centerX, float centerY, float centerZ, 
            float upX, float upY, float upZ) {
        // See the OpenGL GLUT documentation for gluLookAt for a description
        // of the algorithm. We implement it in a straightforward way:
        float fx = centerX - eyeX;
        float fy = centerY - eyeY;
        float fz = centerZ - eyeZ;
        // Normalize f
        float rlf = 1.0f / length(fx, fy, fz);
        fx *= rlf;
        fy *= rlf;
        fz *= rlf;
        // compute s = f x up (x means "cross product")
        float sx = fy * upZ - fz * upY;
        float sy = fz * upX - fx * upZ;
        float sz = fx * upY - fy * upX;
        // and normalize s
        float rls = 1.0f / length(sx, sy, sz);
        sx *= rls;
        sy *= rls;
        sz *= rls;
        // compute u = s x f
        float ux = sy * fz - sz * fy;
        float uy = sz * fx - sx * fz;
        float uz = sx * fy - sy * fx;
        rm[rmOffset + 0] = sx;
        rm[rmOffset + 1] = ux;
        rm[rmOffset + 2] = -fx;
        rm[rmOffset + 3] = 0.0f;
        rm[rmOffset + 4] = sy;
        rm[rmOffset + 5] = uy;
        rm[rmOffset + 6] = -fy;
        rm[rmOffset + 7] = 0.0f;
        rm[rmOffset + 8] = sz;
        rm[rmOffset + 9] = uz;
        rm[rmOffset + 10] = -fz;
        rm[rmOffset + 11] = 0.0f;
        rm[rmOffset + 12] = 0.0f;
        rm[rmOffset + 13] = 0.0f;
        rm[rmOffset + 14] = 0.0f;
        rm[rmOffset + 15] = 1.0f;
        translateM(rm, rmOffset, -eyeX, -eyeY, -eyeZ);
    }
    
    /**
     * Rotates matrix m by angle a (in degrees) around the axis (x, y, z)
     * @param rm returns the result
     * @param rmOffset index into rm where the result matrix starts
     * @param a angle to rotate in degrees
     * @param x scale factor x
     * @param y scale factor y
     * @param z scale factor z
     */
    static void setRotateM(float *rm, int rmOffset, float a, float x, float y, float z) 
    {
        rm[rmOffset + 3] = 0;
        rm[rmOffset + 7] = 0;
        rm[rmOffset + 11]= 0;
        rm[rmOffset + 12]= 0;
        rm[rmOffset + 13]= 0;
        rm[rmOffset + 14]= 0;
        rm[rmOffset + 15]= 1;
        a *= (float) (M_PI / 180.0f);
        float s = (float) sin(a);
        float c = (float) cos(a);
        if (1.0f == x && 0.0f == y && 0.0f == z) {
            rm[rmOffset + 5] = c;   rm[rmOffset + 10]= c;
            rm[rmOffset + 6] = s;   rm[rmOffset + 9] = -s;
            rm[rmOffset + 1] = 0;   rm[rmOffset + 2] = 0;
            rm[rmOffset + 4] = 0;   rm[rmOffset + 8] = 0;
            rm[rmOffset + 0] = 1;
        } else if (0.0f == x && 1.0f == y && 0.0f == z) {
            rm[rmOffset + 0] = c;   rm[rmOffset + 10]= c;
            rm[rmOffset + 8] = s;   rm[rmOffset + 2] = -s;
            rm[rmOffset + 1] = 0;   rm[rmOffset + 4] = 0;
            rm[rmOffset + 6] = 0;   rm[rmOffset + 9] = 0;
            rm[rmOffset + 5] = 1;
        } else if (0.0f == x && 0.0f == y && 1.0f == z) {
            rm[rmOffset + 0] = c;   rm[rmOffset + 5] = c;
            rm[rmOffset + 1] = s;   rm[rmOffset + 4] = -s;
            rm[rmOffset + 2] = 0;   rm[rmOffset + 6] = 0;
            rm[rmOffset + 8] = 0;   rm[rmOffset + 9] = 0;
            rm[rmOffset + 10]= 1;
        } else {
            float len = length(x, y, z);
            if (1.0f != len) {
                float recipLen = 1.0f / len;
                x *= recipLen;
                y *= recipLen;
                z *= recipLen;
            }
            float nc = 1.0f - c;
            float xy = x * y;
            float yz = y * z;
            float zx = z * x;
            float xs = x * s;
            float ys = y * s;
            float zs = z * s;
            rm[rmOffset +  0] = x*x*nc +  c;
            rm[rmOffset +  4] =  xy*nc - zs;
            rm[rmOffset +  8] =  zx*nc + ys;
            rm[rmOffset +  1] =  xy*nc + zs;
            rm[rmOffset +  5] = y*y*nc +  c;
            rm[rmOffset +  9] =  yz*nc - xs;
            rm[rmOffset +  2] =  zx*nc - ys;
            rm[rmOffset +  6] =  yz*nc + xs;
            rm[rmOffset + 10] = z*z*nc +  c;
        }
    }

    /**
     * Scales matrix m in place by sx, sy, and sz
     * @param m matrix to scale
     * @param mOffset index into m where the matrix starts
     * @param x scale factor x
     * @param y scale factor y
     * @param z scale factor z
     */
    static void scaleM(float *m, int mOffset, float x, float y, float z) {
        for (int i=0 ; i<4 ; i++) {
            int mi = mOffset + i;
            m[     mi] *= x;
            m[ 4 + mi] *= y;
            m[ 8 + mi] *= z;
        }
    }
    
    /**
     * Rotates matrix m in place by angle a (in degrees)
     * around the axis (x, y, z)
     * @param m source matrix
     * @param mOffset index into m where the matrix starts
     * @param a angle to rotate in degrees
     * @param x scale factor x
     * @param y scale factor y
     * @param z scale factor z
     */
    static void rotateM(float *m, int mOffset, float a, float x, float y, float z) {
        float temp[32]={0};
        float *shft=0, *srcshf=0;
        setRotateM(temp, 0, a, x, y, z);
        multiplyMM(temp, 16, m, mOffset, temp, 0);

        srcshf = &temp[16];
        shft= &m[mOffset];

        memcpy(shft, srcshf, sizeof(float) * 16);
        //System.arraycopy(temp, 16, m, mOffset, 16);
    }
    
    static GLfloat* getIdentity(GLfloat * p) 
    {
    #define ID_ARRAY_SIZE  (16)
        GLfloat *af=0;
        GLfloat patt[ID_ARRAY_SIZE] = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
        
        if (p) {
            af = p;
        } else {
            af = malloc(sizeof(GLfloat) * ID_ARRAY_SIZE);
        }

        if (!af) return 0;

        memcpy(af, patt, sizeof(GLfloat) * ID_ARRAY_SIZE);

        return af;
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

#if 1/* franebuffer rotating */
    const GLchar* vertexShaderCode = {
            "precision mediump float;\n"
            "attribute vec4 a_position;\n"
            "attribute vec2 a_textureCoordinate;    \n"
            "varying vec2 v_textureCoordinate;     \n"
            "uniform float u_Rotate;\n" 
            "uniform float u_Ratio;\n" 
            "\n" 
            "void main() {\n" 
            "   v_textureCoordinate = a_textureCoordinate;    \n"
            "   vec4 p = a_position;\n" 
            "   p.y = p.y / u_Ratio;\n" 
            "   mat4 rotateMatrix = mat4(cos(u_Rotate), sin(u_Rotate), 0.0, 0.0,\n" 
            "                         -sin(u_Rotate), cos(u_Rotate), 0.0, 0.0,\n" 
            "                         0.0, 0.0, 1.0, 0.0,\n" 
            "                         0.0, 0.0, 0.0, 1.0);\n" 
            "    p = rotateMatrix * p;\n" 
            "    p.y = p.y * u_Ratio;\n" 
            "    gl_Position = p;  \n"
            "}                                \n"};

    const GLchar* fragmentShaderCode = {
            "precision mediump float;\n"
            "uniform sampler2D u_texture;\n"
            "varying vec2 v_textureCoordinate;\n"
            "void main() {\n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    float swap = color.r; \n"
                "    color.r = color.b; \n"
                "    color.b = swap;   \n"
                "    gl_FragColor = color;        \n" // texture2D(u_texture, v_textureCoordinate);\n
            "}      \n"};

    const GLchar* fragmentShaderBackCode = {
            "precision mediump float;\n"
            "uniform sampler2D u_texture;\n"
            "varying vec2 v_textureCoordinate;\n"
            "void main() {\n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    float swap = color.r; \n"
                "    color.r = color.b; \n"
                "    color.b = swap;   \n"
                "    gl_FragColor = color;        \n" // texture2D(u_texture, v_textureCoordinate);\n
            "}      \n"};
#elif 0 /* rotate bmp */
    const GLchar* vertexShaderCode = {
            "precision mediump float;   \n"
            "attribute vec4 a_position;   \n"
            "attribute vec2 a_textureCoordinate;  \n"
            "uniform mat4 u_mvp;  \n"
            "uniform float u_Ratio;\n" 
            "varying vec2 v_textureCoordinate;\n"
            "void main() {  \n"
            "    v_textureCoordinate = a_textureCoordinate;  \n"
            "    vec4 p = a_position;\n" 
            "    p.y = p.y / u_Ratio;\n" 
            "    p = u_mvp * p;\n" 
            "    p.y = p.y * u_Ratio;\n" 
            "    gl_Position = p;  \n"
            "}        \n"};

    const GLchar* fragmentShaderCode = {
            "precision mediump float;\n"
            "uniform sampler2D u_texture;\n"
            "varying vec2 v_textureCoordinate;\n"
            "void main() {\n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    float swap = color.r; \n"
                "    color.r = color.b; \n"
                "    color.b = swap;   \n"
                "    gl_FragColor = color;        \n" // texture2D(u_texture, v_textureCoordinate);\n
            "}      \n"};
#elif 1 /* rotate bmp */
    const GLchar* vertexShaderCode = {
            "precision mediump float;\n"
            "attribute vec4 a_position;\n"
            "attribute vec2 a_textureCoordinate;    \n"
            "varying vec2 v_textureCoordinate;     \n"
            "uniform float u_Rotate;\n" 
            "uniform float u_Ratio;\n" 
            "\n" 
            "void main() {\n" 
            "   v_textureCoordinate = a_textureCoordinate;    \n"
            "   vec4 p = a_position;\n" 
            "   p.y = p.y / u_Ratio;\n" 
            "   mat4 rotateMatrix = mat4(cos(u_Rotate), sin(u_Rotate), 0.0, 0.0,\n" 
            "                         -sin(u_Rotate), cos(u_Rotate), 0.0, 0.0,\n" 
            "                         0.0, 0.0, 1.0, 0.0,\n" 
            "                         0.0, 0.0, 0.0, 1.0);\n" 
            "    p = rotateMatrix * p;\n" 
            "    p.y = p.y * u_Ratio;\n" 
            "    gl_Position = p;  \n"
            "}                                \n"};

    const GLchar* fragmentShaderCode = {
                "precision mediump float;\n"
                "varying vec2 v_textureCoordinate;      \n"
                "uniform sampler2D u_texture;             \n"
                "void main() {                                      \n"
                "    vec4 color = texture2D(u_texture, v_textureCoordinate);  \n"
                "    float swap = color.r; \n"
                "    color.r = 0.0; \n"
                "    color.b = 0.0;   \n"
                "    color.a = 0.0;   \n"
                "    gl_FragColor = color;        \n" // texture2D(u_texture, v_textureCoordinate);\n
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
                "    swap.a = color.r;       \n"
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

 #define VERTEX_COMPONENT_COUNT  (3)
 #define VERTEX_NUM (6 * 6)
 #define VERTEX_SIZE (VERTEX_COMPONENT_COUNT * VERTEX_NUM)

    GLfloat vertexData[VERTEX_SIZE] = {
            // front face
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            // back face
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            // Top face
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            // Bottom face
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
//            // Left face
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,
            // Right face
            1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f
    };
 GLfloat *vertexDataBuffer = vertexData;
    #define TEXTURE_COORDINATE_COMPONENT_COUNT (2)
    #define TEXTURE_COORDINATE_SIZE (VERTEX_NUM * TEXTURE_COORDINATE_COMPONENT_COUNT)
    GLfloat textureCoordinateData[TEXTURE_COORDINATE_SIZE] = {
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
            
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
    };
    
    GLfloat *textureCoordinateDataBuffer = textureCoordinateData;

 float cubePositions[] = {
            -1.0f,1.0f,1.0f,    //正面左上0
            -1.0f,-1.0f,1.0f,   //正面左下1
            1.0f,-1.0f,1.0f,    //正面右下2
            1.0f,1.0f,1.0f,     //正面右上3
            -1.0f,1.0f,-1.0f,    //反面左上4
            -1.0f,-1.0f,-1.0f,   //反面左下5
            1.0f,-1.0f,-1.0f,    //反面右下6
            1.0f,1.0f,-1.0f,     //反面右上7
    };

 float cubeCoods[] = {
            0.0f,0.0f,    //正面左上0
            0.0f,1.0f,   //正面左下1
            1.0f,1.0f,    //正面右下2
            1.0f,0.0f,     //正面右上3
            0.0f,0.0f,    //反面左上4
            0.0f,1.0f,   //反面左下5
            1.0f,1.0f,    //反面右下6
            1.0f,0.0f,     //反面右上7
    };

 short indexs[]={
            0,3,2,0,2,1,    //正面
            0,1,5,0,5,4,    //左面
            0,7,3,0,4,7,    //上面
            6,7,4,6,4,5,    //后面
            6,3,7,6,2,3,    //右面
            6,5,1,6,1,2     //下面
    };
    
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
    #define LOCATION_ATTRIBUTE_POSITION 0
    #define LOCATION_ATTRIBUTE_TEXTURE_COORDINATE 1
    #define LOCATION_UNIFORM_MVP 2
    #define LOCATION_UNIFORM_TEXTURE 0
    
    #define CONTEXT_ES20
    EGLBoolean eglret = 0;
    int screenwidth  = 800, screenheight = 480;
    int ret=0, imgidx=0;
    char *img=0, *raw=0;
    struct bitmapHeader_s *header=0;
    struct timespec tmS, tmE;
    int tmCost=0, meacnt=0, selectPage=0, rotArix=0;
    GLenum glerr=0;
    GLint glrformat=0, glrdtype=0;
    GLint rx=0, ry=0, rw=0, rh=0;
    GLint size=0;
    printf("main() argc: %d \n", argc);

    if (argc == 4) {
        imgidx = atoi(argv[1]);
        selectPage = atoi(argv[2]);
        rotArix = atoi(argv[3]);
        
        printf("main() argv[1]: %d (%s) \n", imgidx, argv[1]);
    } else if (argc == 3) {
        imgidx = atoi(argv[1]);
        selectPage = atoi(argv[2]);
        rotArix = 0;
        
        printf("main() argv[1]: %d (%s) \n", imgidx, argv[1]);
    } else if (argc == 2) {
        imgidx = atoi(argv[1]);
        selectPage = 0;
        rotArix = 0;
        
        printf("main() argv[1]: %d (%s) \n", imgidx, argv[1]);
    } else {
        imgidx = 24;
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


    #if 1 /* framebuff rotating */
    
    GLfloat vertexData[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    GLfloat textureCoordinateData[] = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
    GLfloat textureCoordinateData1[] = {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    GLfloat textureCoordinateData2[] = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat textureCoordinateData3[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
    
    GLint aPositionLocation = glGetAttribLocation(shaderProgram, "a_position");
    glEnableVertexAttribArray(aPositionLocation);
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, false,0, vertexData);

    GLint aTextureCoordinateLocation = glGetAttribLocation(shaderProgram, "a_textureCoordinate");
    glEnableVertexAttribArray(aTextureCoordinateLocation);
    glVertexAttribPointer(aTextureCoordinateLocation, 2, GL_FLOAT, false,0, textureCoordinateData1);
    
    printf("main() line: %d aPositionLocation: %d aTextureCoordinateLocation: %d \n", __LINE__, aPositionLocation, aTextureCoordinateLocation);
    
    GLfloat rtangle=0.0f;
    GLint uRotateLocation = glGetUniformLocation(shaderProgram, "u_Rotate");
    glEnableVertexAttribArray(uRotateLocation);
    glUniform1f(uRotateLocation, (rtangle * 3.1415f) / 180.0f);

    // Get the location of u_Rotate in the shader
    GLint uRatioLocation = glGetUniformLocation(shaderProgram, "u_Ratio");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRatioLocation);
    // Specify the vertex data of u_Ratio
    glUniform1f(uRatioLocation, screenwidth * 1.0f / screenheight);

    printf("main() line: %d uRotateLocation: %d uRatioLocation: %d \n", __LINE__, uRotateLocation, uRatioLocation);

    #if 1

    GLuint color_renderbuffer;
    glGenRenderbuffers(1, &color_renderbuffer);
    glBindRenderbuffer( GL_RENDERBUFFER, (GLuint)color_renderbuffer );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_R8_EXT, screenwidth, screenheight );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );

    // Build the framebuffer.
    GLuint framebufferR8;
    glGenFramebuffers(1, &framebufferR8);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)framebufferR8);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer);

    GLenum statusR8 = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    printf("main() line: %d color_renderbuffer: %d framebufferR8: %d statusR8: 0x%x\n", __LINE__, color_renderbuffer, framebufferR8, statusR8);
    
    GLint fbTextures[0];
    glGenTextures(1, fbTextures);
    GLint fbImageTexture = fbTextures[0];

    // Create frame buffer
    GLint frameBuffers[0];
    glGenFramebuffers(1, frameBuffers);
    GLint frameBuffer = frameBuffers[0];

    // Bind the texture to frame buffer
    glBindTexture(GL_TEXTURE_2D, fbImageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenwidth, screenheight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbImageTexture, 0);
    #endif
        
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
    
    clock_gettime(CLOCK_REALTIME, &tmS);

    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);

    // Decode the image and load it into texture
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_RGB, GL_UNSIGNED_BYTE, raw);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_RED_EXT, GL_UNSIGNED_BYTE, raw);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, raw);

    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);

    GLint uTextureLocation = glGetUniformLocation(shaderProgram, "u_texture");
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uTextureLocation, 0);
    
    glFinish();
    
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("texture in cost: %d.%d ms\n", tmCost/1000, tmCost%1000);
            
    //printf("main() line: %d textureid: %d, texturelocation: %d \n", __LINE__, imageTexture, uTextureLocation);
   
    glClearColor(0.7f, 0.9f, 0.8f, 1.0f);        
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    rtangle = 0.0f;
    glUniform1f(uRotateLocation, (rtangle * M_PI) / 180.0f);
        
    meacnt = 0;
    while (1) {
        wl_display_dispatch_pending(ESContext.native_display);

        clock_gettime(CLOCK_REALTIME, &tmE);
        tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
        printf("pending delay: %d.%d ms\n", tmCost/1000, tmCost%1000);
        
        // Clear the screen to black        
        clock_gettime(CLOCK_REALTIME, &tmS);
            
        glClear(GL_COLOR_BUFFER_BIT);    

        #if 0
        if (meacnt % 2) {
            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        #endif
    
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //glFinish();

        clock_gettime(CLOCK_REALTIME, &tmE);

        tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
        printf("display cost: %d.%d ms\n", tmCost/1000, tmCost%1000);

        RefreshWindow();
        
        clock_gettime(CLOCK_REALTIME, &tmS);
        
        #if 1
        break;
        #else
        if (meacnt >= 179) {
            break;
        } else {
            meacnt ++;
        }
        #endif
    }

    size = 4 * renderBufferHeight * renderBufferWidth;
    printf("print size");
    printf("size %d", size);

    unsigned char *data2 = (unsigned char *) malloc(size);

    memset(data2, 0, size);
    printf("allocate memory size: %d addr: 0x%.8x \n", size, (unsigned int)data2);
  
    glFinish();
  
    clock_gettime(CLOCK_REALTIME, &tmS);

    rx = 0;
    ry = 0;
    rw = renderBufferWidth;
    rh = renderBufferHeight;
  
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &glrformat);
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &glrdtype);

    LOG("GL CLREAD format: [0x%x] type: [0x%x]\n", glrformat, glrdtype);
    
    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
  
    glReadPixels(rx, ry, rw, rh, GL_RGB, GL_UNSIGNED_BYTE, data2);
    
    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
    //glFinish();
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("readPixels cost: %d.%d ms\n", tmCost/1000, tmCost%1000);
          

    grapglbmp(data2, rw, rh, 24, 3 * rw * rh);
    
    glBindTexture(GL_TEXTURE_2D, fbImageTexture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uTextureLocation, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    rtangle = 10.0f;
    glUniform1f(uRotateLocation, (rtangle * M_PI) / 180.0f);
    glVertexAttribPointer(aTextureCoordinateLocation, 2, GL_FLOAT, false,0, textureCoordinateData2);
    
    clock_gettime(CLOCK_REALTIME, &tmS);
    while (1) {
        wl_display_dispatch_pending(ESContext.native_display);
    
        glClear(GL_COLOR_BUFFER_BIT); 

        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        RefreshWindow();

        break;        
    }

    glFinish();
    
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("loop cost: %d.%d ms\n", tmCost/1000, tmCost%1000);
    

    //wl_display_dispatch_pending(ESContext.native_display);
    
    //glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    
    //glClear(GL_COLOR_BUFFER_BIT); 

    //glDrawArrays(GL_TRIANGLES, 0, 6);

    //RefreshWindow();

    //wl_display_dispatch_pending(ESContext.native_display);
    
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glClear(GL_COLOR_BUFFER_BIT); 

    //glDrawArrays(GL_TRIANGLES, 0, 6);

    //RefreshWindow();

    //wl_display_dispatch_pending(ESContext.native_display);

    //glClear(GL_COLOR_BUFFER_BIT); 
    
    //RefreshWindow();
    
    sleep(2);
    
    size = 4 * renderBufferHeight * renderBufferWidth;
    printf("print size");
    printf("size %d", size);

    data2 = (unsigned char *) malloc(size);

    memset(data2, 0, size);
    printf("allocate memory size: %d addr: 0x%.8x \n", size, (unsigned int)data2);
  
    glFinish();
  
    clock_gettime(CLOCK_REALTIME, &tmS);

    rx = 0;
    ry = 0;
    rw = renderBufferWidth;
    rh = renderBufferHeight;
  
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &glrformat);
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &glrdtype);

    LOG("GL CLREAD format: [0x%x] type: [0x%x]\n", glrformat, glrdtype);
    
    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
  
    glReadPixels(rx, ry, rw, rh, GL_RGB, GL_UNSIGNED_BYTE, data2);
    
    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
    //glFinish();
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("readPixels cost: %d.%d ms\n", tmCost/1000, tmCost%1000);
          

    grapglbmp(data2, rw, rh, 24, 3 * rw * rh);
  
    #elif 0
    GLint aPositionLocation = glGetAttribLocation(shaderProgram, "a_position");
    // Enable the parameter of the location
    glEnableVertexAttribArray(aPositionLocation);
    // Specify the data of a_position
    glVertexAttribPointer(aPositionLocation, VERTEX_COMPONENT_COUNT, GL_FLOAT, false,0, vertexData);
    //glVertexAttribPointer(aPositionLocation, VERTEX_COMPONENT_COUNT, GL_FLOAT, false,0, vertices);

    // Get the location of a_textureCoordinate in the shader
    GLint aTextureCoordinateLocation = glGetAttribLocation(shaderProgram, "a_textureCoordinate");
    // Enable the parameter of the location
    glEnableVertexAttribArray(aTextureCoordinateLocation);
    // Specify the data of a_textureCoordinate
    glVertexAttribPointer(aTextureCoordinateLocation, TEXTURE_COORDINATE_COMPONENT_COUNT, GL_FLOAT, false,0, textureCoordinateData);
    //glVertexAttribPointer(aTextureCoordinateLocation, TEXTURE_COORDINATE_COMPONENT_COUNT, GL_FLOAT, false,0, cubeCoods);
    

    // Get the location of u_Rotate in the shader
    GLint uRatioLocation = glGetUniformLocation(shaderProgram, "u_Ratio");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRatioLocation);
    // Specify the vertex data of u_Ratio
    glUniform1f(uRatioLocation, screenwidth * 1.0f / screenheight);

    printf("main() line: %d aPositionLocation: %d aTextureCoordinateLocation: %d uRatioLocation: %d\n", __LINE__, aPositionLocation, aTextureCoordinateLocation, uRatioLocation);

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
    
    clock_gettime(CLOCK_REALTIME, &tmS);
    
    // Decode the image and load it into texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_RGB, GL_UNSIGNED_BYTE, raw);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, header->aspbiWidth, header->aspbiHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, raw);

    GLint uMVPLocation = glGetUniformLocation(shaderProgram, "u_mvp");
    
    GLint uTextureLocation = glGetUniformLocation(shaderProgram, "u_texture");
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uTextureLocation, 0);
    
    printf("main() line: %d textureid: %d, texturelocation: %d, uMVPLocation: %d \n", __LINE__, imageTexture, uTextureLocation, uMVPLocation);    

    glFinish();
    
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("texture in cost: %d.%d ms\n", tmCost/1000, tmCost%1000);

    GLfloat translateX = 0.0f;
    GLfloat translateY = 0.0f;
    GLfloat translateZ = 0.0f;
    GLfloat rotateX = 0.0f;
    GLfloat rotateY = 0.0f;
    GLfloat rotateZ = 0.0f;
    GLfloat scaleX = 1.0f;
    GLfloat scaleY = 1.0f;
    GLfloat scaleZ = 1.0f;
    GLfloat cameraPositionX = 0.0f;
    GLfloat cameraPositionY = 0.0f;
    GLfloat cameraPositionZ = 5.0f;
    GLfloat lookAtX = 0.0f;
    GLfloat lookAtY = 0.0f;
    GLfloat lookAtZ = 0.0f;
    GLfloat cameraUpX = 0.0f;
    GLfloat cameraUpY = 1.0f;
    GLfloat cameraUpZ = 0.0f;
    GLfloat nearPlaneLeft = -1.0f;
    GLfloat nearPlaneRight = 1.0f;
    #if 1
    GLfloat nearPlaneBottom = (-1.0f * (GLfloat)screenheight )/(GLfloat) screenwidth;
    GLfloat nearPlaneTop = (1.0f * (GLfloat)screenheight) /(GLfloat) screenwidth;;
    #else
    GLfloat nearPlaneBottom = -1.0f;
    GLfloat nearPlaneTop = 1.0f;
    #endif
    GLfloat nearPlane = 1.0f;
    GLfloat farPlane = 1000.0f;

    GLfloat *mvpMatrix = getIdentity(0);
    GLfloat *translateMatrix = getIdentity(0);
    GLfloat *rotateMatrix = getIdentity(0);
    GLfloat *scaleMatrix = getIdentity(0);
    GLfloat *modelMatrix = getIdentity(0);
    GLfloat *viewMatrix = getIdentity(0);
    GLfloat *projectMatrix = getIdentity(0);

    // Calculate the Model matrix
    translateM(translateMatrix, 0, translateX, translateY, translateZ);
    rotateM(rotateMatrix, 0, rotateX, 1.0f, 0.0f, 0.0f);
    rotateM(rotateMatrix, 0, rotateY, 0.0f, 1.0f, 0.0f);
    rotateM(rotateMatrix, 0, rotateZ, 0.0f, 0.0f, 1.0f);
    scaleM(scaleMatrix, 0, scaleX, scaleY, scaleZ);
    multiplyMM(modelMatrix, 0, rotateMatrix, 0, scaleMatrix, 0);
    multiplyMM(modelMatrix, 0, modelMatrix, 0, translateMatrix, 0);

    // Calculate the View matrix
    setLookAtM(viewMatrix, 0,
        cameraPositionX, cameraPositionY, cameraPositionZ,
        lookAtX, lookAtY, lookAtZ,
        cameraUpX, cameraUpY, cameraUpZ);

    // Calculate the Project matrix
    frustumM(projectMatrix, 0,
        nearPlaneLeft, nearPlaneRight, nearPlaneBottom, nearPlaneTop,
        nearPlane, farPlane);

    // Calculate the MVP matrix
    multiplyMM(mvpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
    multiplyMM(mvpMatrix, 0, projectMatrix, 0, mvpMatrix, 0);

    glUniformMatrix4fv(uMVPLocation, 1, false, mvpMatrix);

    GLenum func = GL_LEQUAL;
    GLint v1=0, v2=0;
    GLclampf  near_val=0, far_val=1, bfrest=0.0f;;
    //glDepthFunc(GL_NEVER);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_DEPTH_FUNC);
    //glEnable(GL_CULL_FACE);
    glDepthFunc(func);
    glDepthMask(TRUE);

    glGetIntegerv(GL_DEPTH_BITS, &v1);
    glGetIntegerv(GL_DEPTH_FUNC, &v2);
    //glDepthRangef(near_val, far_val);
    //glClearDepthf(bfrest);
    
    
    printf("is depth test enable: %d BIT: %d func: 0x%.4x\n", glIsEnabled(GL_DEPTH_TEST), v1, v2);

    glClearColor(0.7f, 0.9f, 0.8f, 1.0f);        
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);
    
    meacnt = 0;
    GLint vtexpage[6] = {0, 6, 12, 18, 24, 30};
    while (1) {
        wl_display_dispatch_pending(ESContext.native_display);

        clock_gettime(CLOCK_REALTIME, &tmE);
        tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
        
        //printf("pending delay: %d.%d ms\n", tmCost/1000, tmCost%1000);
        
        // Clear the screen to black        
        clock_gettime(CLOCK_REALTIME, &tmS);
        
        //glEnable(GL_DEPTH_TEST);
        //glDepthMask(TRUE);
        
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        //glDepthFunc(GL_LEQUAL);
        //glCullFace(GL_FRONT);
        
        // Call the draw method with GL_TRIANGLES to render 3 vertices
        //glDrawArrays(GL_TRIANGLES, 0, VERTEX_NUM);

        #if 0
        glDrawElements(GL_TRIANGLES,VERTEX_NUM, GL_UNSIGNED_SHORT, indexs);
        #else
        if (selectPage == 6) {
            glDrawArrays(GL_TRIANGLES, 0, VERTEX_NUM);
        } else {
            glDrawArrays(GL_TRIANGLES, vtexpage[selectPage], 6);
        }
        #endif

        //glFinish();

        clock_gettime(CLOCK_REALTIME, &tmE);

        tmCost = asgltime_diff(&tmS, &tmE, 1000);     
        
        //printf("display cost: %d.%d ms\n", tmCost/1000, tmCost%1000);

        // Set the status before rendering
        /*
        glEnableVertexAttribArray(uRatioLocation);
        glVertexAttribPointer(aPositionLocation, VERTEX_COMPONENT_COUNT, GL_FLOAT, false,0, vertexDataBuffer);
        glEnableVertexAttribArray(aTextureCoordinateLocation);
        glVertexAttribPointer(aTextureCoordinateLocation, TEXTURE_COORDINATE_COMPONENT_COUNT, GL_FLOAT, false,0, textureCoordinateDataBuffer);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        */
        mvpMatrix = getIdentity(mvpMatrix);
        translateMatrix = getIdentity(translateMatrix);
        rotateMatrix = getIdentity(rotateMatrix);
        scaleMatrix = getIdentity(scaleMatrix);
        modelMatrix = getIdentity(modelMatrix);
        viewMatrix = getIdentity(viewMatrix);
        projectMatrix = getIdentity(projectMatrix);

        switch (rotArix) {
        case 0:
            rotateX += 1.0f;        
            break;
        case 1:
            rotateY += 1.0f;
            break;
        default:
            rotateY += 1.0f;
            rotateZ += 1.0f;
            break;
        }
        
        //translateX += 0.01f;

        //scaleX = scaleX * 0.99f;
        //scaleY = scaleY * 0.99f;
        //scaleZ = scaleZ * 0.99f;
        
        // Calculate the Model matrix
        translateM(translateMatrix, 0, translateX, translateY, translateZ);
        rotateM(rotateMatrix, 0, rotateX, 1.0f, 0.0f, 0.0f);
        rotateM(rotateMatrix, 0, rotateY, 0.0f, 1.0f, 0.0f);
        rotateM(rotateMatrix, 0, rotateZ, 0.0f, 0.0f, 1.0f);
        scaleM(scaleMatrix, 0, scaleX, scaleY, scaleZ);
        multiplyMM(modelMatrix, 0, rotateMatrix, 0, scaleMatrix, 0);
        multiplyMM(modelMatrix, 0, modelMatrix, 0, translateMatrix, 0);

        // Calculate the View matrix
        setLookAtM(viewMatrix, 0,
            cameraPositionX, cameraPositionY, cameraPositionZ,
            lookAtX, lookAtY, lookAtZ,
            cameraUpX, cameraUpY, cameraUpZ);

        // Calculate the Project matrix
        frustumM(projectMatrix, 0,
            nearPlaneLeft, nearPlaneRight, nearPlaneBottom, nearPlaneTop,
            nearPlane, farPlane);

        // Calculate the MVP matrix
        multiplyMM(mvpMatrix, 0, viewMatrix, 0, modelMatrix, 0);
        multiplyMM(mvpMatrix, 0, projectMatrix, 0, mvpMatrix, 0);

        glUniformMatrix4fv(uMVPLocation, 1, false, mvpMatrix);
                
        clock_gettime(CLOCK_REALTIME, &tmS);
        
        RefreshWindow();
        
        //usleep(10000);
        
        #if 0
        if (meacnt >= 360) {
            break;
        } else {
            meacnt++;
        }
        #endif
    }

        
    #elif 1 /* rotate */
    // Get the location of a_position in the shader
    //GLint aLocation = glGetAttribLocation(shaderProgram, "position");
    //GLint asLocation = glGetAttribLocation(shaderProgram, "s_position");
    //printf("main() line: %d location vertex test: %d asLocation: %d \n", __LINE__, aLocation, asLocation);
    GLfloat vertexData[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    GLfloat textureCoordinateData[] = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

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

    #if 0
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
    #endif
    
    // Get the location of u_Rotate in the shader
    GLfloat rtangle=0.0f;
    GLint uRotateLocation = glGetUniformLocation(shaderProgram, "u_Rotate");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRotateLocation);
    // Specify the vertex data of u_Rotate
    glUniform1f(uRotateLocation, (rtangle * 3.1415f) / 180.0f);

    // Get the location of u_Rotate in the shader
    GLint uRatioLocation = glGetUniformLocation(shaderProgram, "u_Ratio");
    // Enable the parameter of the location
    glEnableVertexAttribArray(uRatioLocation);
    // Specify the vertex data of u_Ratio
    glUniform1f(uRatioLocation, screenwidth * 1.0f / screenheight);

    printf("main() line: %d uRotateLocation: %d uRatioLocation: %d \n", __LINE__, uRotateLocation, uRatioLocation);

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

    
    clock_gettime(CLOCK_REALTIME, &tmS);

    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
  
    // Decode the image and load it into texture
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_RGB, GL_UNSIGNED_BYTE, raw);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, header->aspbiWidth, abs(header->aspbiHeight), 0, GL_ALPHA, GL_UNSIGNED_BYTE, raw);

    glerr = glGetError();
    LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
  
    GLint uTextureLocation = glGetUniformLocation(shaderProgram, "u_texture");
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uTextureLocation, 0);
    
    glFinish();
    
    clock_gettime(CLOCK_REALTIME, &tmE);

    tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
    printf("texture in cost: %d.%d ms\n", tmCost/1000, tmCost%1000);
            
    //printf("main() line: %d textureid: %d, texturelocation: %d \n", __LINE__, imageTexture, uTextureLocation);
   
    glClearColor(0.7f, 0.9f, 0.8f, 1.0f);        
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);
    
    meacnt = 0;
    while (1) {
        wl_display_dispatch_pending(ESContext.native_display);
        //draw();

        clock_gettime(CLOCK_REALTIME, &tmE);
        tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
        //printf("pending delay: %d.%d ms\n", tmCost/1000, tmCost%1000);
        
        // Clear the screen to black        
        clock_gettime(CLOCK_REALTIME, &tmS);
            
        glClear(GL_COLOR_BUFFER_BIT);       
    
        // Draw a triangle from the 3 vertices        
        #if 1
        glDrawArrays(GL_TRIANGLES, 0, 6);
        #else
        glDrawArrays(GL_TRIANGLES, 0, 3);
        #endif

        //glFinish();

        clock_gettime(CLOCK_REALTIME, &tmE);

        tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
        //printf("display cost: %d.%d ms\n", tmCost/1000, tmCost%1000);

        rtangle += 1.0f;
        glUniform1f(uRotateLocation, (rtangle * 3.1415f) / 180.0f);

        clock_gettime(CLOCK_REALTIME, &tmS);
        
        RefreshWindow();
        //usleep(1000);

        
        #if 0
        break;
        #else
        if (meacnt >= 360) {
            break;
        } else {
            meacnt ++;
        }
        #endif
    }

    
    sleep(2);
    
  //qDebug() << eglSwapBuffers(   eglDisplay, eglSurface);
  int size = 4 * renderBufferHeight * renderBufferWidth;
  printf("print size \n");
  printf("size %d \n", size);
  //qDebug() << size;

  unsigned char *data2 = (unsigned char *) malloc(size);

  memset(data2, 0, size);
  printf("allocate memory size: %d addr: 0x%.8x \n", size, (unsigned int)data2);
  
  glFinish();
  
  glerr = glGetError();
  LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);
  
  clock_gettime(CLOCK_REALTIME, &tmS);

  GLint rx=0, ry=0, rw=0, rh=0;

  //rx = renderBufferWidth / 4;
  //ry = renderBufferHeight / 4;
  //rw = renderBufferWidth / 2;
  //rh = renderBufferHeight / 2;

  rx = 0;
  ry = 0;
  rw = renderBufferWidth;
  rh = renderBufferHeight;
  
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &glrformat);
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &glrdtype);

  LOG("GL CLREAD format: [0x%x] type: [0x%x]\n", glrformat, glrdtype);
  
  //glReadPixels(rx, ry, rw, rh, GL_RGB, GL_UNSIGNED_BYTE, data2);
  glReadPixels(0,0,renderBufferWidth,renderBufferHeight,GL_RGB, GL_UNSIGNED_BYTE, data2);
  
  glerr = glGetError();
  LOG("GL ERR: 0x%x line %d\n", glerr, __LINE__);

  ret = eglGetError();    
  //glFinish();
  clock_gettime(CLOCK_REALTIME, &tmE);

  tmCost = asgltime_diff(&tmS, &tmE, 1000);                                                
  printf("readPixels x: %d, y: %d, w: %d, h: %d, screen: %d, %d, cost: %d.%d ms ret: %d \n", rx, ry, rw, rh, renderBufferWidth, renderBufferHeight, tmCost/1000, tmCost%1000, ret);



  grapglbmp(data2, rw, rh, 24, 3 * rw * rh);
  //grapglbmp(data2, renderBufferWidth, renderBufferHeight, 8, 1 * renderBufferHeight * renderBufferWidth);
  
  //shmem_dump(data2, size);

  //QImage image(data2, renderBufferWidth,  renderBufferHeight,renderBufferWidth*2, QImage::Format_RGB16);
  //image.save("result.png");
  //qDebug() << "done";
  //QCoreApplication a(argc, argv);

  const GLubyte *glstr=0;

  glstr = glGetString(GL_VENDOR);

  if (glstr)
      LOG("GL_VENDOR: [%s] \n", glstr);
  else
      LOG("GL_VENDOR Error \n");
      
  glstr = glGetString(GL_RENDERER);
  if (glstr)
      LOG("GL_RENDERER: [%s] \n", glstr);
  else 
      LOG("GL_RENDERER Error \n");
      
  glstr = glGetString(GL_VERSION);
  if (glstr)
    LOG("GL_VERSION: [%s] \n", glstr);

  glstr = glGetString(GL_SHADING_LANGUAGE_VERSION);
  if (glstr)
    LOG("GL_SHADING_LANGUAGE_VERSION: [%s] \n", glstr);

  glstr = glGetString(GL_EXTENSIONS);
  if (glstr)
    LOG("GL_EXTENSIONS: [%s] \n", glstr);

  
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

  return 0;
}
#endif

