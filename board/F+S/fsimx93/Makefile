# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 F&S Elektronik Systeme GmbH
#

include board/$(VENDOR)/common/fus-common.mk

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
else
obj-y += fsimx93.o
endif

# Image type for mkimage
MKIMAGE_TYPE = imx8image

# -----------------------------------------------------------------------------
#
# Rules for uboot.fs.
#
# Data flow:
# 1. bl31.bin/bl31-optee.bin --> (cp) --> <arch>_ATF.fsi
# 2. bl32.bin --> (cp) --> <arch>_TEE.fsi
# 3. u-boot.bin --> (cp) --> <arch>_UBOOT.fsi
# 4. <arch>_ATF.fsi + <arch>_TEE.fsi + <arch>_UBOOT.fsi --> (fsc) --> uboot.fs

ifeq ($(CONFIG_OPTEE),y)
UBOOT_ATF = $(NXP_PATH)/bl31-optee.bin
else
UBOOT_ATF = $(NXP_PATH)/bl31.bin
endif

# 1. Copy ATF file to desired filename
$(obj)/%_ATF.fsi: $(UBOOT_ATF)
	$(call cmd,copy)

# 2. Copy opTEE file to desired filename
$(obj)/%_TEE.fsi: $(NXP_PATH)/bl32.bin
	$(call cmd,copy)

# 3. Copy U-Boot to desired filename
$(obj)/%_U-BOOT.fsi: u-boot.bin
	$(call cmd,copy)

# 4. Build F&S container; delete TEE image line from .cfg if not using opTEE
ifneq ($(CONFIG_OPTEE),y)
SED_UBOOT_EXTRA = -e '/^IMAGE.*TEE/d'
endif

$(eval $(call fsc,uboot,-t U-BOOT-INFO -d $(BOARD),$(SED_UBOOT_EXTRA)))

uboot.fs: $(obj)/uboot.fs
	$(call cmd,copy)
