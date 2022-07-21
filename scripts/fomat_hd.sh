#!/bin/bash
dest=$1
os=$2
cfg1=$3
cfg2=$4
mountdir=/mnt/vostemp
# 虚拟化虚拟硬盘为块设备
dev=`losetup --partscan --show --find $dest`
# 查找分区块设备名
subdev=`ls ${dev}p* -1`
echo "dev is : $dev"
echo "subdev is : $subdev"
echo "mountdir is : $mountdir"
# 格式化分区
mkfs.ext2 $subdev
# 挂载分区
if [[ ! -d $mountdir ]];
then 
    mkdir $mountdir
else
    umount $mountdir
fi
mount $subdev $mountdir
# 安装 grub
grub2-install --force --no-floppy  --root-directory=$mountdir $dev
# 安装配置文件与os
cp $os $mountdir/boot
mkdir $mountdir/etc/default -p
cp $cfg1 $mountdir/etc/default/
cp $cfg2 $mountdir/boot/grub2/grub.cfg
# 卸载分区
tree $mountdir
umount $mountdir
# 卸载磁盘块设备
losetup -d $dev