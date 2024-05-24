# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2024 F&S Elektronik Systeme GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

obj-y += fsimx93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_FSIMX93) += lpddr4_pc_MT53D512M16D1_timing.o
endif

### UBOOT-INFO CREATION ###

ifeq ($(CONFIG_SPL_LOAD_IMX_CONTAINER), y)
CNTR_DEPFILES := $(srctree)/tools/imx_cntr_image.sh
IMAGE_TYPE := imx8image

UBOOTCNTR_CFG = $(CONFIG_FUS_UBOOTCNTR_CONFIG:"%"=%)

MKIMAGEFLAGS_uboot-info.cntr.tmp = -n $(filter-out $(PLUGIN).cntr $< $(PHONY),$^) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

CNTR_LOG = uboot-info.cntr.log
uboot-info.cntr.tmp:  MKIMAGEOUTPUT = $(CNTR_LOG)

endif #CONFIG_SPL_LOAD_IMX_CONTAINER

CNTR_IMGS = $(shell grep IMAGE $(srctree)/board/$(BOARDDIR)/$(UBOOTCNTR_CFG) | cut -f 3 )
IMGS_FSH = $(shell echo ${CNTR_IMGS} | sed -e 's/\.bin/.fsh/g')

define map_type
$(if $(findstring bl31.fsh,$(1)),ATF,\
$(if $(findstring bl32.fsh,$(1)),TEE,\
$(if $(findstring u-boot.fsh,$(1)),UBOOT,\
$(shell basename $(1) | cut -d '.' -f 1 | tr a-z A-Z))))
endef

IMGS_FS_OPT = -a 0x400 -j -t $(call map_type,$@) -d $(BOARD) $<
INDEX_FS_OPT = -w -a 0x400 -i -t INDEX $^ 
INFO_FS_OPT = -t U-BOOT-INFO -d ${BOARD} $^

quiet_cmd_addfsheader = FSIMG   $@
cmd_addfsheader = $(srctree)/scripts/addfsheader.sh $2 > $@

$(IMGS_FSH): ${CNTR_IMGS}
	$(call cmd,addfsheader,$(IMGS_FS_OPT))

uboot-index.fs.tmp: $(IMGS_FSH)
	$(call cmd,addfsheader,$(INDEX_FS_OPT))

uboot-info.cntr.tmp: uboot-index.fs.tmp ${srctree}/board/$(BOARDDIR)/$(UBOOTCNTR_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(UBOOTCNTR_CFG)
	$(call if_changed,mkimage)

uboot-header.fsh: uboot-info.cntr.tmp
	$(eval index_offset := $(shell grep DATA ${CNTR_LOG} | grep -oP 'file_offset = \K[^ ]+'))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(Q)head -c ${index_offset} $< | ${srctree}/scripts/addfsheader.sh -j -t HEADER > $@

uboot-info.fs: uboot-header.fsh uboot-info.cntr.tmp
	$(call cmd,addfsheader,$(INFO_FS_OPT))

### END: UBOOT-INFO CREATION ###
