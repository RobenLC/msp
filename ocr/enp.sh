
#BINFILE="dk2_ocr_scanin"
BINFILE="dk2_linadapt4_sec9"

ssh root@192.168.21.137 mkdir -p /home/root/STM_DK2/

ssh root@192.168.21.137 rm /home/root/STM_DK2/$BINFILE

ssh root@192.168.21.137 sync

scp $BINFILE root@192.168.21.137:/home/root/STM_DK2/$BINFILE

ssh root@192.168.21.137 sync

scp cv_tw_model.txt root@192.168.21.137:/home/root/STM_DK2/
scp cv_tw_model_linear.txt root@192.168.21.137:/home/root/STM_DK2/

ssh root@192.168.21.137 sync
