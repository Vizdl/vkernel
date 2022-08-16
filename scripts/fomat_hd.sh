#!/bin/bash
dest=$1
os=$2
cfg1=$3
cfg2=$4
mountdir=/tmp/mountdir
# 1. 虚拟化虚拟硬盘为块设备
dev=`losetup --partscan --show --find $dest`
# 2. 查找分区块设备名
subdev="${dev}p1" # 如若利用 ls 访问 $subdev 则可能不存在
#echo "dev is : $dev"
#echo "subdev is : $subdev"
# echo "mountdir is : $mountdir"
# 格式化分区
mkfs.ext2 $subdev
# 3. 挂载分区
if [[ ! -d $mountdir ]];
then 
    mkdir $mountdir
else
    umount $mountdir
fi
mount $subdev $mountdir
# 4. 安装 grub
grub2-install --force --no-floppy  --root-directory=$mountdir $dev
# 5. 安装配置文件与os
cp $os $mountdir/boot
mkdir $mountdir/etc/default -p
cp $cfg1 $mountdir/etc/default/
cp $cfg2 $mountdir/boot/grub2/grub.cfg
# 6. 卸载分区
# tree $mountdir
umount $mountdir
rm -rf $mountdir
# 7. 卸载磁盘块设备
losetup -d $dev