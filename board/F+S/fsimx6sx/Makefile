#
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	+= $(BOARD).o

OBJ_COMMON = $(obj)/../common

uboot-sfx-obj-y := $(OBJ_COMMON)/sfx_armv7.o \
	$(OBJ_COMMON)/sfx_uboot.o

uboot-sfx-obj-$(CONFIG_FS_SFX_UBOOT_LZMA) += $(OBJ_COMMON)/unlzma_uboot.o

ifeq ($(CONFIG_FS_SFX_UBOOT_ZSTD),y)
uboot-sfx-obj-y += \
	$(OBJ_COMMON)/unzstd_uboot.o \
	$(OBJ_COMMON)/zstd/decompress/zstd_decompress.o \
	$(OBJ_COMMON)/zstd/decompress/zstd_decompress_block.o \
	$(OBJ_COMMON)/zstd/decompress/huf_decompress.o \
	$(OBJ_COMMON)/zstd/decompress/zstd_ddict.o \
	$(OBJ_COMMON)/zstd/common/zstd_common.o \
	$(OBJ_COMMON)/zstd/common/entropy_common.o \
	$(OBJ_COMMON)/zstd/common/fse_decompress.o \
	$(OBJ_COMMON)/zstd/common/error_private.o \
	$(OBJ_COMMON)/zstd/xxhash.o \
	$(objtree)/arch/arm/lib/memset.o \
	$(objtree)/arch/arm/lib/memcpy.o \
	$(objtree)/arch/arm/lib/lib1funcs.o

uboot-sfx-obj-$(CONFIG_USE_ARCH_MEMMOVE) += $(objtree)/arch/arm/lib/memmove.o
endif

LDFLAGS_.uboot.sfx.tmp = -T $(src)/sfx_uboot.lds
.uboot.sfx.tmp: $(uboot-sfx-obj-y)
	$(call if_changed,ld)

OBJCOPYFLAGS_uboot.sfx = -j .uboot_sfx -j .ubootcomp -O binary
uboot.sfx: .uboot.sfx.tmp
	$(call if_changed,objcopy)
