#!/bin/bash
size=$2
dest=$1/hd${size}M.img
# 创建 raw disk
sudo rm -rf $dest
sudo bximage -hd -mode="flat" -size=$size -q $dest
# 执行 expect 脚本,设置分区表创建分区。
sudo expect<<-EOF
set timeout -1
spawn fdisk $dest
expect {
    "Command (m for help)" {send "n\n";exp_continue}
    "Select (default p)" {send "p\n";exp_continue}
    "Partition number (1-4, default 1)" {send "1\n";exp_continue}
    "default 2048)" {send "\n";exp_continue}
    "Last sector, +sectors or +size{K,M,G}" {send "\n"}
}
expect "Command (m for help)" {send "w\n";exp_continue} 
EOF
# 展示磁盘分区
sudo fdisk -l  $dest