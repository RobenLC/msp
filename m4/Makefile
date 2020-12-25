
INCLUDE = -I ./Inc

.PHONY : clean TARGET FORCE

TARGET_FILE = m4_process.a

TARGET : $(TARGET_FILE)

M4_SRCS = cmd_handler.c
M4_SRCS_PATH = Src/$(M4_SRCS)
M4_SRCS_PATH_OBJ = $(M4_SRCS_PATH:.c=.o)

M4_OBJS = cmd_handler.o

m4_process.a : $(M4_SRCS_PATH_OBJ) $(M4_OBJS)
	ar -rcs m4_process.a $(M4_OBJS)
	@echo

.c.o:
	$(CC) -D MAKELIB -c $< $(INCLUDE)
	@echo

clean:
	rm -r $(TARGET_FILE)
	rm -f *.o



