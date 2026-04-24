#
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	+= $(BOARD).o

uboot_sfx_obj := $(obj)/../common/sfx_armv7.o \
	$(obj)/../common/unlzma_uboot.o

LDFLAGS_.uboot.sfx.tmp = -T $(src)/sfx_uboot.lds
.uboot.sfx.tmp: $(uboot_sfx_obj)
	$(call if_changed,ld)

OBJCOPYFLAGS_uboot.sfx = -j .uboot_sfx -j .ubootcomp -O binary
uboot.sfx: .uboot.sfx.tmp
	$(call if_changed,objcopy)
