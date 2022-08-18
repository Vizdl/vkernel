#!/bin/bash
dest=$1
os=$2
cfg1=$3
cfg2=$4
mountdir=/tmp/mountdir
# 1. 虚拟化虚拟硬盘为块设备并格式化分区(这里用 && 是为了等待前面执行结束)
dev=`losetup --partscan --show --find $dest` && subdev="${dev}p1" && mkfs.ext2 $subdev
# 2. 挂载分区
if [[ ! -d $mountdir ]];
then 
    mkdir $mountdir
else
    umount $mountdir
fi
mount $subdev $mountdir
# 3. 安装 grub
grub2-install --force --no-floppy  --root-directory=$mountdir $dev
# 4. 安装配置文件与os
cp $os $mountdir/boot
mkdir $mountdir/etc/default -p
cp $cfg1 $mountdir/etc/default/
cp $cfg2 $mountdir/boot/grub2/grub.cfg
# 5. 卸载分区
umount $mountdir
rm -rf $mountdir
# 7. 卸载磁盘块设备
losetup -d $dev