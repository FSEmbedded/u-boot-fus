# fsimage is supposed to run on the target system, which means it should be
# built with cross tools., which is why "HOSTCC" is replaced with "CC".
# Everything else is taken from scripts/Makefile.host.
override HOSTCC = $(CC)

# Compile for a hosted environment on the target
HOST_EXTRACFLAGS  = -I$(srctree)/tools \
		$(patsubst -I%,-idirafter %, $(filter -I%, $(UBOOTINCLUDE))) \
		-idirafter $(srctree)/include \
		-idirafter $(srctree)/tools/fsimage \
		-idirafter $(srctree)/board/$(VENDOR)/common \
		-DUSE_HOSTCC \
		-DTEXT_BASE=$(TEXT_BASE) -D__KERNEL__

always := fsimage
hostprogs-y := fsimage

lib-y += fsimage_linux.o fdt.o fdt_ro.o fdt_rw.o fdt_wip.o crc32.o

lib-$(CONFIG_FS_BOARD_COMMON) += fs_board_common.o
lib-$(CONFIG_FS_CNTR_COMMON) += fs_cntr_common.o
lib-$(CONFIG_FS_IMAGE_COMMON) += \
	fs_image_common.o fs_image_nonspl.o fsimage_cmd.o

fsimage-objs := $(lib-y)

quiet_cmd_crosstools_strip = STRIP   $^
      cmd_crosstools_strip = $(STRIP) $^; touch $@

$(obj)/.strip: $(obj)/fsimage
	$(call cmd,crosstools_strip)

always += .strip
