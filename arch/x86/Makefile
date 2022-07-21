KBUILD_CFLAGS += -m32
KBUILD_AFLAGS += -m32
LDFLAGS += -m elf_i386
obj-y += boot/

PHONY += vimage

all: vimage

vimage: vkernel
	if [[ ! -f $(KBUILD_IMAGE).img ]]; \
	then \
		scripts/mk_hd.sh ./$(KBUILD_IMAGE).img 60 && \
		scripts/fomat_hd.sh \
		$(KBUILD_IMAGE).img \
		$(KBUILD_IMAGE) \
		arch/$(ARCH)/boot/grub \
		arch/$(ARCH)/boot/grub.cfg; \
	fi

PHONY += bochsrun

bochsrun: vimage
	bochs -qf arch/$(ARCH)/boot/bochsrc

define archhelp
  echo  '* vimage      - vkernel image (arch/x86/vimage)'
  echo  '  bochsrun    - run vkernel using bochs'
endef