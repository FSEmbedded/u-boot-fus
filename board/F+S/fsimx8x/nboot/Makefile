# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 F&S Elektronik Systeme GmbH
#

include spl/include/autoconf.mk

# NBoot version with year and month. Please update for every release. If there
# needs to be more than one release within a month, add .1, .2, .3, ..., e.g.
# 2021.03.2 for the third release in March 2021.
NBOOT_VERSION = 2025.07

# List of files to add to BOARD-CONFIGS image. The file name depends on the
# names given in the Selectsoft WaWi for the board, without the PCB revision.
# The format is:
#
#   <board>-FERT<n>.fs
#
# <board>: Name of the board
# <n>:     Subtype of board (i.e. mounting variant)
BOARD_CONFIGS = \
	EFUSMX8X-FERT1.fs

# List of DRAM Timings for each DRAM-TYPE
RT_DDR3 :=
RT_LPDDR4 := \
	lpddr4_k4f8e3s4hd_mgcl_timing.fs \
	lpddr4_fl4c2001g_d9_timing.fs

# List of DRAM-TYPES for DRAM-SETTINGS image; only add those that have timings
DRAM_TYPES :=
ifneq ($(RT_LPDDR4),)
DRAM_TYPES += lpddr4_type.fs
endif
ifneq ($(RT_DDR3),)
DRAM_TYPES += ddr3_type.fs
endif

# List of files to add to FIRMWARE image
ifeq ($(CONFIG_IMX_OPTEE),y)
FIRMWARE_FILES = $(BOARD)_dram_info.fs $(BOARD)_atf.fs $(BOARD)_optee.fs
else
FIRMWARE_FILES = $(BOARD)_dram_info.fs $(BOARD)_atf.fs
endif

# List of files to add to EXTRA image
EXTRA_FILES = addfsheader.sh.fs fsimage.sh.fs

# List of files to add to NBOOT image
NBOOT_FILES = $(BOARD)_spl.fs $(BOARD)_board_info.fs \
	$(BOARD)_firmware.fs $(BOARD)_extra.fs

# Final NBOOT image
NBOOT = $(BOARD)_nboot.fs

# Add source tree path for board configurations
NBOOT_PATH = $(srctree)/board/$(BOARDDIR)/nboot
DTS_BOARD_CONFIGS = $(addprefix $(NBOOT_PATH)/, $(BOARD_CONFIGS:.fs,.dts))

# Add object path for F&S images
OBJ_BOARD_CONFIGS = $(addprefix $(obj)/, $(BOARD_CONFIGS))
OBJ_RT_DDR3 = $(addprefix $(obj)/, $(RT_DDR3))
OBJ_RT_LPDDR4 = $(addprefix $(obj)/, $(RT_LPDDR4))
OBJ_DRAM_TYPES = $(addprefix $(obj)/, $(DRAM_TYPES))
OBJ_FIRMWARE_FILES = $(addprefix $(obj)/, $(FIRMWARE_FILES))
OBJ_EXTRA_FILES = $(addprefix $(obj)/, $(EXTRA_FILES))
OBJ_NBOOT_FILES = $(addprefix $(obj)/, $(NBOOT_FILES))
OBJ_NBOOT = $(addprefix $(obj)/, $(NBOOT))

# List of provided firmware files from NXP
NXP_DDR3 = ddr3_imem_1d.bin ddr3_dmem_1d.bin
NXP_LPDDR4 = lpddr4_pmu_train_1d_imem.bin lpddr4_pmu_train_1d_dmem.bin \
	lpddr4_pmu_train_2d_imem.bin lpddr4_pmu_train_2d_dmem.bin
NXP_ATF = bl31.bin

# Add path for NXP firmware images
NXP_PATH = $(srctree)/board/F+S/NXP-Firmware/$(BOARD)
NXP_PATH_DDR3 = $(addprefix $(NXP_PATH)/, $(NXP_DDR3))
NXP_PATH_LPDDR4 = $(addprefix $(NXP_PATH)/, $(NXP_LPDDR4))
NXP_PATH_ATF = $(addprefix $(NXP_PATH)/, $(NXP_ATF))

quiet_cmd_sed = SED     $@
cmd_sed = sed $2 $< > $@

DRAM_TIMINGS_LDS = $(obj)/.dram-timings.lds.tmp
DRAM_TIMINGS_REPLACE_SED_CMD = \
	-e "s/\#\#\#DRAM_TIMING_ADDR\#\#\#/$(CONFIG_SPL_DRAM_TIMING_ADDR)/g"

$(DRAM_TIMINGS_LDS): $(NBOOT_PATH)/dram-timings.lds ${srctree}/include/configs/$(BOARD).h
	$(call cmd,sed,$(DRAM_TIMINGS_REPLACE_SED_CMD))

quiet_cmd_dram_elf = LINK    $@
cmd_dram_elf = $(LD) -T $(DRAM_TIMINGS_LDS) -o $@ $<
#cmd_dram_elf = $(LD) -T $(DRAM_TIMINGS_LDS) -Map $*.map -o $@ $<

$(obj)/.%.elf.tmp: $(obj)/%.o $(DRAM_TIMINGS_LDS)
	$(call cmd,dram_elf)

quiet_cmd_dram_bin = OBJCOPY $@
cmd_dram_bin = $(OBJCOPY) --gap-fill=0xff -O binary $< $@

$(obj)/.%.bin.tmp: $(obj)/.%.elf.tmp
	$(call cmd,dram_bin)

quiet_cmd_fsimg = FSIMG   $@
cmd_fsimg = ${srctree}/scripts/addfsheader.sh -q -a 16 $2 $^ > $@

$(obj)/%_timing.fs: $(obj)/.%_timing.bin.tmp
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_ATF_ADDR) -t DRAM-TIMING -d $(*F))

$(obj)/%_fw.fs: $(obj)/.%_fw.tmp
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_ATF_ADDR) -t DRAM-FW -d $(*F))

$(obj)/ddr3_type.fs: $(OBJ_RT_DDR3)
$(obj)/lpddr4_type.fs: $(OBJ_RT_LPDDR4)

%_type.fs:
	$(call cmd,fsimg,-s -t DRAM-TYPE -d $(*F))

%_dram_info.fs: $(OBJ_DRAM_TYPES)
	$(call cmd,fsimg,-s -t DRAM-INFO -d $(*F))

%_atf.fs: $(NXP_PATH_ATF)
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_ATF_ADDR) -t ATF -d $(*F))

%_optee.fs: $(NXP_PATH_TEE)
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_TEE_ADDR) -t TEE -d $(*F))

%_firmware.fs: $(OBJ_FIRMWARE_FILES)
	$(call cmd,fsimg,-s -t FIRMWARE -d $(*F))

%_spl.fs: SPL
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_ATF_ADDR) -t SPL -d $(*F))

# Prepend some macros with varying content to BOARD-CFG .dts
$(obj)/.%.dts.tmp: $(NBOOT_PATH)/%.dts
	@echo "/* Macros with varying content */" > $@
	@echo "#define NBOOT_VERSION \"$(NBOOT_VERSION)\"" >> $@
	@echo "#define BOARD_CFG_NAME \"$(*F)\"" >> $@
	@echo "#define CONFIG_SPL_BUILD" >> $@
	@cat $< >> $@

$(obj)/.%.dtb.tmp: $(obj)/.%.dts.tmp $(NBOOT_PATH)/nboot-info.dtsi
	$(call cmd,dtco)

$(obj)/%.fs: $(obj)/.%.dtb.tmp
	$(call cmd,fsimg,-s -c -p32[7]=$(CFG_FUS_BOARDCFG_ADDR) -t BOARD-CFG -d $(*F))

%_board_info.fs: $(OBJ_BOARD_CONFIGS)
	$(call cmd,fsimg,-s -t BOARD-INFO -d $(*F))

$(obj)/addfsheader.sh.fs: $(srctree)/scripts/addfsheader.sh
$(obj)/fsimage.sh.fs: $(srctree)/scripts/fsimage.sh
%.sh.fs:
	$(call cmd,fsimg,-t BASH-SCRIPT -d $(*F))

%_extra.fs: $(OBJ_EXTRA_FILES)
	$(call cmd,fsimg,-t EXTRA -d $(*F))

nboot-$(BOARD)-$(NBOOT_VERSION).fs: $(OBJ_NBOOT_FILES)
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SYS_LOAD_ADDR) -t NBOOT -d $(BOARD))

PHONY += nboot
nboot: nboot-$(BOARD)-$(NBOOT_VERSION).fs
	@echo "Finished doing nboot"
