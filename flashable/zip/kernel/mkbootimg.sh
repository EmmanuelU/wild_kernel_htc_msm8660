#!/sbin/sh
echo \#!/sbin/sh > /tmp/createnewboot.sh
echo /tmp/mkbootimg --kernel /tmp/zImage --ramdisk /tmp/ramdisks/$1.gz --cmdline "no_console_suspend=1" --base 0x48000000 --output /tmp/boot.img >> /tmp/createnewboot.sh
rm -rf /sdcard/WildKernel-boot.img
cp /tmp/boot.img /sdcard/WildKernel-boot.img
chmod 777 /tmp/createnewboot.sh
/tmp/createnewboot.sh
return $?
