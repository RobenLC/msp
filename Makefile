
DISTRIBUTION_PATH = ~/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20
COMPONENT_PATH =  $(DISTRIBUTION_PATH)/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components

INCLUDE  = -I $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/opencv/usr/include/opencv
INCLUDE += -I $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/opencv/usr/include/
INCLUDE += -I ./include

OPENCV_LIB  = -L $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/ffmpeg/usr/lib
OPENCV_LIB += -L $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/libgphoto2/usr/lib
OPENCV_LIB += -L $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/libexif/usr/lib
OPENCV_LIB += -L $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/x264/usr/lib
OPENCV_LIB += -L $(COMPONENT_PATH)/cortexa7t2hf-neon-vfpv4/opencv/usr/lib
OPENCV_LIB += -L $(COMPONENT_PATH)/cortexa7hf-neon-vfpv4/tbb/usr/lib/
OPENCV_LIB += -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm
OPENCV_LIB += -lopencv_bioinspired -lopencv_ccalib -lopencv_dpm -lopencv_face -lopencv_photo -lopencv_fuzzy
OPENCV_LIB += -lopencv_hfs -lopencv_img_hash -lopencv_line_descriptor -lopencv_optflow -lopencv_reg
OPENCV_LIB += -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping
OPENCV_LIB += -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_plot -lopencv_xfeatures2d
OPENCV_LIB += -lopencv_shape -lopencv_video -lopencv_ml -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d
OPENCV_LIB += -lopencv_highgui -lopencv_videoio -lopencv_flann -lopencv_xobjdetect -lopencv_imgcodecs
OPENCV_LIB += -lopencv_objdetect -lopencv_xphoto -lopencv_imgproc -lopencv_core -lavcodec
OPENCV_LIB += -lavformat -lavutil -lswscale -lgphoto2 -lgphoto2_port -ltbb -lswresample -lx264 -lexif

#CC = $(PWD)/stgcc
#LD = $(PWD)/stld
#PROG = out/st_mothership.bin
#PROG = out/st_mslave_test.bin
#PROG = out/st_master_test.bin
#PROG = out/mainrot.bin
#SRCS = source/mothership.c
#SRCS = source/mainrot.c
#SRCS += source/rotate.c
#SRCS = source/master_spidev_test.c
#SRCS = source/mslave_spidev_test.c

#CLEANFILES = $(PROG)

# Add / change option in CFLAGS and LDFLAGS
#CFLAGS += $(shell pkg-config --cflags jpg)
#LDFLAGS += $(shell pkg-config --libs jpg)
#CFLAGS += $(shell pkg-config --cflags gl)
#LDFLAGS += $(shell pkg-config --libs gl)
#CFLAGS += $(shell pkg-config --cflags glext)
#LDFLAGS += $(shell pkg-config --libs glext)
#CFLAGS += $(shell pkg-config --cflags glut)
#LDFLAGS += $(shell pkg-config --libs glut)


#CFLAGS += -O3
CFLAGS += -O0
CFLAGS += -lpthread
CFLAGS += -funsigned-char
CFLAGS += -v
#CFLAGS += -traditional -E
CFLAGS += -Wall
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-unused-but-set-variable
CFLAGS += -Wno-unused-function
#CFLAGS += -static
#CFLAGS += -g 
#CFLAGS += -D_GNU_SOURCE

#LDFLAGS += -L $(PWD)/libs
#LDFLAGS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Developer-Packager/SDK/sysroots/x86_64-openstlinux_weston_sdk-linux/usr/share/gcc-arm-none-eabi/arm-none-eabi/lib/thumb/v7e-m/fpv4-sp/hard


#LIBS += -pipe
#LIBS += -march=armv7ve
#LIBS += -mfpu=neon-vfpv4
#LIBS += -mfloat-abi=hard
#LIBS += -mthumb
#LIBS += -mcpu=cortex-a7
#LIBS += -Wl,--hash-style=sysv
#LIBS += --sysroot=$(PWD)/sysroot
LIBS += -lm
LIBS += -lwayland-client
LIBS += -lwayland-server
LIBS += -lwayland-egl
LIBS += -lEGL
LIBS += -lGLESv2
LIBS += -lGLESv1_CM
LIBS += -ljpeg
#LIBS += -lturbojpeg
#LIBS += -lopencv_core

.PHONY : clean TARGET FORCE

TARGET_FILE =  out/st_mothership.bin out/mainrot.bin out/eglex.bin

TARGET : $(TARGET_FILE)

BKOCR_LIB = ./ocr/libbkocr.a ./ocr/libmakemodel.a
M4_LIB = ./m4/m4_process.a

BKOCRTEST_CPPSOURCES = ocr_agent.cpp
BKOCRTEST_CPPOBJECTS = $(BKOCRTEST_CPPSOURCES:.cpp=.o)

MOTHERSHIP_CSOURCES = mothership.c
MOTHERSHIP_CSOURCES_PATH = source/$(MOTHERSHIP_CSOURCES)
MOTHERSHIP_COBJECTS_PATH = $(MOTHERSHIP_CSOURCES_PATH:.c=.o)
MOTHERSHIP_COBJECTS = $(MOTHERSHIP_CSOURCES:.c=.o)

ROTATE_SRCS = rotate.c
ROTATE_SRCS_PATH = source/$(ROTATE_SRCS)
ROTATE_SRCS_PATH_OBJ = $(ROTATE_SRCS_PATH:.c=.o)

MAINROT_SRCS = mainrot.c
MAINROT_SRCS_PATH = source/$(MAINROT_SRCS)
MAINROT_SRCS_PATH_OBJ = $(MAINROT_SRCS_PATH:.c=.o)

MAINROT_COBJECTS = mainrot.o
MAINROT_COBJECTS += rotate.o

EGLEX_SRCS = gl_example.c
EGLEX_SRCS_PATH = source/$(EGLEX_SRCS)
EGLEX_SRCS_PATH_OBJ = $(EGLEX_SRCS_PATH:.c=.o)
EGLEX_COBJECTS = $(EGLEX_SRCS:.c=.o)
EGLEX_COBJECTS += rotate.o


SRCS = $(MOTHERSHIP_CSOURCES)
SRCS += $(MAINROT_SRCS)
SRCS += $(ROTATE_SRCS)

#$(BKOCR_LIB) : FORCE
$(BKOCR_LIB) :
	$(MAKE) -C ./ocr -f makefilelib
	@echo

$(M4_LIB): FORCE
	$(MAKE) -C ./m4 clean
	$(MAKE) -C ./m4
	@echo
	
out/st_mothership.bin : $(MOTHERSHIP_COBJECTS_PATH) $(MOTHERSHIP_COBJECTS) $(BKOCRTEST_CPPOBJECTS) $(BKOCR_LIB) $(M4_LIB)
	$(CXX) $(CFLAGS) $(LIBS) $(MOTHERSHIP_COBJECTS) $(BKOCRTEST_CPPOBJECTS) $(INCLUDE) $(OPENCV_LIB) $(BKOCR_LIB) $(M4_LIB) -o $@
	@echo

out/mainrot.bin : $(ROTATE_SRCS_PATH_OBJ) $(MAINROT_SRCS_PATH_OBJ) $(MAINROT_COBJECTS) $(BKOCRTEST_CPPOBJECTS) $(BKOCR_LIB)
	$(CXX) $(CFLAGS) $(LIBS) $(MAINROT_COBJECTS) $(BKOCRTEST_CPPOBJECTS) $(INCLUDE) $(OPENCV_LIB) $(BKOCR_LIB) -o $@
	@echo

out/eglex.bin : $(EGLEX_SRCS_PATH_OBJ) $(EGLEX_COBJECTS) $(ROTATE_SRCS_PATH_OBJ)
	$(CXX) $(CFLAGS) $(LIBS) $(EGLEX_COBJECTS) $(INCLUDE) -o $@
	@echo

.c.o :
	$(CC) $(CFLAGS) $(INCLUDE) -c $<
	@echo

.cpp.o :
	$(CXX) $(CFLAGS) $(INCLUDE) -c $^
	@echo

#all: $(PROG)
#$(PROG): $(SRCS)
#	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(CLEANFILES) $(patsubst %.c,%.o, $(SRCS)) $(patsubst %.cpp,%.o, $(BKOCRTEST_CPPSOURCES))
	$(MAKE) -C ./ocr -f makefilelib clean
