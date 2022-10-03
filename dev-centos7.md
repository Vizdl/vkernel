# vkernel centos7 开发环境搭建
## 安装基础开发工具
```bash
sudo yum install make gcc gcc-c++ -y
```
## 编译安装 grub
目前发现2.00 有bug(无法直接在 centos7 上编译), 2.06 需要 5.1 或以上的 gcc,故选择 2.02 版本
```bash
# 下载
wget --no-check-certificate https://ftp.gnu.org/gnu/grub/grub-2.02.tar.gz
# 解压
tar -xzvf grub-2.02.tar.gz
# 安装依赖
sudo yum install bison flex -y
# 配置
./configure
# 编译
make -j32
# 安装
make install
```
## 编译安装 bochs
```bash
# 添加 bochs 环境变量到全局
# make bochsrun 需要用到该环境变量
echo 'export BOCHS_ROOT_PATH="/opt/bochs"' >> /etc/profile
source /etc/profile
# 安装 bochs 依赖
sudo yum install gtk2 gtk2-devel gtk2-devel-docs -y
# 源码下载路径
https://sourceforge.net/projects/bochs/files/bochs/2.6.2/
# 解压
tar -xzvf bochs-2.6.2.tar.gz
## 配置
./configure \
--prefix=$BOCHS_ROOT_PATH \
--enable-debugger \
--enable-disasm \
--enable-iodebug \
--enable-x86-debugger \
--with-x \
--with-x11 
# 在 Makefile 92 行末尾添加 -lpthread,解决编译 bug(每次configure后都要执行一次)
sed -i 's/LIBS =  -lm/LIBS =  -lm -lpthread/' Makefile
## 编译
sudo make -j32
# 安装 
sudo make install
## 软链接
ln -sf $BOCHS_ROOT_PATH/bin/bochs /usr/bin/bochs && \
ln -sf $BOCHS_ROOT_PATH/bin/bximage /usr/bin/bximage
```
## 安装辅助工具
```bash
sudo yum install expect dos2unix tree -y
```
## 文本处理
如若是使用 git 下载在 windows 的代码需要文本格式转换
```bash
find . -name Kconfig -print -or -name *.sh -print | xargs -I {} dos2unix {}
```
## 编译常用指令
```bash
# 查看编译帮助手册
make help
# 配置
make config
# 生成镜像
make
# 生成镜像并使用 bochs 运行
make bochsrun
# 清除编译文件
make clean
# 清除编译与配置文件
make distclean
```