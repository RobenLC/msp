
#INCLUDE = $(shell pkg-config --cflags opencv)
#LIBS = $(shell pkg-config --libs opencv)

INCLUDE = -I /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/opencv/usr/include/opencv
INCLUDE += -I /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/opencv/usr/include

LIBS = -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/ffmpeg/usr/lib
LIBS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/libgphoto2/usr/lib
LIBS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/libexif/usr/lib
LIBS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/x264/usr/lib
LIBS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7hf-neon-vfpv4/tbb/usr/lib/
LIBS += -L /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components/cortexa7t2hf-neon-vfpv4/opencv/usr/lib
LIBS += -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dpm -lopencv_face -lopencv_photo -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_line_descriptor -lopencv_optflow -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_plot -lopencv_xfeatures2d -lopencv_shape -lopencv_video -lopencv_ml -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_flann -lopencv_xobjdetect -lopencv_imgcodecs -lopencv_objdetect -lopencv_xphoto -lopencv_imgproc -lopencv_core -lavcodec
LIBS += -lavformat -lavutil -lswscale -lgphoto2 -lgphoto2_port -ltbb -lswresample -lx264 -lexif


#SOURCES = t1_20200309_dk2_sec1.cpp svm.cpp 
#SOURCES = t1_20200309_dk2_sec2.cpp svm.cpp 
#SOURCES = t1_20200309_dk2_sec3.cpp svm.cpp 
#SOURCES = t1_20200309_dk2_sec4.cpp svm.cpp 
#SOURCES = t1_20200309_dk2_sec5.cpp svm.cpp 
#SOURCES = t1_20200309_dk2_sec6.cpp svm.cpp 
#SOURCES = t1_20200311_dk2_loop1.cpp svm.cpp 
#SOURCES = t1_20200316_4.cpp svm.cpp 
#------- LIBLINEAR mode 
#SOURCES = dk2_liblin_sec1.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_liblin_sec2.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_liblin_sec3.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_liblin_sec4.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_liblin_sec5.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_liblin_sec6.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#-------- LIBLINEAR + adaptive threshold
#SOURCES = dk2_linadapt2_sec1.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt2_sec2.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt2_sec3.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt2_sec4.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt2_sec5.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt2_sec6.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#-----------LIBLINEAR + adaptive + font > 10
#SOURCES = dk2_linadapt3_sec3.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt3_sec9.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt3_sec10.cpp svm.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#----------LIBLINEAR + adaptive + crop area
#SOURCES = dk2_linadapt4_sec3.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec4.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec5.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec6.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec7.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec8.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
SOURCES = dk2_linadapt4_sec9.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec10.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec11.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c
#SOURCES = dk2_linadapt4_sec12.cpp linear.cpp tron.cpp daxpy.c ddot.c dnrm2.c dscal.c




# ?®æ??‡ä»¶
OBJECTS = $(SOURCES:.cpp=.o)
#------------- LIBSVM model 
#TARGET =  dk2_ocr_sec1
#TARGET =  dk2_ocr_sec2
#TARGET =  dk2_ocr_sec3
#TARGET =  dk2_ocr_sec4
#TARGET =  dk2_ocr_sec5
#TARGET =  dk2_ocr_sec6
#TARGET =  dk2_ocr_loop1
#TARGET =  dk2_ocr_scanin
#-------------------- LIBLINEAR mode 
#TARGET =  dk2_lin_sec1
#TARGET =  dk2_lin_sec2
#TARGET =  dk2_lin_sec3
#TARGET =  dk2_lin_sec4
#TARGET =  dk2_lin_sec5
#TARGET =  dk2_lin_sec6
#------------------------ adaptive threshold 
#TARGET =  dk2_linadapt2_sec1
#TARGET =  dk2_linadapt2_sec2
#TARGET =  dk2_linadapt2_sec3
#TARGET =  dk2_linadapt2_sec4
#TARGET =  dk2_linadapt2_sec5
#TARGET =  dk2_linadapt2_sec6
#------------------------ #font > 10 error handle
#TARGET =  dk2_linadapt3_sec3
#TARGET =  dk2_linadapt3_sec9
#TARGET =  dk2_linadapt3_sec10
#------------------------ crop area
#TARGET =  dk2_linadapt4_sec3
#TARGET =  dk2_linadapt4_sec4
#TARGET =  dk2_linadapt4_sec5
#TARGET =  dk2_linadapt4_sec6
#TARGET =  dk2_linadapt4_sec7
#TARGET =  dk2_linadapt4_sec8
TARGET =  dk2_linadapt4_sec9
#TARGET =  dk2_linadapt4_sec10
#TARGET =  dk2_linadapt4_sec11
#TARGET =  dk2_linadapt4_sec12


$(TARGET):$(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(INCLUDE) $(LIBS) -Wall

$(OBJECTS):$(SOURCES)
	$(CXX) -c $(SOURCES) $(INCLUDE) $(LIBS)

clean:
	rm $(TARGET)
	rm *.o
	$(CXX) -I $(INCLUDE) -o $@ -c $<
  
