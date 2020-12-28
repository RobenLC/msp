
INCLUDE = -include ./Inc/main.h
#INCLUDE += -I ../../STM32CUBE/Drivers/STM32MP1xx_HAL_Driver/Inc/
#INCLUDE += -I ../../STM32CUBE/Drivers/CMSIS/Device/ST/STM32MP1xx/Include/
#INCLUDE += -I ../../STM32CUBE/Middlewares/Third_Party/OpenAMP/open-amp/lib/include/
#INCLUDE += -I ../../STM32CUBE/Middlewares/Third_Party/OpenAMP/libmetal/lib/include/
#INCLUDE += -I ../../STM32CUBE/Drivers/BSP/STM32MP15xx_DISCO/
#INCLUDE += -I ../../STM32CUBE/Middlewares/Third_Party/OpenAMP/virtual_driver/

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

.PHONY: clean TARGET FORCE

TARGET_LIB = m4_process.a
TARGET_BIN = m4_process.bin

TARGET: $(TARGET_BIN)

#M4_SOURCES = $(wildcard ./Src/*.c)
M4_SOURCES = ./Src/cmd_handler.c
M4_SOURCES += ./Src/Image_PreProcess.c
M4_SOURCES += ./Src/ImageProcessing.c
M4_SOURCES += ./Src/BasicSet.c
M4_SOURCES += ./Src/CFilters.c
M4_SOURCES += ./Src/BKNote_PreSet.c

M4_OBJECTS = $(patsubst %.c,%.o,$(M4_SOURCES))

$(TARGET_BIN): $(M4_OBJECTS) $(M4_SOURCES) 
	$(CC) $< -o $@
	@echo

$(TARGET_LIB): $(M4_SOURCES) $(M4_OBJECTS)
	ar -rcs $@ $(M4_OBJECTS)
	@echo

$(M4_OBJECTS): %.o : %.c
	$(CC) -D MAKELIB -c $< -o $@ $(INCLUDE) $(CFLAGS)
	@echo

clean:
	rm -f *.o $(M4_OBJECTS)
	rm $(TARGET_BIN)



