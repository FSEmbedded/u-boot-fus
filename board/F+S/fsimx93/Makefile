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


ifdef CONFIG_SPL_BUILD
obj-y += spl.o
else
obj-y += fsimx93.o
endif

### UBOOT-INFO CREATION ###
CNTR_DEPFILES := $(srctree)/tools/imx_cntr_image.sh
IMAGE_TYPE := imx8image

UBOOTCNTR_CFG = $(CONFIG_FUS_UBOOTCNTR_CONFIG:"%"=%)

MKIMAGEFLAGS_uboot-info.cntr = -n ${srctree}/board/$(BOARDDIR)/$(UBOOTCNTR_CFG) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

CNTR_LOG = board/$(BOARDDIR)/uboot-info.cntr.log
board/$(BOARDDIR)/uboot-info.cntr:  MKIMAGEOUTPUT = $(CNTR_LOG)

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

quiet_cmd_addfsheader = FSIMG   $@
cmd_addfsheader = $(srctree)/scripts/addfsheader.sh $2 > $@

$(IMGS_FSH): ${CNTR_IMGS}
%.fsh: %.bin
	$(call cmd,addfsheader,$(IMGS_FS_OPT))

board/$(BOARDDIR)/uboot-index.fs: $(IMGS_FSH)
	$(call cmd,addfsheader,$(INDEX_FS_OPT))

board/$(BOARDDIR)/uboot-info.cntr: board/$(BOARDDIR)/uboot-index.fs ${srctree}/board/$(BOARDDIR)/$(UBOOTCNTR_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(UBOOTCNTR_CFG)
	$(call if_changed,mkimage)

uboot-info.fs: board/$(BOARDDIR)/uboot-info.cntr
	$(eval index_offset := $(shell grep DATA ${CNTR_LOG} | grep -oP 'file_offset = \K[^ ]+'))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(call cmd,addfsheader,-a 0x400 -e ${index_offset} -t U-BOOT-INFO -d ${BOARD} $^)

### END: UBOOT-INFO CREATION ###
