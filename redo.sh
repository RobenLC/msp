
cp /mnt/socket/helloworld/source/rotate.c ./source/rotate.c
cp /mnt/socket/helloworld/source/mainrot.c ./source/mainrot.c
cp /mnt/socket/helloworld/source/mothership.c ./source/mothership.c
#cp /mnt/socket/helloworld/source/master_spidev_test.c ./source/master_spidev_test.c
cp /mnt/socket/helloworld/source/mslave_spidev_test.c ./source/mslave_spidev_test.c

make clean

make


#./enp.sh

