# SPDX-License-Identifier: GPL-2.0+

obj-y   += fsimx8ulp.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_SOLDERCORE8ULP_LPDDR4_FORESEE_1GBYTE) += lpddr4_FL4C2001G-D9_timing.o
obj-$(CONFIG_PICOCOREMX8ULP_LPDDR4_NANYA_2GBYTE) += lpddr4_NT6AN512T32AV-J2I_timing.o
else
obj-y				+= ../../freescale/common/mmc.o
endif #CONFIG_SPL_BUILD
