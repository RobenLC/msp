
# path: /home/leoc/ST/STM32MP15-Ecosystem-v1.0.0/sysroots

dest=root@192.168.19.3
libfolder=~/ST/STM32MP15-Ecosystem-v1.0.0/Distribution-Package/openstlinux-4.19-thud-mp1-19-02-20/build-openstlinuxweston-stm32mp1/tmp-glibc/sysroots-components


scp -r $libfolder/cortexa7t2hf-neon-vfpv4/ffmpeg/usr/lib/*       $dest:/usr/lib/
scp -r $libfolder/cortexa7t2hf-neon-vfpv4/libgphoto2/usr/lib/*   $dest:/usr/lib/
scp -r $libfolder/cortexa7t2hf-neon-vfpv4/libexif/usr/lib/*      $dest:/usr/lib/
scp -r $libfolder/cortexa7t2hf-neon-vfpv4/x264/usr/lib/*         $dest:/usr/lib/
scp -r $libfolder/cortexa7hf-neon-vfpv4/tbb/usr/lib/*            $dest:/usr/lib/
scp -r $libfolder/cortexa7t2hf-neon-vfpv4/opencv/usr/lib/*       $dest:/usr/lib/

ssh $dest sync
