# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 F&S Elektronik Systeme GmbH
#
# Common macros used when building F&S images.
#
# F&S Images
# ----------

# An F&S image consists of a 64-Byte F&S header (FSH) and arbitrary image data
# (FSI), probably padded to a certain alignment. Besides size and padding
# information, the header also holds the image type (e.g. SPL, BOARD-CFG,
# DRAM-TIMING) and a type specific description (e.g. CPU architecture, board
# ID, DRAM chip type). It can also hold flags and parameters of size 8, 16, 32
# and 64 bits. The flags and parameters can be used to store application
# specific data together with the image data. A few flags are predifined, for
# example one flag tells whether the image has a CRC32 checksum added or not.
#
# An F&S image typically has the filename extension ".fs", a separate F&S
# header ".fsh" and separate image data ".fsi".
#
#   +-----------------------------------+
#   | FSH: TYPE (descr)                 |
#   |   +-------------------------------+
#   |   | FSI: Image data               |
#   +---+-------------------------------+
#
# F&S images may be concatenated.
#
#   +-----------------------------------+
#   | FSH: TYPE 1 (descr 1)             |
#   |   +-------------------------------+
#   |   | FSI: Image 1 data             |
#   +---+-------------------------------+
#   | FSH: TYPE 2 (descr 2)             |
#   |   +-------------------------------+
#   |   | FSI: Image 2 data             |
#   +---+-------------------------------+
#
# The F&S header can also be used to group several images. Then the top header
# can be used to hold information for the whole group.
#
#   +-----------------------------------+
#   | FSH: TYPE 0 (descr 0)             | Header for group
#   |   +-------------------------------+
#   |   | FSH: TYPE 1 (descr 1)         |
#   |   |   +---------------------------+
#   |   |   | FSI: Image 1 data         |
#   |   +-------------------------------+
#   |   | FSH: TYPE 2 (descr 2)         |
#   |   |   +---------------------------+
#   |   |   | FSI: Image 2 data         |
#   +---+---+---------------------------+
#
# An F&S image may even represent a whole hierarchy of images.
#
#   +-----------------------------------+
#   | FSH: TYPE 0 (descr 0)             |
#   |   +-------------------------------+
#   |   | FSH: TYPE 1 (descr 1)         |
#   |   |   +---------------------------+
#   |   |   | FSI: Image 1 data         |
#   |   +---+---------------------------+
#   |   | FSH: TYPE 2 (descr 2)         |
#   |   |   +---------------------------+
#   |   |   | FSI: Image 2 data         |
#   |   +---+---------------------------+
#   |   | FSH: TYPE 3 (descr 3)         |
#   |   |   +---------------------------+
#   |   |   | FSH: TYPE 3.1 (descr 3.1) |
#   |   |   |   +-----------------------+
#   |   |   |   | FSI: Image 3.1 data   |
#   |   |   +---+-----------------------+
#   |   |   | FSH: TYPE 3.2 (descr 3.2) |
#   |   |   |   +-----------------------+
#   |   |   |   | FSI: Image 3.2 data   |
#   |   +---+---------------------------+
#   |   | FSH: TYPE 4 (descr 4)         |
#   |   |   +---------------------------+
#   |   |   | FSI: Image 4 data         |
#   +---+---+---------------------------+
#
# The tool scripts/addfsheader.sh is used to add an F&S header to one or more
# images, grouping them as an F&S image. For hierarchical structures, several
# calls to addfsheader.sh are necessary, one for each header in the hierarchy.
#
# The tool scripts/fsimage.sh is used to list contents of an F&S image and to
# extract single images again.
#
# F&S Containers
# --------------
# Many i.MX CPUs use an NXP specific container format for booting. A container
# may hold several images, for example boot software for all cores (Cortex-A,
# Cortex-M), but also firmwares for DSPs, security and power controllers.
# U-Boot is also often grouped together with ATF and opTEE in such a container.
#
# A container consists of a header, that holds size information and signature
# information. It also holds a list of image references for all included
# images. Each reference holds the size and type of an image and a pointer to
# the offset, where the real image begins. The single images as well as the
# container itself typically need to be aligned to 1KB boundaries.
#
# A container is built by calling tools/mkimage. The list of images to be
# included is defined in a .cfg configuration file that is passed to mkimage.
#
#   +---------------------------+
#   | Container Header          |
#   |     Image Reference 1     |---+
#   |     Image Reference 2     |---|--+
#   |     ...                   |---|--|--+
#   +---------------------------+   |  |  |
#   | Image 1 data              |<--+  |  |
#   +---------------------------+      |  |
#   | Image 2 data              |<-----+  |
#   +---------------------------+         |
#   | ...                       |<--------+
#   +---------------------------+
#
# F&S uses a combination of F&S images and i.MX container format to keep the
# additional information of image type, description, flags and parameters. Such
# F&S containers can then be listed and extracted like any other F&S image.
#
# However the basic idea to just prepend an F&S header to each image does not
# work (see left side). It violates the alignment of the images or requires
# modifications of the mkimage tool. This is where INDEX images come into play.
# Instead of prepending the F&S header to each image, all these F&S headers are
# collected in a separate INDEX image and this image is inserted as first image
# in the image list (see right side). Apart from that new image, the container
# part itself remains the same as before.
#
#   Basic idea:                         Final implementation with INDEX:
#   +---------------------------+       +-------------------------------+
#   | FSH: TYPE 0 (descr 0)     |       | FSH: TYPE 0 (descr 0)         |
#   |   +-----------------------+       |   +---------------------------+
#   |   | Container Header      |       |   | Container Header          |
#   |   |     Image Reference 1 |       |   |     INDEX Reference       | new
#   |   |     Image Reference 2 |       |   |     Image Reference 1     |
#   |   |     ...               |       |   |     Image Reference 2     |
#   |   +-----------------------+       |   |     ...                   |
#   |   | FSH: TYPE 1 (descr 1) |---+   |   +---------------------------+
#   |   |   +-------------------+   |   |   | FSH: INDEX                |\
#   |   |   | FSI: Image 1 data |   |   |   |   +-----------------------+ |
#   |   +---+-------------------+   +-->|   |   | FSH: TYPE 1 (descr 1) | | new
#   |   | FSH: TYPE 2 (descr 2) |------>|   |   | FSH: TYPE 2 (descr 2) | |
#   |   |   +-------------------+   +-->|   |   | FSH: ...              |/
#   |   |   | FSI: Image 2 data |   |   |   +---+-----------------------+
#   |   +---+-------------------+   |   |   | FSI: Image 1 data         |
#   |   | FSH: ...              |---+   |   +---------------------------+
#   |   |   +-------------------+       |   | FSI: Image 2 data         |
#   |   |   | FSI: ...          |       |   +---------------------------+
#   +---+---+-------------------+       |   | FSI: ...                  |
#                                       +---+---------------------------+

NXP_PATH = $(srctree)/board/$(VENDOR)/NXP-Firmware/$(BOARD)
SRC_PATH = $(srctree)/$(src)

FSIMG = $(srctree)/scripts/addfsheader.sh
FSIMG_FSH = $(FSIMG) -q -c -a 0x400 -j
#FSIMG_FSH = $(FSIMG) -c -a 0x400 -j

quiet_cmd_fsimg = FSIMG   $@
      cmd_fsimg = $(FSIMG) -q $2 > $@

quiet_cmd_copy = COPY    $@
      cmd_copy = cp $< $@

quiet_cmd_sed = SED     $@
      cmd_sed = sed $2 $< > $@

quiet_cmd_cat = CAT     $@
      cmd_cat = cat $^ > $@

quiet_cmd_dram_cc = CC      $@
      cmd_dram_cc = $(CC) $(c_flags) -c -o $@ $<

quiet_cmd_dram_elf = LINK    $@
      cmd_dram_elf = $(LD) -T $(2) -o $@ $<

quiet_cmd_dram_bin = OBJCOPY $@
      cmd_dram_bin = $(OBJCOPY) --gap-fill=0x00 -O binary $< $@

# sed commands to replace path name variables in mkimage cfg files
SED_PATHS = \
	-e 's:$$(obj):$(obj):' \
	-e 's:$$(src):$(SRC_PATH):' \
	-e 's:$$(nxp):$(NXP_PATH):'

# sed command to extract the image filenames from an mkimage cfg file
SED_FILES = \
	-e '/^\(DATA\|IMAGE\)/s/\S\+\s\+\S\+\s\+\(\S\+\).*/\1/p'

quiet_cmd_cfg = CFG     $@
      cmd_cfg = sed $(SED_PATHS) $(2) $< > $@

fstype = $(lastword $(subst _, ,$(basename $(notdir $(1)))))
fsdescr = $(subst _$(call fstype,$(1)),,$(basename $(notdir $(1))))
quiet_cmd_index = INDEX   $@
      cmd_index =\
	($(foreach image, $^, \
		$(FSIMG_FSH) -t $(call fstype,$(image)) \
				-d $(call fsdescr,$(image)) $(image);)) \
	| $(FSIMG) -q -c -w -a 0x400 -i -t $(call fstype,$@) > $@

# -----------------------------------------------------------------------------
#
# Rules to build F&S container (fsc) from single files
#
# Usage
# -----
# To build container <c>.fs (e.g. board_info.fs), follow these steps:
#
# 1. Create <c>.cfg with a list of all images required to build the container
#    with mkimage.
# 2. Create rules that build all these images.
# 3. Call $(eval $(call fsc,<c>,<a>)). This automatically creates all the
#    rules needed to build the final F&S container <c>.fs, including the INDEX
#    image.
#
# argument $(1): Name <c> of F&S container to build, e.g. board_info; this is
#                used to build file names and variable names. Therefore avoid
#                '-' in <c> and use '_' instead.
# argument $(2): Additional arguments to add to the addfsheader command for the
#                final container <c>.fs. This typically includes options -a, -t
#                and -d for alignment, image type and image description.
# argument $(3): Additional sed script to apply on cfg file (optional).
#
# Data flow:
# 1. <c>.cfg --> (cmd_cfg/sed) --> .<c>.cfg.tmp
# 2. images of .<c>.cfg.tmp --> (cmd_index) --> <c>_INDEX.fs
# 3. images of .<c>.cfg.tmp --> (mkimage with .<c>_cfg.tmp) --> .<c>_cntr.tmp
# 4. .<c>_cntr.tmp --> addfsheader --> <c>.fs

define fsc
# Define shortcuts for used filenames

$(1)_cfg := $$(srctree)/$$(src)/$(1).cfg
$(1)_cfg_tmp := $$(obj)/.$(1).cfg.tmp
$(1)_images := $$(shell sed -n $$(SED_PATHS) $(3) $$(SED_FILES) $$($(1)_cfg))
$(1)_index := $$(word 1,$$($(1)_images))
$(1)_others := $$(wordlist 2,$$(words $$($(1)_images)),$$($(1)_images))

# 1. Replace path variables in cfg file
$$($(1)_cfg_tmp): $$($(1)_cfg)
	$$(call cmd,cfg,$(3))

# 2. Build INDEX by building F&S headers for all images
$$($(1)_index): $$($(1)_others)
	$$(call cmd,index)

MKIMAGEFLAGS_.$(1)_cntr.tmp = \
	-n $$($(1)_cfg_tmp) -T $$(MKIMAGE_TYPE) -e $$(CONFIG_TEXT_BASE)

# 3. Build the container by calling mkimage
$$(obj)/.$(1)_log.tmp: $$(obj)/.$(1)_cntr.tmp
$$(obj)/.$(1)_cntr.tmp: MKIMAGEOUTPUT := $$(obj)/.$(1)_log.tmp
$$(obj)/.$(1)_cntr.tmp: /dev/null $$($(1)_index) $$($(1)_cfg_tmp)
#	$$(Q)$(srctree)/tools/imx_cntr_image.sh $$(obj)/.$(1)_cfg.tmp
	$$(call if_changed,mkimage)

# 4. Add F&S header (with extra offset to the INDEX) for the final F&S image
$$(obj)/$(1).fs: $$(obj)/.$(1)_cntr.tmp $$(obj)/.$(1)_log.tmp
	$$(eval $(1)_index_offs := \
		$$(shell grep -oP 'image offset \(aligned\): \K[^ ]+' \
			 $$(obj)/.$(1)_log.tmp))
	$$(call cmd,fsimg, -c -w -e $$($(1)_index_offs) $(2) $$<)

endef
