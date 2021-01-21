
DST=137

ssh root@192.168.21.$DST rm /home/root/sd/ocr/nxp_mothership.bin

ssh root@192.168.21.$DST sync

scp out/st_mothership.bin root@192.168.21.$DST:/home/root/sd/ocr/nxp_mothership.bin

ssh root@192.168.21.$DST sync
