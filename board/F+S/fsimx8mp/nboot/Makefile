# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 F&S Elektronik Systeme GmbH
#

include spl/include/autoconf.mk

# NBoot version with year and month. Please update for every release. If there
# needs to be more than one release within a month, add .1, .2, .3, ..., e.g.
# 2021.03.2 for the third release in March 2021.
NBOOT_VERSION = 2025.12

# List of files to add to BOARD-CONFIGS image. The file name depends on the
# names given in the Selectsoft WaWi for the board, without the PCB revision.
# The format is:
#
#   <board>-FERT<n>.fs
#
# <board>: Name of the board
# <n>:     Subtype of board (i.e. mounting variant)
BOARD_CONFIGS = \
	PCoreMX8MP-FERT1.fs \
	PCoreMX8MP-FERT2.fs \
	PCoreMX8MP-FERT3.fs \
	PCoreMX8MP-FERT4.fs \
	PCoreMX8MP-FERT5.fs \
	PCoreMX8MP-FERT6.fs \
	PCoreMX8MPr2-FERT0.fs \
	PCoreMX8MPr2-FERT1.fs \
	PCoreMX8MPr2-FERT2.fs \
	PCoreMX8MPr2-FERT3.fs \
	PCoreMX8MPr2-FERT4.fs \
	PCoreMX8MPr2-FERT5.fs \
	PCoreMX8MPr2-FERT6.fs \
	PCoreMX8MPr2-FERT7.fs \
	PCoreMX8MPr2-FERT8.fs \
	PCoreMX8MPr2-FERT9.fs \
	PCoreMX8MPr2-FERT10.fs \
	PCoreMX8MPr2-FERT11.fs \
	PCoreMX8MPr2-FERT12.fs \
	PCoreMX8MPr2-FERT13.fs \
	PCoreMX8MPr2-FERT14.fs \
	armStoneMX8MP-FERT0.fs \
	armStoneMX8MP-FERT2.fs \
	armStoneMX8MP-FERT4.fs \
	armStoneMX8MP-FERT5.fs \
	armStoneMX8MP-FERT6.fs \
	armStoneMX8MP-FERT7.fs \
	armStoneMX8MP-FERT8.fs \
	armStoneMX8MP-FERT9.fs \
	armStoneMX8MP-FERT10.fs \
	armStoneMX8MP-FERT22.fs \
	armStoneMX8MP-FERT24.fs \
	efusMX8MP-FERT3.fs \
	efusMX8MP-FERT4.fs \
	efusMX8MP-FERT5.fs \
	efusMX8MP-FERT13.fs \
	SM8MP-FERT3.fs \
	SM8MP-FERT4.fs \
	SM8MP-FERT5.fs \
	OSM8MPL-FERT2.fs \
	OSM8MPL-FERT3.fs


# List of DRAM Timings for each DRAM-TYPE
RT_LPDDR4 := \
	lpddr4_as_4g1500m2ch2cs_timing.fs \
	lpddr4_as_2g1600m2ch1cs_timing.fs \
	lpddr4_as_1g2000m2ch1cs_timing.fs \
	lpddr4_as_1g1800m2ch1cs_timing.fs \
	lpddr4_ef_2g1800m2ch1cs_timing.fs \
	lpddr4_ef_1g1800m2ch1cs_timing.fs \
	lpddr4_ef_1g2000m2ch1cs_timing.fs \
	lpddr4_os_2g2000m2ch1cs_timing.fs \
	lpddr4_os_1g2000m2ch1cs_timing.fs \
	lpddr4_pc_4g1500m2ch2cs_timing.fs \
	lpddr4_pc_2g1500m2ch1cs_timing.fs \
	lpddr4_pc_2g1300m2ch1cs_timing.fs \
	lpddr4_pc_1g1800m2ch1cs_timing.fs \
	lpddr4_pc_1g1500m2ch1cs_timing.fs \
	lpddr4_sm_2g2000m2ch1cs_timing.fs \
	lpddr4_sm_1g2000m2ch1cs_timing.fs
RT_DDR4 :=

# List of DRAM-TYPES for DRAM-SETTINGS image; only add those that have timings
DRAM_TYPES :=
ifneq ($(RT_LPDDR4),)
DRAM_TYPES += lpddr4_type.fs
endif
ifneq ($(RT_DDR4),)
DRAM_TYPES += ddr4_type.fs
endif

# Prepend the DDR Training Firmware as an additional file before the timings
RT_LPDDR4 := lpddr4_fw.fs $(RT_LPDDR4)
RT_DDR4 := ddr4_fw.fs $(RT_DDR4)

# List of files to add to EXTRA image
EXTRA_FILES = addfsheader.sh.fs fsimage.sh.fs

# List of files to add to NBOOT image
NBOOT_FILES = $(BOARD)_spl.fs $(BOARD)_board_info.fs \
	$(BOARD)_dram_info.fs $(BOARD)_atf.fs \
	$(if $(CONFIG_OPTEE),$(BOARD)_optee.fs) $(BOARD)_extra.fs

# Final NBOOT image
NBOOT = $(BOARD)_nboot.fs

# Add source tree path for board configurations
NBOOT_PATH = $(srctree)/board/$(BOARDDIR)/nboot
DTS_BOARD_CONFIGS = $(addprefix $(NBOOT_PATH)/, $(BOARD_CONFIGS:.fs,.dts))

# Add object path for F&S images
OBJ_BOARD_CONFIGS = $(addprefix $(obj)/, $(BOARD_CONFIGS))
OBJ_RT_LPDDR4 = $(addprefix $(obj)/, $(RT_LPDDR4))
OBJ_RT_DDR4 = $(addprefix $(obj)/, $(RT_DDR4))
OBJ_DRAM_TYPES = $(addprefix $(obj)/, $(DRAM_TYPES))
OBJ_EXTRA_FILES = $(addprefix $(obj)/, $(EXTRA_FILES))
OBJ_NBOOT_FILES = $(addprefix $(obj)/, $(NBOOT_FILES))
OBJ_NBOOT = $(addprefix $(obj)/, $(NBOOT))

# List of provided firmware files from NXP
NXP_LPDDR4 = lpddr4_pmu_train_1d_imem_202006.bin \
	lpddr4_pmu_train_1d_dmem_202006.bin \
	lpddr4_pmu_train_2d_imem_202006.bin \
	lpddr4_pmu_train_2d_dmem_202006.bin
NXP_DDR4 = ddr4_imem_1d_202006.bin ddr4_dmem_1d_202006.bin \
	ddr4_imem_2d_202006.bin ddr4_dmem_2d_202006.bin
ifeq ($(CONFIG_OPTEE),y)
NXP_ATF = bl31-optee.bin
NXP_TEE = bl32.bin
else
NXP_ATF = bl31.bin
endif

# Add path for NXP firmware images
NXP_PATH = $(srctree)/board/F+S/NXP-Firmware/$(BOARD)
NXP_PATH_LPDDR4 = $(addprefix $(NXP_PATH)/, $(NXP_LPDDR4))
NXP_PATH_DDR4 = $(addprefix $(NXP_PATH)/, $(NXP_DDR4))
NXP_PATH_ATF = $(addprefix $(NXP_PATH)/, $(NXP_ATF))
NXP_PATH_TEE = $(addprefix $(NXP_PATH)/, $(NXP_TEE))

# Concatenate two single files with padding: 32KB + x
quiet_cmd_dd2 = GROUP   $@
cmd_dd2 = dd if=$(word 1,$^) of=$@ bs=16k status=none; \
	dd if=$(word 2,$^) of=$@ bs=16k seek=2 conv=notrunc status=none

# Concatenate four single files with padding: 32KB + 16KB + 32KB + x
quiet_cmd_dd4 = GROUP   $@
cmd_dd4 = dd if=$(word 1,$^) of=$@ bs=16k status=none; \
	dd if=$(word 2,$^) of=$@ bs=16k seek=2 conv=notrunc status=none; \
	dd if=$(word 3,$^) of=$@ bs=16k seek=3 conv=notrunc status=none; \
	dd if=$(word 4,$^) of=$@ bs=16k seek=5 conv=notrunc status=none

.INTERMEDIATE: $(obj)/.lpddr4_fw.tmp $(obj)/.ddr4_fw.tmp

$(obj)/.lpddr4_fw.tmp: $(NXP_PATH_LPDDR4)
	$(call cmd,dd4)

$(obj)/.ddr4_fw.tmp: $(NXP_PATH_DDR4)
	$(call cmd,dd4)

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

$(obj)/lpddr4_type.fs: $(OBJ_RT_LPDDR4)
$(obj)/ddr4_type.fs: $(OBJ_RT_DDR4)

%_type.fs:
	$(call cmd,fsimg,-s -t DRAM-TYPE -d $(*F))

%_dram_info.fs: $(OBJ_DRAM_TYPES)
	$(call cmd,fsimg,-s -t DRAM-INFO -d $(*F))

%_atf.fs: $(NXP_PATH_ATF)
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_ATF_ADDR) -t ATF -d $(*F))

%_optee.fs: $(NXP_PATH_TEE)
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_SPL_TEE_ADDR) -t TEE -d $(*F))


MKIMAGEFLAGS_u-boot-spl-ivt.bin = -n spl/u-boot-spl.cfgout -T imx8mimage -e $(CONFIG_SPL_TEXT_BASE)
$(obj)/u-boot-spl-ivt.bin: MKIMAGEOUTPUT = $(obj)/u-boot-spl-ivt.log

$(obj)/u-boot-spl-ivt.bin: spl/u-boot-spl.bin FORCE
	$(call if_changed,mkimage)

%_spl.fs: $(obj)/u-boot-spl-ivt.bin
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
	$(call cmd,fsimg,-s -c -p32[7]=$(CONFIG_FUS_BOARDCFG_ADDR) -t BOARD-CFG -d $(*F))

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
