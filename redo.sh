
#cp /mnt/socket/helloworld/source/rotate.c ./source/rotate.c
#cp /mnt/socket/helloworld/source/mainrot.c ./source/mainrot.c
cp /mnt/socket/helloworld/source/mothership.c ./source/mothership.c
#cp /mnt/socket/helloworld/source/master_spidev_test.c ./source/master_spidev_test.c
#cp /mnt/socket/helloworld/source/mslave_spidev_test.c ./source/mslave_spidev_test.c
#cp /mnt/socket/helloworld/source/gl_example.c ./source/gl_example.c

#make clean

make


./enpocr_imx.sh
#./enpocr.sh
#./enpgl.sh
#./enp.sh

ssh root@192.168.21.137 ls
