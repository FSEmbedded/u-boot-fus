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

include spl/include/autoconf.mk

# NBoot suffix with year and month. Please update for every release. If there
# needs to be more than one release within a month, add .1, .2, .3, ..., e.g.
# 2021.03.2 for the third release in March 2021.
NBOOT_SUFFIX = 2024.08
GIT_VERSION = $(shell $(srctree)/scripts/setlocalversion $(srctree))
NBOOT_VERSION = ${NBOOT_SUFFIX}${GIT_VERSION}

# List of files to add to BOARD-CONFIGS image. The file name depends on the
# names given in the Selectsoft WaWi for the board and the PCB revision. The
# format is:
#
#   <board>-FERT<n>.<rev>.fs
#
# <board>: Name of the board
# <n>:     Subtype of board (i.e. mounting variant)
# <rev>:   Revision number, typically the PCB revision 100, 110, 120, etc.
#
# Group by <board> and <n>, then within each group sort by *descending* <rev>
# number, i.e. add new revisions in front of the list. The first configuration
# where <board> and <n> match and where <rev> is less than or equal to the
# <rev> of the BOARD-ID is selected. This is because not every new board
# revision needs a new software configuration. And a board with a new board
# revision that does not have a its own configuration yet can at least be
# started with an older configuration.
#
# For example assume for a certain board type, there are BOARD-CFGs with
# revisions .130, .120 and .100. Boards with revision .121 or .120 in their
# BOARD-ID would both select .120, a board with .110 would select .100.
BOARD_CONFIGS = \
	PCore93-FERT2.fs \
	PCore93-FERT3.fs \
	PCore93-FERT4.fs \
	PCore93-FERT5.fs \
	OSM93-FERT1.fs \
	OSM93-FERT2.fs \
	OSM93-FERT21.fs \
	OSM93-FERT22.fs \
	OSM93-FERT23.fs 

# List of DRAM Timings for each DRAM-TYPE
RT_LPDDR4 := \
	lpddr4_pc_1g3200m1ch1cs_timing.fs \
	lpddr4_pc_2g3200m1ch1cs_timing.fs \
	lpddr4_osm_1g3733m1ch1cs_timing.fs \
	lpddr4_osm_2g3733m1ch1cs_timing.fs
	
RT_LPDDR4X :=

DDR_FW_VERSION = v202201

# List of provided firmware files from NXP
NXP_LPDDR4 = \
	lpddr4_imem_1d_${DDR_FW_VERSION}.bin \
	lpddr4_dmem_1d_${DDR_FW_VERSION}.bin \
	lpddr4_imem_2d_${DDR_FW_VERSION}.bin \
	lpddr4_dmem_2d_${DDR_FW_VERSION}.bin

NXP_LPDDR4X = 

# List of DRAM-INFO (TYPES) for DRAM-SETTINGS Container; only add those that have timings
DRAM_TYPES :=
ifneq ($(RT_LPDDR4),)
DRAM_TYPES += lpddr4_type.fs
endif
ifneq ($(RT_LPDDR4X),)
DRAM_TYPES += lpddr4x_type.fs
endif

# Prepend the DDR Training Firmware as an additional file before the timings
RT_LPDDR4 := lpddr4_fw.fs $(RT_LPDDR4)
RT_LPDDR4X := lpddr4x_fw.fs $(RT_LPDDR4X)

# List of files to add to EXTRA image
EXTRA_FILES = addfsheader.sh.fs fsimage.sh.fs

# List of files to add to NBOOT Image
NBOOT_FILES = \
	$(BOARD)_boot-info.fs \
	$(BOARD)_board-id.fsh \
	$(BOARD)_board-info.fs \
	$(BOARD)_dram-infos.fs \
	$(BOARD)_extra.fs

# Final NBOOT image
NBOOT = $(BOARD)_nboot.bin

NBOOT_PATH = board/$(BOARDDIR)/nboot
DTS_BOARD_CONFIGS = $(addprefix $(NBOOT_PATH)/, $(BOARD_CONFIGS:.fs,.dts))

# Add object path for F&S images
OBJ_BOARD_CONFIGS = $(addprefix $(NBOOT_PATH)/, $(BOARD_CONFIGS))
OBJ_RT_LPDDR4 = $(addprefix $(NBOOT_PATH)/, $(RT_LPDDR4))
OBJ_RT_LPDDR4X = $(addprefix $(NBOOT_PATH)/, $(RT_LPDDR4X))
OBJ_DRAM_TYPES = $(addprefix $(NBOOT_PATH)/, $(DRAM_TYPES))
OBJ_EXTRA_FILES = $(addprefix $(NBOOT_PATH)/, $(EXTRA_FILES))
OBJ_NBOOT_FILES = $(addprefix $(NBOOT_PATH)/, $(NBOOT_FILES))
OBJ_NBOOT = $(addprefix $(NBOOT_PATH)/, $(NBOOT))

# Add path for NXP firmware images
NXP_PATH = board/F+S/NXP-Firmware
NXP_PATH_LPDDR4 = $(addprefix $(NXP_PATH)/, $(NXP_LPDDR4))
NXP_PATH_LPDDR4X = $(addprefix $(NXP_PATH)/, $(NXP_LPDDR4X))

# MKIMAGE defines
CNTR_DEPFILES := $(srctree)/tools/imx_cntr_image.sh
IMAGE_TYPE := imx8image
CNTR_LOG = cntr.log

### DDR SPECIFIC TARGETS ###
quiet_cmd_ddrfw = DDRFW	$@
cmd_ddrfw= \
	objcopy -I binary -O binary --pad-to 0x8000 --gap-fill=0x0 $(word 1,$^) $(NBOOT_PATH)/.$2_train_1d_imem_pad.tmp; \
	objcopy -I binary -O binary --pad-to 0x4000 --gap-fill=0x0 $(word 2,$^) $(NBOOT_PATH)/.$2_train_1d_dmem_pad.tmp; \
	objcopy -I binary -O binary --pad-to 0x8000 --gap-fill=0x0 $(word 3,$^) $(NBOOT_PATH)/.$2_train_2d_imem_pad.tmp; \
	cat $(NBOOT_PATH)/.$2_train_1d_imem_pad.tmp $(NBOOT_PATH)/.$2_train_1d_dmem_pad.tmp > $(NBOOT_PATH)/.$2_train_1d_fw.tmp; \
	cat $(NBOOT_PATH)/.$2_train_2d_imem_pad.tmp $(word 4,$^) > $(NBOOT_PATH)/.$2_train_2d_fw.tmp; \
	cat $(NBOOT_PATH)/.$2_train_1d_fw.tmp $(NBOOT_PATH)/.$2_train_2d_fw.tmp > $@;

.INTERMEDIATE: $(NBOOT_PATH)/.lpddr4_fw.tmp $(NBOOT_PATH)/.lpddr4x_fw.tmp $(DRAM_TIMINGS_LDS)

$(NBOOT_PATH)/.lpddr4_fw.tmp: $(NXP_PATH_LPDDR4)
	$(call cmd,ddrfw,lpddr4)
	

$(NBOOT_PATH)/.lpddr4x_fw.tmp: $(NXP_PATH_LPDDR4X)
	$(call cmd,ddrfw,lpddr4x)

quiet_cmd_sed = SED     $@
cmd_sed = sed $2 $< > $@

DRAM_IMAGE_ADDR = $(shell printf "%d" 0x204dc000)
DRAM_TIMING_ADDR = $(shell expr ${DRAM_IMAGE_ADDR} + 64 )
DRAM_TIMINGS_LDS = $(NBOOT_PATH)/.dram-timings.lds.tmp
DRAM_TIMINGS_REPLACE_SED_CMD = \
	-e "s/\#\#\#DRAM_TIMING_ADDR\#\#\#/$(shell printf "0x%lx" $(DRAM_TIMING_ADDR))/g"

$(DRAM_TIMINGS_LDS): $(obj)/dram-timings.lds
	$(call cmd,sed,$(DRAM_TIMINGS_REPLACE_SED_CMD))

quiet_cmd_dram_elf = LINK    $@
cmd_dram_elf = $(LD) -T $(DRAM_TIMINGS_LDS) -o $@ $<

$(NBOOT_PATH)/%.o: $(obj)/%.c
	$(Q)$(CC) $(c_flags) -c -o $@ $<

$(NBOOT_PATH)/.%.elf.tmp: $(NBOOT_PATH)/%.o $(DRAM_TIMINGS_LDS)
	$(call cmd,dram_elf)

quiet_cmd_dram_bin = OBJCOPY $@
cmd_dram_bin = $(OBJCOPY) --gap-fill=0xff -O binary $< $@

$(NBOOT_PATH)/.%.bin.tmp: $(NBOOT_PATH)/.%.elf.tmp
	$(call cmd,dram_bin)

quiet_cmd_fsimg = FSIMG   $@
cmd_fsimg = ${srctree}/scripts/addfsheader.sh $2 > $@

$(NBOOT_PATH)/%_timing.fs: $(NBOOT_PATH)/.%_timing.bin.tmp
	$(call cmd,fsimg,-c -w -a 0x400 -p32[7]=$(shell printf "0x%lx" $$(($(CONFIG_SPL_FUS_DRAM_TIMING_ADDR) + 0x40))) -t DRAM-TIMING -d $(*F) $< > $@)

$(NBOOT_PATH)/lpddr4_fw.fs: $(NBOOT_PATH)/.lpddr4_fw.tmp
$(NBOOT_PATH)/lpddr4x_fw.fs: $(NBOOT_PATH)/.lpddr4x_fw.tmp
$(NBOOT_PATH)/%_fw.fs: $(NBOOT_PATH)/.%_fw.tmp
	$(call cmd,fsimg,-c -w -a 0x400 -p32[7]=$(shell printf "0x%lx" $$(($(CONFIG_SPL_FUS_DRAM_FW_ADDR) + 0x40))) -t DRAM-FW -d $(*F) $< > $@ )

LPDDR4_CFG = lpddr4_type.cfg
LPDDR4X_CFG = lpddr4x_type.cfg

MKIMAGEFLAGS_lpddr4_type.cntr = -n ${srctree}/board/$(BOARDDIR)/$(LPDDR4_CFG) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

MKIMAGEFLAGS_lpddr4x_type.cntr = -n ${srctree}/board/$(BOARDDIR)/$(LPDDR4X_CFG) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

$(NBOOT_PATH)/lpddr4_type.cntr:  MKIMAGEOUTPUT = $(NBOOT_PATH)/lpddr4_type$(CNTR_LOG)
$(NBOOT_PATH)/lpddr4x_type.cntr: MKIMAGEOUTPUT = $(NBOOT_PATH)/lpddr4x_type$(CNTR_LOG)
$(NBOOT_PATH)/lpddr4_type.cntr: $(OBJ_RT_LPDDR4) 
$(NBOOT_PATH)/lpddr4x_type.cntr: $(OBJ_RT_LPDDR4X)

$(NBOOT_PATH)/lpddr4_type.cntr: /dev/null ${srctree}/board/$(BOARDDIR)/$(LPDDR4_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(LPDDR4_CFG)
	$(call if_changed,mkimage)

$(NBOOT_PATH)/lpddr4x_type.cntr: /dev/null ${srctree}/board/$(BOARDDIR)/$(LPDDR4X_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(LPDDR4X_CFG)
	$(call if_changed,mkimage)

$(NBOOT_PATH)/lpddr4_type.fs: $(NBOOT_PATH)/lpddr4_type.cntr
	$(eval index_offset := $(shell grep DATA $(NBOOT_PATH)/lpddr4_type${CNTR_LOG} | grep -oP 'file_offset = \K[^ ]+' | head -1))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(call cmd,fsimg, -c -w -a 0x400 -e ${index_offset} -t DRAM-INFO -d lpddr4 $^)

$(NBOOT_PATH)/lpddr4x_type.fs: $(NBOOT_PATH)/lpddr4x_type.cntr
	$(eval index_offset := $(shell grep DATA $(NBOOT_PATH)/lpddr4x_type${CNTR_LOG} | grep -oP 'file_offset = \K[^ ]+' | head -1))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(call cmd,fsimg, -c -w -a 0x400 -e ${index_offset} -t DRAM-INFO -d lpddr4x $^)

$(NBOOT_PATH)/%_dram-infos.fs: $(OBJ_DRAM_TYPES)
	$(Q)cat $(OBJ_DRAM_TYPES) > $@

### END: DDR SPECIFIC TARGETS ###

### BOARD-CFG SPECIFIC TARGETS ###

ifneq ($(KBUILD_SRC),)
prepare_nboot:
	$(Q)cp -a ${KBUILD_SRC}/board/F+S/${BOARD}/nboot/*.dtsi board/F+S/${BOARD}/nboot/
else
prepare_nboot:
endif

# Prepend some macros with varying content to BOARD-CFG .dts
$(NBOOT_PATH)/.%.dts.tmp: $(obj)/%.dts $(NBOOT_PATH)/$(BOARD)_boot-info.fs $(NBOOT_PATH)/nboot-info.dtsi FORCE
	@echo "/* Macros with varying content */" > $@
	@echo "#define NBOOT_VERSION \"$(NBOOT_VERSION)\"" >> $@
	@echo "#define BOARD_CFG_NAME \"$(*F)\"" >> $@
	@echo "#define CONFIG_SPL_BUILD" >> $@
	@echo "#define SPL_SIZE $(shell expr $(shell ls -la $(NBOOT_PATH)/$(BOARD)_boot-info.fs | cut -d ' ' -f 5 ) - 64)" >> $@
	@cat $< >> $@

$(NBOOT_PATH)/.%.dtb.tmp: $(NBOOT_PATH)/.%.dts.tmp
	$(call cmd,dtco)

$(NBOOT_PATH)/%.fs: $(NBOOT_PATH)/.%.dtb.tmp
	$(call cmd,fsimg, -c -w -a 0x400 -p32[7]=$(CONFIG_FUS_BOARDCFG_ADDR) -t BOARD-CFG -d $(*F) $<)

BOARDINFO_CFG = board-info.cfg

MKIMAGEFLAGS_board-info.cntr = -n ${srctree}/board/$(BOARDDIR)/$(BOARDINFO_CFG) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

$(NBOOT_PATH)/board-info.cntr: $(OBJ_BOARD_CONFIGS)
$(NBOOT_PATH)/board-info.cntr: MKIMAGEOUTPUT = $(NBOOT_PATH)/board-info_$(CNTR_LOG)
$(NBOOT_PATH)/board-info.cntr: /dev/null ${srctree}/board/$(BOARDDIR)/$(BOARDINFO_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(BOARDINFO_CFG)
	$(call if_changed,mkimage)

$(NBOOT_PATH)/%_board-info.fs: $(NBOOT_PATH)/board-info.cntr
	$(eval index_offset := $(shell grep DATA $(NBOOT_PATH)/board-info_$(CNTR_LOG) | grep -oP 'file_offset = \K[^ ]+' | head -1))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(call cmd,fsimg, -c -w -a 0x400 -e ${index_offset} -t BOARD-INFO -d $(BOARD) $^)

### END: BOARD-CFG SPECIFIC TARGETS ###

### BOOT-INFO SPECIFIC TARGETS ###

BOOTINFO_CFG = boot-info.cfg

BOOTCNTR_IMGS = $(shell grep IMAGE $(srctree)/board/$(BOARDDIR)/$(BOOTINFO_CFG) | cut -f 3 )
BOOTIMGS_FSH = $(shell echo ${BOOTCNTR_IMGS} | sed -e 's/\.bin/.fsh/g')

BOOTINDEX_FS_OPT = -c -w -a 0x400 -i -t INDEX $^
BOOTIMGS_FS_OPT = -c -a 0x400 -j -t $(call map_boottype,$@) -d $(BOARD) $<

define map_boottype
$(if $(findstring m33_image.fsh,$(1)),CORTEX-M,\
$(if $(findstring u-boot-spl.fsh,$(1)),SPL,\
$(shell basename $(1) | cut -d '.' -f 1 | tr a-z A-Z)))
endef

$(BOOTIMGS_FSH): ${BOOTCNTR_IMGS}
%.fsh: %.bin
	$(call cmd,fsimg,$(BOOTIMGS_FS_OPT))

$(NBOOT_PATH)/boot-index.fs: $(BOOTIMGS_FSH)
	$(call cmd,fsimg,$(BOOTINDEX_FS_OPT))

MKIMAGEFLAGS_boot-info.cntr = -n ${srctree}/board/$(BOARDDIR)/$(BOOTINFO_CFG) \
	-T $(IMAGE_TYPE) -e $(CONFIG_TEXT_BASE)

$(NBOOT_PATH)/boot-info.cntr: $(NBOOT_PATH)/boot-index.fs
$(NBOOT_PATH)/boot-info.cntr: MKIMAGEOUTPUT = $(NBOOT_PATH)/boot-info_$(CNTR_LOG)
$(NBOOT_PATH)/boot-info.cntr: /dev/null ${srctree}/board/$(BOARDDIR)/$(BOOTINFO_CFG)
	$(Q)$(CNTR_DEPFILES) ${srctree}/board/$(BOARDDIR)/$(BOOTINFO_CFG)
	$(call if_changed,mkimage)

# Do not align the binary. The correct Padding is done in boot-info.cntr
# The Padding recuires BOARD-ID.fs directly after boot-info.fs
$(NBOOT_PATH)/%_boot-info.fs: $(NBOOT_PATH)/boot-info.cntr
	$(eval index_offset := $(shell grep -e DATA -e M33 -e AP $(NBOOT_PATH)/boot-info_$(CNTR_LOG) | grep -oP 'file_offset = \K[^ ]+' | head -1))
	$(eval index_offset := $(shell printf "%d\n" "${index_offset}"))
	$(call cmd,fsimg, -c -w -a 0x400-0x40 -e ${index_offset} -t BOOT-INFO -d $(BOARD) $^)

### END: BOOT-INFO SPECIFIC TARGETS ###

### NBOOT TARGETS ###
$(NBOOT_PATH)/addfsheader.sh.fs: $(srctree)/scripts/addfsheader.sh
$(NBOOT_PATH)/fsimage.sh.fs: $(srctree)/scripts/fsimage.sh
%.sh.fs:
	$(call cmd,fsimg, -t BASH-SCRIPT -d $(*F) $^)

%_extra.fs: $(OBJ_EXTRA_FILES)
	$(call cmd,fsimg, -t EXTRA -a 0x400-0x40 -d $(*F) $^)

$(NBOOT_PATH)/$(BOARD)_board-id.fsh:
	${Q}dd if=/dev/zero bs=1 count=0 status=none | $(srctree)/scripts/addfsheader.sh -j -t BOARD-ID -d DUMMY-ID > $@

nboot-$(BOARD)-$(NBOOT_SUFFIX).fs: $(OBJ_NBOOT_FILES)
	$(Q)cat $(OBJ_NBOOT_FILES) > $@

nboot.fs: nboot-$(BOARD)-$(NBOOT_SUFFIX).fs
	$(Q)cp $< nboot.fs

PHONY += nboot prepare_nboot
nboot: prepare_nboot nboot.fs
	@echo "Finished doing nboot"
### END: NBOOT TARGETS ###
