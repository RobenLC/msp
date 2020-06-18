
#CC = $(PWD)/stgcc
#LD = $(PWD)/stld
PROG = out/st_mothership.bin
#PROG = out/st_mslave_test.bin
#PROG = out/st_master_test.bin
#PROG = out/mainrot.bin
SRCS = source/mothership.c
#SRCS = source/mainrot.c
#SRCS += source/rotate.c
#SRCS = source/master_spidev_test.c
#SRCS = source/mslave_spidev_test.c

CLEANFILES = $(PROG)

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
#CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-pointer-sign
CFLAGS += -Wno-unused-but-set-variable
CFLAGS += -Wno-unused-function
#CFLAGS += -static
CFLAGS += -I ./include
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
LIBS += -lEGL
LIBS += -lGLESv2
LIBS += -ljpeg
LIBS += -lturbojpeg
#LIBS += -lopencv_core

all: $(PROG)

$(PROG): $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(CLEANFILES) $(patsubst %.c,%.o, $(SRCS))
