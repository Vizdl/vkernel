BUILD_DIR = ./build
ENTRY_POINT = 0x100000
AS = nasm
CC = gcc
LD = ld
LIB = -I boot/multiboot2/
BOOTHEADER =
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes \
         -Wmissing-prototypes -m32
LDFLAGS = -Ttext $(ENTRY_POINT) -e _start -Map $(BUILD_DIR)/kernel.map -m elf_i386
OBJS = $(BUILD_DIR)/boot.o $(BUILD_DIR)/multiboot2.o 
	
$(BUILD_DIR)/boot.o: boot/multiboot2/boot.S boot/multiboot2/multiboot2.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/multiboot2.o: boot/multiboot2/multiboot2.c boot/multiboot2/multiboot2.h
	$(CC) $(CFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir proc_scripts clean all mk_boot_img mk_hd80M run

proc_scripts:scripts
	find scripts/ -name *.sh | xargs -I {} dos2unix {} && chmod 755 scripts -R

mk_boot_img:mk_dir proc_scripts
	if [[ ! -f $(BUILD_DIR)/hd60M.img ]]; \
	then \
		scripts/mk_hd.sh $(BUILD_DIR) 60 && \
		scripts/fomat_hd.sh $(BUILD_DIR)/hd60M.img ${BUILD_DIR}/kernel.bin boot/multiboot2/grub boot/multiboot2/grub.cfg; \
	fi

mk_hd80M:mk_dir proc_scripts
	scripts/mk_hd.sh $(BUILD_DIR) 80

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build mk_hd80M mk_boot_img

run: all config/bochs/bochsrc
	bochs -qf config/bochs/bochsrc