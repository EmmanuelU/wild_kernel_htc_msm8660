#!/sbin/sh
echo \#!/sbin/sh > /tmp/createnewboot.sh
echo /tmp/mkbootimg --kernel /tmp/zImage --ramdisk /tmp/boot.img-ramdisk.gz --cmdline \"$(cat /tmp/boot.img-cmdline)\" --base 0x48000000 --output /tmp/newboot.img >> /tmp/createnewboot.sh
chmod 777 /tmp/createnewboot.sh
/tmp/createnewboot.sh
rm -rf /sdcard/WildKernel-boot.img
cp /tmp/newboot.img /sdcard/WildKernel-boot.img
return $?
