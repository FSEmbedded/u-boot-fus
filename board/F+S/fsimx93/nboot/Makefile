# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 F&S Elektronik Systeme GmbH
#
# Build NBoot image as a set of F&S images.
#
#  +---------------------+
#  | BOOT-INFO           | (Container)
#  +---------------------+
#  | BOARD-ID            |
#  +---------------------+
#  | BOARD-INFO          | (Container)
#  +---------------------+
#  | DRAM-TYPE (lpddr4)  | (Container)
#  +---------------------+
#  | DRAM-TYPE (lpddr4x) | (Container)
#  +---------------------+
#  | EXTRA               |
#  +---------------------+
#
# For the layout of a container image, see board/F+S/common/fus-common.mk.
# A detailed layout of the NBoot image is in board/F+S/common/fs_cntr_common.c.

include spl/include/autoconf.mk
include board/$(VENDOR)/common/fus-common.mk

# NBoot suffix with year and month. Please update for every release. If there
# needs to be more than one release within a month, add .1, .2, .3, ..., e.g.
# 2021.03.2 for the third release in March 2021.
NBOOT_SUFFIX = 2025.07

# Image type for mkimage
MKIMAGE_TYPE = imx8image

# - New BOARD-CFGs are added in board_info.cfg.
# - New DRAM-TIMINGs are added in lpddr4(x)_info.cfg)

# -----------------------------------------------------------------------------
#
# Rules for boot_info.fs.
#
# Data flow:
# 1. u-boot-spl.bin -> (cp) -> fsimx93_SPL.fsi
# 1. fsimx93_SPL.fsi -> (fsc) -> boot_info.fs
#
# TODO: Add optional Cortex-M image

$(obj)/%_SPL.fsi: spl/u-boot-spl.bin
	$(call cmd,copy)

$(eval $(call fsc,boot_info,-a 0x400-0x40 -t BOOT-INFO -d $(BOARD)))

# -----------------------------------------------------------------------------
#
# Rules for board_id.fsh.

$(obj)/board_id.fsh:
	${Q}cat /dev/null | $(FSIMG) -q -j -t BOARD-ID -d DUMMY-ID > $@

# -----------------------------------------------------------------------------
#
# Rules for board_info.fs.
#
# Data flow:
# 1. Dynamic macros + <cfg>.dts --> (cat) --> .<cfg>.dts.tmp
# 2. .<cfg>.dts.tmp + nboot-info.dtsi --> (dtc) --> <cfg>_BOARD-CFG.fsi
# 3. *_BOARD-CFG.fsi --> (fsc) --> board_info.fs

GIT_VERSION = $(shell $(srctree)/scripts/setlocalversion $(srctree))
NBOOT_VERSION = ${NBOOT_SUFFIX}${GIT_VERSION}

# 1. Prepend some macros with varying content to .dts file of each board cfg
quiet_cmd_cfg_dts_tmp = CAT     $@
      cmd_cfg_dts_tmp = \
	(echo "/* Macros with varying content */"; \
	echo "\#define NBOOT_VERSION \"$(NBOOT_VERSION)\""; \
	echo "\#define BOARD_CFG_NAME \"$(*F)\""; \
	echo "\#define SPL_SIZE $$(($$(stat -c %s $(obj)/boot_info.fs)))"; \
	cat $<) > $@

$(obj)/.%.dts.tmp: $(SRC_PATH)/%.dts $(obj)/boot_info.fs
	$(call cmd,cfg_dts_tmp)

# 2. Call device tree compiler (dtc) to build dtb files
$(obj)/%_BOARD-CFG.fsi: $(obj)/.%.dts.tmp
	$(call cmd,dtco)

# 3. Build F&S container image
$(eval $(call fsc,board_info,-a 0x400 -t BOARD-INFO -d $(BOARD)))

# -----------------------------------------------------------------------------
#
# Rules for lpddr4_info.fs/lpddr4x_info.fs
#
# Data flow (<t> is either lpddr4 or lpddr4x):
# 1. dram-timings.lds --> (sed) --> .dram-timings.lds.tmp
# 2. <t>_imem_1d/dmem_1d/imem_2d/dmem_2d.bin --> .<t>_type-fw.bin.tmp
# 3. <t>_<timing>_timing.c --> (gcc) --> <t>_<timing>_timing.o
# 4. <t>_<timing>_timing.o -> (ld) -> .<t>_<timing>_timing.elf.tmp
# 5. .<t>_<timing>_timing.elf.tmp --> (objcopy) --> <t>_DRAM-TIMING.fsi
# 6. *_DRAM-TIMING.fsi --> (fsc) --> <t>_info.fs

LPDDR4_FW_FILES = \
	$(NXP_PATH)/lpddr4_imem_1d_v202201.bin \
	$(NXP_PATH)/lpddr4_dmem_1d_v202201.bin \
	$(NXP_PATH)/lpddr4_imem_2d_v202201.bin \
	$(NXP_PATH)/lpddr4_dmem_2d_v202201.bin
LPDDR4_CFG = $(srctree)/$(src)/lpddr4_info.cfg

$(obj)/lpddr4_DRAM-FW.fsi: $(LPDDR4_FW_FILES)

### FIXME: Substitute the correct lpddr4x files
LPDDR4X_FW_FILES = \
	$(NXP_PATH)/lpddr4_imem_1d_v202201.bin \
	$(NXP_PATH)/lpddr4_dmem_1d_v202201.bin \
	$(NXP_PATH)/lpddr4_imem_2d_v202201.bin \
	$(NXP_PATH)/lpddr4_dmem_2d_v202201.bin
LPDDR4X_CFG = $(srctree)/$(src)/lpddr4x_info.cfg

$(obj)/lpddr4x_DRAM-FW.fsi: $(LPDDR4X_FW_FILES)

# 1. Build final linker script by replacing ###DRAM_TIMING_ADDR### with value
DRAM_TIMING_ADDR = 0x204dc000
DRAM_TIMINGS_LDS = $(obj)/.dram-timings.lds.tmp
DRAM_TIMINGS_REPLACE_SED_CMD = \
	-e "s/\#\#\#DRAM_TIMING_ADDR\#\#\#/$(DRAM_TIMING_ADDR)/g"

$(DRAM_TIMINGS_LDS): $(SRC_PATH)/dram-timings.lds
	$(call cmd,sed,$(DRAM_TIMINGS_REPLACE_SED_CMD))

# 2. Prepare DRAM firmware by padding and concatenating single firmware files
quiet_cmd_ddrfw = DDRFW   $@
      cmd_ddrfw= \
	dd if=$(word 1,$^) of=$@ bs=16k status=none; \
	dd if=$(word 2,$^) of=$@ bs=16k seek=2 conv=notrunc status=none; \
	dd if=$(word 3,$^) of=$@ bs=16k seek=3 conv=notrunc status=none; \
	dd if=$(word 4,$^) of=$@ bs=16k seek=5 conv=notrunc status=none

$(obj)/%_DRAM-FW.fsi:
	$(call cmd,ddrfw)

# 3. Compile DRAM timings
$(obj)/%_timing.o: TYPE = $(*F).x
$(obj)/%_timing.o: $(SRC_PATH)/%_timing.c
	$(call cmd,dram_cc)

# 4. Link DRAM timings with our own linker skript
$(obj)/.%_timing.elf.tmp: $(obj)/%_timing.o $(DRAM_TIMINGS_LDS)
	$(call cmd,dram_elf,$(DRAM_TIMINGS_LDS))

# 5. Convert timings from ELF to binary format
$(obj)/%_DRAM-TIMING.fsi: $(obj)/.%_timing.elf.tmp
	$(call cmd,dram_bin)

# 6. Create lpddr4_info.fs/lpddr4x_info.fs
ifneq ($(wildcard $(LPDDR4_CFG)),)
$(eval $(call fsc,lpddr4_info,-a 0x400 -t DRAM-INFO -d lpddr4))
endif

ifneq ($(wildcard $(LPDDR4X_CFG)),)
$(eval $(call fsc,lpddr4x_info,-a 0x400 -t DRAM-INFO -d lpddr4x))
endif

# -----------------------------------------------------------------------------
#
# Rules for extra.fs. This is not a container image, just a simple F&S image.
#
# Data flow:
# 1. <script>.sh --> addfsheader --> .<script>.sh.fs.tmp
# 2. .<script>.sh.fs.tmp --> addfsheader --> extra.fs

EXTRA_FILES = addfsheader.sh fsimage.sh
EXTRA_FILES_OBJ = $(addprefix $(obj)/,$(addsuffix .fs,$(EXTRA_FILES)))

$(obj)/%.sh.fs: $(srctree)/scripts/%.sh
	$(call cmd,fsimg, -t BASH-SCRIPT -d $(*F) $<)

$(obj)/extra.fs: $(EXTRA_FILES_OBJ)
	$(call cmd,fsimg, -t EXTRA -a 0x400-0x40 -d $(BOARD) $^)

# -----------------------------------------------------------------------------
#
# Rules for nboot.fs. This consists of a group of F&S images.
#
# Data flow:
# 1. $(NBOOT_FILES) --> (cat) --> nboot-<arch>-<vers>.fs
# 2. nboot-<arch>-<vers>.fs --> (cp) --> nboot.fs

NBOOT_FILES = \
	$(obj)/boot_info.fs \
	$(obj)/board_id.fsh \
	$(obj)/board_info.fs \
	$(if $(wildcard $(LPDDR4_CFG)),$(obj)/lpddr4_info.fs) \
	$(if $(wildcard $(LPDDR4X_CFG)),$(obj)/lpddr4x_info.fs) \
	$(obj)/extra.fs

nboot-$(BOARD)-$(NBOOT_SUFFIX).fs: $(NBOOT_FILES)
	$(call cmd,cat)

nboot.fs: nboot-$(BOARD)-$(NBOOT_SUFFIX).fs
	$(call cmd,copy)

PHONY += nboot
nboot: nboot.fs
	@echo "Finished doing nboot"
