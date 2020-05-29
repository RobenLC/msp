
#ssh root@192.168.21.137 rm /home/root/mainrot.bin
ssh root@192.168.21.137 rm /home/root/st_mothership.bin
#ssh root@192.168.21.137 rm /home/root/st_mslave_test.bin

ssh root@192.168.21.137 sync

#scp out/mainrot.bin root@192.168.21.137:/home/root/mainrot.bin
scp out/st_mothership.bin root@192.168.21.137:/home/root/st_mothership.bin
#scp out/st_mslave_test.bin root@192.168.21.137:/home/root/st_mslave_test.bin

ssh root@192.168.21.137 sync
